/** @file ksz8081_stm32_hal.h @brief STM32 HAL MDIO adapter for KSZ8081. */
#ifndef KSZ8081_STM32_HAL_H
#define KSZ8081_STM32_HAL_H

#include "ksz8081.h"
#include "main.h"

#if defined(HAL_ETH_MODULE_ENABLED)
typedef struct
{
  ETH_HandleTypeDef *eth;
} ksz8081_stm32_hal_t;

ksz8081_result_t KSZ8081_Stm32HalBind(ksz8081_t *device,
                                       ksz8081_stm32_hal_t *transport,
                                       ETH_HandleTypeDef *eth);
#endif

#endif /* KSZ8081_STM32_HAL_H */
