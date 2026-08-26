/**
 * @file vesc_uart.h
 * @brief Minimal VESC UART helper for STM32H7 (USART1).
 */
#ifndef VESC_UART_H
#define VESC_UART_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

typedef struct {
  float tempMosfet;
  float avgMotorCurrent;
  float avgInputCurrent;
  float rpm;
  float inpVoltage;
  float ampHours;
  float tachometerAbs;
} vesc_values_t;

void VESC_UART_Init(UART_HandleTypeDef *huart);
void VESC_UART_Poll(uint32_t now_ms, int current_throttle, vesc_values_t *out);

#endif /* VESC_UART_H */

