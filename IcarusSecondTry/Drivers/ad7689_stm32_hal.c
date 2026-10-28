#include "ad7689_stm32_hal.h"

static bool Transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
  ad7689_stm32_hal_t *transport = context;
  return (HAL_SPI_TransmitReceive(transport->spi, (uint8_t *)tx, rx, (uint16_t)length,
                                  transport->timeout_ms) == HAL_OK);
}
static void SetConvert(void *context, bool asserted)
{
  ad7689_stm32_hal_t *transport = context;
  HAL_GPIO_WritePin(transport->convert_port, transport->convert_pin,
                    asserted ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
static void DelayNs(void *context, uint32_t nanoseconds)
{
  uint32_t start;
  uint32_t cycles;
  (void)context;
  /* DWT is enabled by the board startup path before the pressure task runs. */
  cycles = (uint32_t)(((uint64_t)SystemCoreClock * nanoseconds + 999999999ULL) /
                      1000000000ULL);
  start = DWT->CYCCNT;
  while ((uint32_t)(DWT->CYCCNT - start) < cycles) { }
}
ad7689_result_t AD7689_Stm32HalBind(ad7689_t *device, ad7689_stm32_hal_t *transport,
                                     SPI_HandleTypeDef *spi, GPIO_TypeDef *convert_port,
                                     uint16_t convert_pin, uint32_t timeout_ms,
                                     const uint16_t config[AD7689_CHANNEL_COUNT],
                                     uint32_t conversion_time_ns)
{
  const ad7689_bus_t bus = {.context = transport, .transfer = Transfer,
                             .set_convert = SetConvert, .delay_ns = DelayNs};
  if ((device == NULL) || (transport == NULL) || (spi == NULL) ||
      (convert_port == NULL) || (timeout_ms == 0U)) return AD7689_ARGUMENT_ERROR;
  transport->spi = spi; transport->convert_port = convert_port;
  transport->convert_pin = convert_pin; transport->timeout_ms = timeout_ms;
  return AD7689_Init(device, &bus, config, conversion_time_ns);
}
