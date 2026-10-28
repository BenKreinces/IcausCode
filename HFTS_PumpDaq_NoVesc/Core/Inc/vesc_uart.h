/**
 * @file vesc_uart.h
 * @brief Minimal VESC UART helper for COMM_SET_RPM and COMM_GET_VALUES.
 *
 * Frame format (UART):
 *  - Start: 0x02 (short packet)
 *  - Length: 1 byte, payload length
 *  - Payload: [cmd_id, ...]
 *  - CRC16: high, low (over payload only, poly 0x1021, init 0)
 *  - Stop: 0x03
 *
 * Only a subset of COMM_GET_VALUES fields are parsed into vesc_values_t.
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

/**
 * @brief Poll VESC at ~10 Hz: send COMM_SET_RPM with current_throttle and
 *        request/parse COMM_GET_VALUES into out (if non-NULL).
 *
 * @param now_ms          current HAL_GetTick() in ms (for future use)
 * @param current_throttle eRPM command (already scaled from host)
 * @param out             optional pointer to store latest values
 */
void VESC_UART_Poll(uint32_t now_ms, int current_throttle, vesc_values_t *out);

#endif /* VESC_UART_H */

