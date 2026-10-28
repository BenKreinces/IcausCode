#include "vesc_uart_stm32_hal.h"

static bool Send(void *context, const uint8_t *data, uint16_t length)
{
  vesc_uart_stm32_hal_t *transport = context;
  return HAL_UART_Transmit(transport->uart, (uint8_t *)data, length,
                           transport->timeout_ms) == HAL_OK;
}
bool VescUart_Stm32HalBind(vesc_uart_t *device, vesc_uart_stm32_hal_t *transport,
                           UART_HandleTypeDef *uart, uint32_t timeout_ms)
{
  if ((device == NULL) || (transport == NULL) || (uart == NULL) || (timeout_ms == 0U))
    return false;
  transport->uart = uart; transport->timeout_ms = timeout_ms;
  VescUart_Init(device, transport, Send);
  return true;
}
