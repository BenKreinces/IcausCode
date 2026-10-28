/** @file ad7124_stm32_hal.c @brief STM32 HAL transport adapter for AD7124. */
#include "ad7124_stm32_hal.h"

#if defined(HAL_SPI_MODULE_ENABLED)

static void AD7124_Stm32HalSetChipSelect(void *context, bool asserted)
{
  ad7124_stm32_hal_t *transport = (ad7124_stm32_hal_t *)context;
  HAL_GPIO_WritePin(transport->chip_select_port,
                    transport->chip_select_pin,
                    asserted ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static bool AD7124_Stm32HalTransfer(void *context,
                                    const uint8_t *tx,
                                    uint8_t *rx,
                                    size_t length,
                                    uint32_t timeout_ms)
{
  ad7124_stm32_hal_t *transport = (ad7124_stm32_hal_t *)context;
  uint8_t discard[4];
  const uint8_t *transmit_data = tx;

  if ((transport == NULL) || (transport->spi == NULL) ||
      (tx == NULL) || (length == 0U) || (length > sizeof(discard)))
  {
    return false;
  }

  if (rx == NULL)
  {
    rx = discard;
  }

  return HAL_SPI_TransmitReceive(transport->spi,
                                 (uint8_t *)transmit_data,
                                 rx,
                                 (uint16_t)length,
                                 timeout_ms) == HAL_OK;
}

static void AD7124_Stm32HalDelay(void *context, uint32_t delay_ms)
{
  (void)context;
  HAL_Delay(delay_ms);
}

ad7124_result_t AD7124_Stm32HalBind(ad7124_t *device,
                                     ad7124_stm32_hal_t *transport,
                                     SPI_HandleTypeDef *spi,
                                     GPIO_TypeDef *chip_select_port,
                                     uint16_t chip_select_pin,
                                     uint32_t transfer_timeout_ms)
{
  ad7124_bus_t bus;

  if ((transport == NULL) || (spi == NULL) || (chip_select_port == NULL) ||
      (chip_select_pin == 0U))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  transport->spi = spi;
  transport->chip_select_port = chip_select_port;
  transport->chip_select_pin = chip_select_pin;
  bus.context = transport;
  bus.set_chip_select = AD7124_Stm32HalSetChipSelect;
  bus.transfer = AD7124_Stm32HalTransfer;
  bus.delay_ms = AD7124_Stm32HalDelay;

  return AD7124_Init(device, &bus, transfer_timeout_ms);
}

#endif /* HAL_SPI_MODULE_ENABLED */
