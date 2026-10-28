/**
 * @file vesc_uart.c
 * @brief Minimal VESC UART helper for STM32H7 (USART1).
 *
 * On each call to VESC_UART_Poll():
 *  - send COMM_SET_RPM with the provided current_throttle (eRPM)
 *  - send COMM_GET_VALUES, then attempt to read and parse one response frame.
 *
 * Field order and scaling follow the Teensy HFTS_model_arduino_code comment.
 */

#include "vesc_uart.h"

/* VESC command IDs (subset) */
#define COMM_GET_VALUES  4u
#define COMM_SET_RPM     8u

/* UART frame markers (short frame) */
#define VESC_START_BYTE  0x02u
#define VESC_STOP_BYTE   0x03u

static UART_HandleTypeDef *s_huart = NULL;

static uint16_t vesc_crc16(const uint8_t *data, uint16_t len)
{
  uint16_t crc = 0u;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= ((uint16_t)data[i]) << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000u) {
        crc = (uint16_t)((crc << 1) ^ 0x1021u);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

static void vesc_send_frame(const uint8_t *payload, uint8_t payload_len)
{
  if (!s_huart || !payload || payload_len == 0u) {
    return;
  }

  uint8_t frame[1 + 1 + 64 + 2 + 1];
  if (payload_len > 64u) {
    return;
  }

  uint16_t idx = 0;
  frame[idx++] = VESC_START_BYTE;
  frame[idx++] = payload_len;
  for (uint8_t i = 0; i < payload_len; i++) {
    frame[idx++] = payload[i];
  }
  uint16_t crc = vesc_crc16(payload, payload_len);
  frame[idx++] = (uint8_t)(crc >> 8);
  frame[idx++] = (uint8_t)(crc & 0xFFu);
  frame[idx++] = VESC_STOP_BYTE;

  (void)HAL_UART_Transmit(s_huart, frame, idx, 10);
}

void VESC_UART_Init(UART_HandleTypeDef *huart)
{
  s_huart = huart;
}

static int16_t buf_get_int16(const uint8_t *buf, uint16_t *index)
{
  int16_t res = (int16_t)((buf[*index] << 8) | buf[*index + 1u]);
  *index += 2u;
  return res;
}

static int32_t buf_get_int32(const uint8_t *buf, uint16_t *index)
{
  int32_t res = ((int32_t)buf[*index] << 24) |
                ((int32_t)buf[*index + 1u] << 16) |
                ((int32_t)buf[*index + 2u] << 8) |
                ((int32_t)buf[*index + 3u]);
  *index += 4u;
  return res;
}

static float buf_get_float16_scaled(const uint8_t *buf, uint16_t *index, float scale)
{
  int16_t v = buf_get_int16(buf, index);
  return (float)v / scale;
}

static float buf_get_float32_scaled(const uint8_t *buf, uint16_t *index, float scale)
{
  int32_t v = buf_get_int32(buf, index);
  return (float)v / scale;
}

static void vesc_try_parse_values(const uint8_t *payload, uint8_t payload_len, vesc_values_t *out)
{
  if (!payload || payload_len < 5u || !out) {
    return;
  }
  if (payload[0] != COMM_GET_VALUES) {
    return;
  }

  uint16_t idx = 1u;

  (void)buf_get_float16_scaled(payload, &idx, 10.0f);      /* tempMotor - unused */
  out->tempMosfet = buf_get_float16_scaled(payload, &idx, 10.0f);

  out->avgMotorCurrent = buf_get_float32_scaled(payload, &idx, 100.0f);
  out->avgInputCurrent = buf_get_float32_scaled(payload, &idx, 100.0f);

  idx += 4u; /* avg_id */
  idx += 4u; /* avg_iq */

  (void)buf_get_float16_scaled(payload, &idx, 1000.0f);    /* dutyCycleNow - unused */

  out->rpm = buf_get_float32_scaled(payload, &idx, 1.0f);
  out->inpVoltage = buf_get_float16_scaled(payload, &idx, 10.0f);

  out->ampHours = buf_get_float32_scaled(payload, &idx, 10000.0f);

  idx += 4u * 3u; /* ampHoursCharged, wattHours, wattHoursCharged */

  (void)buf_get_int32(payload, &idx); /* tachometer - unused */
  out->tachometerAbs = (float)buf_get_int32(payload, &idx);
}

void VESC_UART_Poll(uint32_t now_ms, int current_throttle, vesc_values_t *out)
{
  (void)now_ms;
  if (!s_huart) {
    return;
  }

  {
    uint8_t pl[1 + 4];
    int32_t rpm_cmd = (int32_t)current_throttle;
    pl[0] = COMM_SET_RPM;
    pl[1] = (uint8_t)((rpm_cmd >> 24) & 0xFF);
    pl[2] = (uint8_t)((rpm_cmd >> 16) & 0xFF);
    pl[3] = (uint8_t)((rpm_cmd >> 8) & 0xFF);
    pl[4] = (uint8_t)(rpm_cmd & 0xFF);
    vesc_send_frame(pl, sizeof(pl));
  }

  {
    uint8_t pl = COMM_GET_VALUES;
    vesc_send_frame(&pl, 1u);
  }

  uint8_t rx[96];
  uint16_t rx_len = 0u;

  if (HAL_UART_Receive(s_huart, rx, sizeof(rx), 20) != HAL_OK) {
    return;
  }
  rx_len = (uint16_t)sizeof(rx);

  for (uint16_t i = 0; i + 5u < rx_len; i++) {
    if (rx[i] != VESC_START_BYTE) {
      continue;
    }
    uint8_t len = rx[i + 1u];
    uint16_t frame_total = (uint16_t)(1u + 1u + len + 2u + 1u);
    if (i + frame_total > rx_len) {
      continue;
    }
    const uint8_t *payload = &rx[i + 2u];
    uint16_t crc_rx = ((uint16_t)rx[i + 2u + len] << 8) |
                      ((uint16_t)rx[i + 2u + len + 1u]);
    uint16_t crc_calc = vesc_crc16(payload, len);
    if (crc_rx != crc_calc || rx[i + frame_total - 1u] != VESC_STOP_BYTE) {
      continue;
    }
    vesc_try_parse_values(payload, len, out);
    break;
  }
}

