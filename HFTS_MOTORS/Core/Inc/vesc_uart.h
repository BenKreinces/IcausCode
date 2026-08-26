#pragma once
/*
 * Minimal VESC UART protocol (packet) implementation.
 *
 * - Pure C (C99), no dynamic allocation
 * - Incremental byte-by-byte RX parser state machine
 * - TX via user callback (HAL wrapper lives in application code)
 */

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float tempMosfet;
    float avgMotorCurrent;
    float avgInputCurrent;
    float rpm;
    float inpVoltage;
    float ampHours;
    float tachometerAbs;
} VescValues;

typedef void (*vesc_tx_fn_t)(const uint8_t *data, uint16_t len);
typedef void (*vesc_values_cb_t)(const VescValues *values);

typedef enum {
    VESC_RX_WAIT_START = 0,
    VESC_RX_LEN_1,
    VESC_RX_LEN_2,
    VESC_RX_PAYLOAD,
    VESC_RX_CRC_1,
    VESC_RX_CRC_2,
    VESC_RX_END
} VescRxState;

typedef struct {
    vesc_tx_fn_t tx_cb;
    vesc_values_cb_t values_cb;

    VescRxState rx_state;
    bool rx_long_frame;
    uint16_t rx_len;
    uint16_t rx_idx;
    uint16_t rx_crc_rx;
    uint16_t rx_crc_calc;

    uint8_t rx_buf[128];
} VescUart;

void vesc_init(VescUart *v, vesc_tx_fn_t tx_cb, vesc_values_cb_t values_cb);

void vesc_set_rpm(VescUart *v, int32_t rpm);
void vesc_request_values(VescUart *v);
void vesc_send_alive(VescUart *v);

void vesc_process_rx_byte(VescUart *v, uint8_t byte);

