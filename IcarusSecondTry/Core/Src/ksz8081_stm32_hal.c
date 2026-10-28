/** @file ksz8081_stm32_hal.c @brief STM32 HAL MDIO adapter for KSZ8081. */
#include "ksz8081_stm32_hal.h"

#if defined(HAL_ETH_MODULE_ENABLED)
static bool KSZ8081_Stm32HalReadRegister(void *context,
                                          uint8_t phy_address,
                                          uint8_t register_address,
                                          uint16_t *value)
{
  ksz8081_stm32_hal_t *transport = (ksz8081_stm32_hal_t *)context;
  uint32_t read_value;

  if ((transport == NULL) || (transport->eth == NULL) || (value == NULL))
  {
    return false;
  }

  if (HAL_ETH_ReadPHYRegister(transport->eth, phy_address, register_address,
                              &read_value) != HAL_OK)
  {
    return false;
  }

  *value = (uint16_t)read_value;
  return true;
}

ksz8081_result_t KSZ8081_Stm32HalBind(ksz8081_t *device,
                                       ksz8081_stm32_hal_t *transport,
                                       ETH_HandleTypeDef *eth)
{
  ksz8081_bus_t bus;

  if ((transport == NULL) || (eth == NULL))
  {
    return KSZ8081_RESULT_ARGUMENT;
  }

  transport->eth = eth;
  HAL_ETH_SetMDIOClockRange(eth);
  bus.context = transport;
  bus.read_register = KSZ8081_Stm32HalReadRegister;
  return KSZ8081_Init(device, &bus);
}
#endif
