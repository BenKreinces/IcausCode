/** @file ad7124_stm32_hal.h @brief STM32 HAL transport adapter for ad7124.h. */
#ifndef AD7124_STM32_HAL_H
#define AD7124_STM32_HAL_H

#include "main.h"
#include "ad7124.h"

#if defined(HAL_SPI_MODULE_ENABLED)

typedef struct
{
  SPI_HandleTypeDef *spi;
  GPIO_TypeDef *chip_select_port;
  uint16_t chip_select_pin;
} ad7124_stm32_hal_t;

/** Bind an initialized SPI peripheral and a software-controlled chip select. */
ad7124_result_t AD7124_Stm32HalBind(ad7124_t *device,
                                     ad7124_stm32_hal_t *transport,
                                     SPI_HandleTypeDef *spi,
                                     GPIO_TypeDef *chip_select_port,
                                     uint16_t chip_select_pin,
                                     uint32_t transfer_timeout_ms);

#endif /* HAL_SPI_MODULE_ENABLED */

#endif /* AD7124_STM32_HAL_H */
