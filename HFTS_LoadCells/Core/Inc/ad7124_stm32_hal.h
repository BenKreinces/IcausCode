/**
 * @file ad7124_stm32_hal.h
 * @brief STM32 HAL glue for AD7124 on SPI2 (CS on PD4 = SPI2_CS_LC).
 * Do not init SPI1 if using PB3 for SWO; SPI2 uses PB13/14/15.
 */

#ifndef AD7124_STM32_HAL_H
#define AD7124_STM32_HAL_H

#include "ad7124.h"
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint32_t timeout_ms;
} ad7124_stm32_user_t;

/** Fill dev with STM32 SPI2 + CS transport. user_storage holds hspi, cs_port, cs_pin, timeout. */
bool ad7124_stm32_hal_init(SPI_HandleTypeDef *hspi,
                            GPIO_TypeDef *cs_port, uint16_t cs_pin,
                            uint32_t timeout_ms,
                            ad7124_stm32_user_t *user_storage,
                            ad7124_dev_t *dev);

#endif /* AD7124_STM32_HAL_H */
