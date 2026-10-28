#include "vesc_uart.h"

#include <string.h>

#define VESC_START_SHORT UINT8_C(2)
#define VESC_START_LONG UINT8_C(3)
#define VESC_END UINT8_C(3)
#define VESC_COMM_GET_VALUES UINT8_C(4)
#define VESC_COMM_SET_RPM UINT8_C(8)

enum
{
  VESC_RX_WAIT_START = 0,
  VESC_RX_LENGTH_1,
  VESC_RX_LENGTH_2,
  VESC_RX_PAYLOAD,
  VESC_RX_CRC_HIGH,
  VESC_RX_CRC_LOW,
  VESC_RX_END
};

static void PutBe32(uint8_t *output, uint32_t value)
{
  output[0] = (uint8_t)(value >> 24U);
  output[1] = (uint8_t)(value >> 16U);
  output[2] = (uint8_t)(value >> 8U);
  output[3] = (uint8_t)value;
}

static uint16_t GetBe16(const uint8_t *input)
{
  return (uint16_t)(((uint16_t)input[0] << 8U) | input[1]);
}

static int32_t GetBeI32(const uint8_t *input)
{
  return (int32_t)(((uint32_t)input[0] << 24U) | ((uint32_t)input[1] << 16U) |
                   ((uint32_t)input[2] << 8U) | input[3]);
}

uint16_t VescUart_Crc16(const uint8_t *data, uint16_t length)
{
  uint16_t crc = 0U;
  uint16_t index;
  if ((data == NULL) && (length != 0U)) return 0U;
  for (index = 0U; index < length; ++index)
  {
    uint8_t bit;
    crc ^= (uint16_t)data[index] << 8U;
    for (bit = 0U; bit < 8U; ++bit)
    {
      crc = ((crc & UINT16_C(0x8000)) != 0U) ?
                (uint16_t)((crc << 1U) ^ UINT16_C(0x1021)) :
                (uint16_t)(crc << 1U);
    }
  }
  return crc;
}

static void ResetParser(vesc_uart_t *device)
{
  device->state = VESC_RX_WAIT_START;
  device->expected_payload_length = 0U;
  device->received_payload_length = 0U;
  device->received_crc = 0U;
}

static vesc_uart_result_t SendPayload(vesc_uart_t *device, const uint8_t *payload,
                                      uint16_t payload_length)
{
  uint8_t frame[VESC_UART_MAX_PAYLOAD + 6U];
  uint16_t crc;
  uint16_t frame_length;

  if ((device == NULL) || (device->send == NULL) || (payload == NULL) ||
      (payload_length == 0U) || (payload_length > VESC_UART_MAX_PAYLOAD))
  {
    return VESC_UART_ARGUMENT_ERROR;
  }
  frame[0] = VESC_START_SHORT;
  frame[1] = (uint8_t)payload_length;
  memcpy(&frame[2], payload, payload_length);
  crc = VescUart_Crc16(payload, payload_length);
  frame[2U + payload_length] = (uint8_t)(crc >> 8U);
  frame[3U + payload_length] = (uint8_t)crc;
  frame[4U + payload_length] = VESC_END;
  frame_length = (uint16_t)(payload_length + 5U);
  if (!device->send(device->context, frame, frame_length))
  {
    ++device->tx_errors;
    return VESC_UART_TRANSMIT_ERROR;
  }
  return VESC_UART_OK;
}

void VescUart_Init(vesc_uart_t *device, void *context, vesc_uart_send_fn send)
{
  if (device == NULL) return;
  memset(device, 0, sizeof(*device));
  device->context = context;
  device->send = send;
  ResetParser(device);
}

vesc_uart_result_t VescUart_SetRpm(vesc_uart_t *device, int32_t rpm)
{
  uint8_t payload[5];
  payload[0] = VESC_COMM_SET_RPM;
  PutBe32(&payload[1], (uint32_t)rpm);
  return SendPayload(device, payload, sizeof(payload));
}

vesc_uart_result_t VescUart_Stop(vesc_uart_t *device)
{
  return VescUart_SetRpm(device, 0);
}

vesc_uart_result_t VescUart_RequestValues(vesc_uart_t *device, uint32_t now_ms)
{
  const uint8_t payload[] = {VESC_COMM_GET_VALUES};
  vesc_uart_result_t result = SendPayload(device, payload, sizeof(payload));
  if (result == VESC_UART_OK) device->last_values_request_ms = now_ms;
  return result;
}

static vesc_uart_result_t DecodePayload(vesc_uart_t *device, uint32_t now_ms)
{
  /* COMM_GET_VALUES layout through input voltage, as specified by the VESC
   * UART protocol.  Additional firmware-specific fields are ignored. */
  if ((device->expected_payload_length < 33U) ||
      (device->payload[0] != VESC_COMM_GET_VALUES))
  {
    return VESC_UART_UNSUPPORTED_PAYLOAD;
  }
  device->latest_values.motor_current_ma = GetBeI32(&device->payload[5]);
  device->latest_values.input_current_ma = GetBeI32(&device->payload[9]);
  device->latest_values.rpm = GetBeI32(&device->payload[27]);
  device->latest_values.input_voltage_dv = GetBe16(&device->payload[31]);
  device->latest_values.received_at_ms = now_ms;
  device->latest_values.valid = true;
  return VESC_UART_OK;
}

vesc_uart_result_t VescUart_ProcessByte(vesc_uart_t *device, uint8_t byte,
                                         uint32_t now_ms)
{
  vesc_uart_result_t result = VESC_UART_OK;
  if (device == NULL) return VESC_UART_ARGUMENT_ERROR;

  switch (device->state)
  {
    case VESC_RX_WAIT_START:
      if (byte == VESC_START_SHORT)
      {
        device->state = VESC_RX_LENGTH_1;
      }
      else if (byte == VESC_START_LONG)
      {
        device->state = VESC_RX_LENGTH_2;
        device->expected_payload_length = UINT16_C(0xFFFF);
      }
      break;
    case VESC_RX_LENGTH_1:
      device->expected_payload_length = byte;
      if ((byte == 0U) || (byte > VESC_UART_MAX_PAYLOAD))
      {
        ++device->frame_errors;
        ResetParser(device);
        return VESC_UART_FRAME_ERROR;
      }
      device->received_payload_length = 0U;
      device->state = VESC_RX_PAYLOAD;
      break;
    case VESC_RX_LENGTH_2:
      if (device->expected_payload_length == UINT16_C(0xFFFF))
      {
        device->expected_payload_length = (uint16_t)byte << 8U;
      }
      else
      {
        device->expected_payload_length |= byte;
        if ((device->expected_payload_length == 0U) ||
            (device->expected_payload_length > VESC_UART_MAX_PAYLOAD))
        {
          ++device->frame_errors;
          ResetParser(device);
          return VESC_UART_FRAME_ERROR;
        }
        device->received_payload_length = 0U;
        device->state = VESC_RX_PAYLOAD;
      }
      break;
    case VESC_RX_PAYLOAD:
      device->payload[device->received_payload_length++] = byte;
      if (device->received_payload_length == device->expected_payload_length)
      {
        device->state = VESC_RX_CRC_HIGH;
      }
      break;
    case VESC_RX_CRC_HIGH:
      device->received_crc = (uint16_t)byte << 8U;
      device->state = VESC_RX_CRC_LOW;
      break;
    case VESC_RX_CRC_LOW:
      device->received_crc |= byte;
      device->state = VESC_RX_END;
      break;
    case VESC_RX_END:
      if ((byte != VESC_END) ||
          (device->received_crc != VescUart_Crc16(device->payload,
                                                   device->expected_payload_length)))
      {
        ++device->crc_errors;
        result = VESC_UART_CRC_ERROR;
      }
      else
      {
        result = DecodePayload(device, now_ms);
      }
      ResetParser(device);
      return result;
    default:
      ++device->frame_errors;
      ResetParser(device);
      return VESC_UART_FRAME_ERROR;
  }
  return result;
}

bool VescUart_IsHealthy(const vesc_uart_t *device, uint32_t now_ms)
{
  return (device != NULL) && device->latest_values.valid &&
         ((uint32_t)(now_ms - device->latest_values.received_at_ms) <=
          VESC_UART_VALUES_TIMEOUT_MS);
}
