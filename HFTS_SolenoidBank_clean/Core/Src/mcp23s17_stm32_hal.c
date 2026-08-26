/*
 * mcp23s17_stm32_hal.c
 *
 *  Created on: Feb 8, 2026
 *      Author: alexi
 */


#include "mcp23s17_stm32_hal.h"
#include <string.h> /* for memset */

static uint8_t stm32_spi_txrx_impl(void *user, uint8_t byte) {
    mcp23s17_stm32_user_t *u = (mcp23s17_stm32_user_t *)user;
    if (!u || !u->hspi) return 0xFF;
    uint8_t rx = 0;
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(u->hspi, &byte, &rx, 1, u->timeout_ms);
    (void)st; /* optionally handle errors */
    return rx;
}

static void stm32_cs_impl(void *user, bool level) {
    mcp23s17_stm32_user_t *u = (mcp23s17_stm32_user_t *)user;
    if (!u) return;
    HAL_GPIO_WritePin(u->cs_port, u->cs_pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool mcp23s17_stm32_hal_init(SPI_HandleTypeDef *hspi,
                             GPIO_TypeDef *cs_port, uint16_t cs_pin,
                             uint32_t timeout_ms,
                             mcp23s17_stm32_user_t *user_storage,
                             mcp23s17_hal_t *out_hal)
{
    if (!hspi || !cs_port || !user_storage || !out_hal) return false;
    memset(user_storage, 0, sizeof(*user_storage));
    user_storage->hspi = hspi;
    user_storage->cs_port = cs_port;
    user_storage->cs_pin = cs_pin;
    user_storage->timeout_ms = timeout_ms ? timeout_ms : 10U;

    /* Ensure CS idle high */
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

    out_hal->user = user_storage;
    out_hal->spi_txrx = stm32_spi_txrx_impl;
    out_hal->cs = stm32_cs_impl;
    return true;
}
