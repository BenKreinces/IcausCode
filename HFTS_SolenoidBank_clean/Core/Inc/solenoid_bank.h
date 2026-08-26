/**
 * @file solenoid_bank.h
 * @brief Solenoid bank API: one MCP23S17 driving 12 outputs (solenoids 1–12).
 *
 * Hardware: MCP23S17T-E/SS → three TPS4H160AQPWPRQ1.
 * Solenoid output order 1–12: B4, B5, B6, B7, A2, A1, A0, A3, A6, A7, B0, B1.
 * DIAGEN and fault reads are reserved for later; this API is outputs only.
 *
 * Usage: call solenoid_bank_init() once with your SPI and CS pin; then
 * solenoid_bank_set(n, on) and solenoid_bank_all_off() as needed.
 */

#ifndef SOLENOID_BANK_H
#define SOLENOID_BANK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle; implementation uses STM32 HAL (SPI_HandleTypeDef, GPIO_TypeDef). */
struct solenoid_bank_impl;
typedef struct solenoid_bank_impl *solenoid_bank_handle_t;

/**
 * Initialize the solenoid bank.
 * - Uses the given SPI handle and CS pin (e.g. &hspi4, GPIOE, GPIO_PIN_3).
 * - All 12 solenoid outputs are set to 0.
 * @param hspi        SPI handle (e.g. &hspi4).
 * @param cs_port     CS GPIO port (e.g. GPIOE).
 * @param cs_pin      CS pin (e.g. GPIO_PIN_3).
 * @param timeout_ms  SPI transfer timeout in ms (0 = use default).
 * @return Handle to use with set/all_off, or NULL on failure.
 */
solenoid_bank_handle_t solenoid_bank_init(void *hspi, void *cs_port, uint16_t cs_pin, uint32_t timeout_ms);

/**
 * Set a single solenoid on or off.
 * @param bank     Handle from solenoid_bank_init().
 * @param solenoid Solenoid index 1–12.
 * @param on       true = on, false = off.
 * @return true on success; false if bank is invalid or solenoid out of range.
 */
bool solenoid_bank_set(solenoid_bank_handle_t bank, uint8_t solenoid, bool on);

/**
 * Turn all 12 solenoids off.
 * @param bank  Handle from solenoid_bank_init().
 * @return true on success.
 */
bool solenoid_bank_all_off(solenoid_bank_handle_t bank);

#ifdef __cplusplus
}
#endif

#endif /* SOLENOID_BANK_H */
