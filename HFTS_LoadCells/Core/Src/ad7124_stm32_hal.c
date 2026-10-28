/**
 * @file ad7124_stm32_hal.c
 * @brief STM32 HAL transport for AD7124: CS + SPI2.
 */

#include "ad7124_stm32_hal.h"
#include <string.h>

static bool ad7124_stm32_xfer(void *user, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    ad7124_stm32_user_t *u = (ad7124_stm32_user_t *)user;
    if (!u || !u->hspi || !tx || !rx || len == 0) return false;

    HAL_GPIO_WritePin(u->cs_port, u->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(u->hspi, (uint8_t *)tx, rx, len, u->timeout_ms);
    HAL_GPIO_WritePin(u->cs_port, u->cs_pin, GPIO_PIN_SET);

    return (s == HAL_OK);
}

bool ad7124_stm32_hal_init(SPI_HandleTypeDef *hspi,
                           GPIO_TypeDef *cs_port, uint16_t cs_pin,
                           uint32_t timeout_ms,
                           ad7124_stm32_user_t *user_storage,
                           ad7124_dev_t *dev)
{
    if (!hspi || !cs_port || !user_storage || !dev) return false;
    memset(user_storage, 0, sizeof(*user_storage));
    user_storage->hspi = hspi;
    user_storage->cs_port = cs_port;
    user_storage->cs_pin = cs_pin;
    user_storage->timeout_ms = (timeout_ms != 0) ? timeout_ms : 50;

    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

    dev->user = user_storage;
    dev->spi_xfer = ad7124_stm32_xfer;
    return true;
}
