/** @file mcp23s17_stm32_hal.h @brief HAL transport binding for MCP23S17. */
#ifndef ICARUS_MCP23S17_STM32_HAL_H
#define ICARUS_MCP23S17_STM32_HAL_H

#include "main.h"
#include "mcp23s17.h"

typedef struct
{
  SPI_HandleTypeDef *spi;
  GPIO_TypeDef *chip_select_port;
  uint16_t chip_select_pin;
  uint32_t timeout_ms;
} mcp23s17_stm32_hal_t;

bool MCP23S17_Stm32HalBind(mcp23s17_t *device, mcp23s17_stm32_hal_t *transport,
                            SPI_HandleTypeDef *spi, GPIO_TypeDef *chip_select_port,
                            uint16_t chip_select_pin, uint32_t timeout_ms,
                            const mcp23s17_config_t *config);

#endif
