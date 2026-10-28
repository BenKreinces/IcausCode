/**
 * @file solenoid_bank.c
 * @brief Solenoid bank: one MCP23S17, 12 outputs. Pin map 1–12:
 *       B4, B5, B6, B7, A2, A1, A0, A3, A6, A7, B0, B1.
 */

#include "solenoid_bank.h"
#include "mcp23s17.h"
#include "mcp23s17_stm32_hal.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/* Single bank instance (no malloc). */
static struct solenoid_bank_impl {
    mcp23s17_t              mcp;
    mcp23s17_stm32_user_t   hal_user;
    mcp23s17_hal_t          hal;
    bool                    initialized;
} s_bank;

/* Solenoid 1–12 → (port, pin). Order: B4,B5,B6,B7, A2,A1,A0,A3, A6,A7, B0,B1. */
#define SOL_COUNT 12
static const struct { uint8_t port; uint8_t pin; } s_sol_map[SOL_COUNT] = {
    { 1, 4 },  /*  1 = B4 */
    { 1, 5 },  /*  2 = B5 */
    { 1, 6 },  /*  3 = B6 */
    { 1, 7 },  /*  4 = B7 */
    { 0, 2 },  /*  5 = A2 */
    { 0, 1 },  /*  6 = A1 */
    { 0, 0 },  /*  7 = A0 */
    { 0, 3 },  /*  8 = A3 */
    { 0, 6 },  /*  9 = A6 */
    { 0, 7 },  /* 10 = A7 */
    { 1, 0 },  /* 11 = B0 */
    { 1, 1 },  /* 12 = B1 */
};
/* port: 0 = PORTA, 1 = PORTB */

/* IODIR: 1=input, 0=output. Solenoid pins = output; A4,A5 and B2,B3 = input (fault/DIAGEN). */
#define IODIR_A_SOL_BANK  ((uint8_t)0x30)  /* A0,A1,A2,A3,A6,A7 out; A4,A5 in */
#define IODIR_B_SOL_BANK  ((uint8_t)0x0C)  /* B0,B1,B4,B5,B6,B7 out; B2,B3 in */

solenoid_bank_handle_t solenoid_bank_init(void *hspi, void *cs_port, uint16_t cs_pin, uint32_t timeout_ms) {
    SPI_HandleTypeDef *spi = (SPI_HandleTypeDef *)hspi;
    GPIO_TypeDef      *cs  = (GPIO_TypeDef *)cs_port;
    if (!spi || !cs) return NULL;

    memset(&s_bank, 0, sizeof(s_bank));
    if (!mcp23s17_stm32_hal_init(spi, cs, cs_pin, timeout_ms ? timeout_ms : 10U,
                                  &s_bank.hal_user, &s_bank.hal)) {
        return NULL;
    }
    if (!mcp23s17_init(&s_bank.mcp, &s_bank.hal)) {
        return NULL;
    }
    if (!mcp23s17_set_iodir(&s_bank.mcp, IODIR_A_SOL_BANK, IODIR_B_SOL_BANK)) {
        return NULL;
    }
    /* All outputs 0 */
    if (!mcp23s17_write_port(&s_bank.mcp, MCP23S17_PORTA, 0x00)) return NULL;
    if (!mcp23s17_write_port(&s_bank.mcp, MCP23S17_PORTB, 0x00)) return NULL;
    s_bank.initialized = true;
    return (solenoid_bank_handle_t)&s_bank;
}

bool solenoid_bank_set(solenoid_bank_handle_t bank, uint8_t solenoid, bool on) {
    if (!bank || solenoid < 1 || solenoid > SOL_COUNT) return false;
    struct solenoid_bank_impl *b = (struct solenoid_bank_impl *)bank;
    if (!b->initialized) return false;

    uint8_t idx = (uint8_t)(solenoid - 1);
    mcp23s17_port_t port = s_sol_map[idx].port ? MCP23S17_PORTB : MCP23S17_PORTA;
    uint8_t pin = s_sol_map[idx].pin;
    return mcp23s17_write_pin(&b->mcp, port, pin, on);
}

bool solenoid_bank_all_off(solenoid_bank_handle_t bank) {
    if (!bank) return false;
    struct solenoid_bank_impl *b = (struct solenoid_bank_impl *)bank;
    if (!b->initialized) return false;
    if (!mcp23s17_write_port(&b->mcp, MCP23S17_PORTA, 0x00)) return false;
    if (!mcp23s17_write_port(&b->mcp, MCP23S17_PORTB, 0x00)) return false;
    return true;
}
