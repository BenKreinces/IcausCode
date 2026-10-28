#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "vesc_uart.h"

typedef struct
{
  uint8_t frame[140];
  uint16_t length;
} fake_uart_t;

static bool Send(void *context, const uint8_t *data, uint16_t length)
{
  fake_uart_t *uart = context;
  assert(length <= sizeof(uart->frame));
  memcpy(uart->frame, data, length);
  uart->length = length;
  return true;
}

static void PutBe32(uint8_t *data, uint32_t value)
{
  data[0] = (uint8_t)(value >> 24U);
  data[1] = (uint8_t)(value >> 16U);
  data[2] = (uint8_t)(value >> 8U);
  data[3] = (uint8_t)value;
}

static void FeedValues(vesc_uart_t *vesc, uint8_t payload[33], uint32_t now_ms)
{
  const uint16_t crc = VescUart_Crc16(payload, 33U);
  uint16_t index;
  assert(VescUart_ProcessByte(vesc, 2U, now_ms) == VESC_UART_OK);
  assert(VescUart_ProcessByte(vesc, 33U, now_ms) == VESC_UART_OK);
  for (index = 0U; index < 33U; ++index)
  {
    assert(VescUart_ProcessByte(vesc, payload[index], now_ms) == VESC_UART_OK);
  }
  assert(VescUart_ProcessByte(vesc, (uint8_t)(crc >> 8U), now_ms) == VESC_UART_OK);
  assert(VescUart_ProcessByte(vesc, (uint8_t)crc, now_ms) == VESC_UART_OK);
  assert(VescUart_ProcessByte(vesc, 3U, now_ms) == VESC_UART_OK);
}

int main(void)
{
  fake_uart_t uart;
  vesc_uart_t vesc;
  uint8_t payload[33];

  memset(&uart, 0, sizeof(uart));
  VescUart_Init(&vesc, &uart, Send);
  assert(VescUart_SetRpm(&vesc, -1200) == VESC_UART_OK);
  assert(uart.length == 10U && uart.frame[0] == 2U && uart.frame[1] == 5U);
  assert(uart.frame[2] == 8U && uart.frame[3] == 0xFFU && uart.frame[4] == 0xFFU);
  assert(VescUart_RequestValues(&vesc, 50U) == VESC_UART_OK);
  assert(uart.frame[2] == 4U && vesc.last_values_request_ms == 50U);

  memset(payload, 0, sizeof(payload));
  payload[0] = 4U;
  PutBe32(&payload[5], 12345U);
  PutBe32(&payload[9], 6789U);
  PutBe32(&payload[27], 4321U);
  payload[31] = 0x01U;
  payload[32] = 0xF4U;
  FeedValues(&vesc, payload, 100U);
  assert(vesc.latest_values.valid && vesc.latest_values.rpm == 4321);
  assert(vesc.latest_values.motor_current_ma == 12345);
  assert(vesc.latest_values.input_voltage_dv == 500U);
  assert(VescUart_IsHealthy(&vesc, 349U));
  assert(!VescUart_IsHealthy(&vesc, 351U));
  puts("VESC UART tests passed");
  return 0;
}
