/** @file vesc_uart.h @brief Bounded VESC UART framing and health driver. */
#ifndef ICARUS_VESC_UART_H
#define ICARUS_VESC_UART_H

#include <stdbool.h>
#include <stdint.h>

#define VESC_UART_MAX_PAYLOAD UINT16_C(128)
#define VESC_UART_VALUES_TIMEOUT_MS UINT32_C(250)

typedef bool (*vesc_uart_send_fn)(void *context, const uint8_t *data,
                                  uint16_t length);

typedef enum
{
  VESC_UART_OK = 0,
  VESC_UART_ARGUMENT_ERROR,
  VESC_UART_TRANSMIT_ERROR,
  VESC_UART_FRAME_ERROR,
  VESC_UART_CRC_ERROR,
  VESC_UART_UNSUPPORTED_PAYLOAD
} vesc_uart_result_t;

typedef struct
{
  int32_t rpm;
  int32_t motor_current_ma;
  int32_t input_current_ma;
  uint16_t input_voltage_dv;
  uint32_t received_at_ms;
  bool valid;
} vesc_uart_values_t;

typedef struct
{
  void *context;
  vesc_uart_send_fn send;
  uint8_t state;
  uint16_t expected_payload_length;
  uint16_t received_payload_length;
  uint16_t received_crc;
  uint8_t payload[VESC_UART_MAX_PAYLOAD];
  vesc_uart_values_t latest_values;
  uint32_t tx_errors;
  uint32_t frame_errors;
  uint32_t crc_errors;
  uint32_t last_values_request_ms;
} vesc_uart_t;

void VescUart_Init(vesc_uart_t *device, void *context, vesc_uart_send_fn send);
vesc_uart_result_t VescUart_SetRpm(vesc_uart_t *device, int32_t rpm);
vesc_uart_result_t VescUart_Stop(vesc_uart_t *device);
vesc_uart_result_t VescUart_RequestValues(vesc_uart_t *device, uint32_t now_ms);
vesc_uart_result_t VescUart_ProcessByte(vesc_uart_t *device, uint8_t byte,
                                         uint32_t now_ms);
bool VescUart_IsHealthy(const vesc_uart_t *device, uint32_t now_ms);
uint16_t VescUart_Crc16(const uint8_t *data, uint16_t length);

#endif /* ICARUS_VESC_UART_H */
