/** @file ad7689_stm32_hal.h @brief HAL transport binding for AD7689. */
#ifndef ICARUS_AD7689_STM32_HAL_H
#define ICARUS_AD7689_STM32_HAL_H

#include "ad7689.h"
#include "main.h"

typedef struct
{
  SPI_HandleTypeDef *spi;
  GPIO_TypeDef *convert_port;
  uint16_t convert_pin;
  uint32_t timeout_ms;
} ad7689_stm32_hal_t;

ad7689_result_t AD7689_Stm32HalBind(ad7689_t *device, ad7689_stm32_hal_t *transport,
                                     SPI_HandleTypeDef *spi, GPIO_TypeDef *convert_port,
                                     uint16_t convert_pin, uint32_t timeout_ms,
                                     const uint16_t config[AD7689_CHANNEL_COUNT],
                                     uint32_t conversion_time_ns);

#endif
