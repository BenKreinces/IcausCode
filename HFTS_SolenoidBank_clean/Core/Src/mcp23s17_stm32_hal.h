#ifndef MCP23S17_STM32_HAL_H
#define MCP23S17_STM32_HAL_H

#include "mcp23s17.h"
#include "stm32h7xx_hal.h" /* for SPI_HandleTypeDef, GPIO_TypeDef, GPIO_PIN definitions */
#include <stdint.h>
#include <stdbool.h>

/*
 * STM32 HAL glue for mcp23s17 driver.
 *
 * This variant does NOT malloc. The caller provides a pointer to a per-chip
 * user struct (mcp23s17_stm32_user_t) which the HAL hooks will use. That
 * keeps allocation simple and deterministic for embedded usage.
 */

typedef struct {
    SPI_HandleTypeDef *hspi;   /* pointer to CubeMX SPI handle (e.g., &hspi1) */
    GPIO_TypeDef *cs_port;     /* CS GPIO port */
    uint16_t cs_pin;           /* CS GPIO pin number (GPIO_PIN_X) */
    uint32_t timeout_ms;       /* HAL timeout for blocking SPI transfers */
} mcp23s17_stm32_user_t;

/* Initialize a mcp23s17_hal_t for a particular SPI handle and CS pin.
 * - user_storage must point to a valid mcp23s17_stm32_user_t (caller-provided).
 * - out_hal is filled to be passed to mcp23s17_init().
 *
 * Returns true on success.
 */
bool mcp23s17_stm32_hal_init(SPI_HandleTypeDef *hspi,
                             GPIO_TypeDef *cs_port, uint16_t cs_pin,
                             uint32_t timeout_ms,
                             mcp23s17_stm32_user_t *user_storage,
                             mcp23s17_hal_t *out_hal);

/* No dynamic allocation used; nothing to free. */

#endif /* MCP23S17_STM32_HAL_H */
