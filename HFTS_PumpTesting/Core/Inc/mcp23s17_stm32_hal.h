#ifndef MCP23S17_STM32_HAL_H
#define MCP23S17_STM32_HAL_H

#include "mcp23s17.h"
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint32_t timeout_ms;
} mcp23s17_stm32_user_t;

bool mcp23s17_stm32_hal_init(SPI_HandleTypeDef *hspi,
                             GPIO_TypeDef *cs_port, uint16_t cs_pin,
                             uint32_t timeout_ms,
                             mcp23s17_stm32_user_t *user_storage,
                             mcp23s17_hal_t *out_hal);

#endif /* MCP23S17_STM32_HAL_H */
