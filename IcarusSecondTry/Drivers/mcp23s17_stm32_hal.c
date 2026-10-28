#include "mcp23s17_stm32_hal.h"

static bool Transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
  mcp23s17_stm32_hal_t *transport = context;
  if ((transport == NULL) || (transport->spi == NULL) || (length > UINT16_MAX)) return false;
  return HAL_SPI_TransmitReceive(transport->spi, (uint8_t *)tx, rx, (uint16_t)length,
                                 transport->timeout_ms) == HAL_OK;
}

static void SetChipSelect(void *context, bool asserted)
{
  mcp23s17_stm32_hal_t *transport = context;
  HAL_GPIO_WritePin(transport->chip_select_port, transport->chip_select_pin,
                    asserted ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool MCP23S17_Stm32HalBind(mcp23s17_t *device, mcp23s17_stm32_hal_t *transport,
                            SPI_HandleTypeDef *spi, GPIO_TypeDef *chip_select_port,
                            uint16_t chip_select_pin, uint32_t timeout_ms,
                            const mcp23s17_config_t *config)
{
  const mcp23s17_bus_t bus = {.context = transport, .transfer = Transfer,
                               .set_chip_select = SetChipSelect};
  if ((device == NULL) || (transport == NULL) || (spi == NULL) ||
      (chip_select_port == NULL) || (config == NULL) || (timeout_ms == 0U)) return false;
  transport->spi = spi;
  transport->chip_select_port = chip_select_port;
  transport->chip_select_pin = chip_select_pin;
  transport->timeout_ms = timeout_ms;
  return MCP23S17_Init(device, &bus, config) == MCP23S17_OK;
}
