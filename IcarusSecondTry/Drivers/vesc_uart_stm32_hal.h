/** @file vesc_uart_stm32_hal.h @brief STM32 HAL TX binding for VESC UART. */
#ifndef ICARUS_VESC_UART_STM32_HAL_H
#define ICARUS_VESC_UART_STM32_HAL_H

#include "main.h"
#include "vesc_uart.h"

typedef struct { UART_HandleTypeDef *uart; uint32_t timeout_ms; } vesc_uart_stm32_hal_t;
bool VescUart_Stm32HalBind(vesc_uart_t *device, vesc_uart_stm32_hal_t *transport,
                           UART_HandleTypeDef *uart, uint32_t timeout_ms);

#endif
