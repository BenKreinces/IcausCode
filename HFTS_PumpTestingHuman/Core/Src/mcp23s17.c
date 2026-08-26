/**
 * @file mcp23s17.c
 * @brief MCP23S17 driver. Pin map: 1-12 = B4,B5,B6,B7,A2,A1,A0,A3,B1,B0,A7,A6.
 */
#include "mcp23s17.h"
#include <stddef.h>

#define MCP23S17_IODIRA   0x00
#define MCP23S17_IODIRB   0x01
#define MCP23S17_GPIOA    0x12
#define MCP23S17_GPIOB    0x13
#define MCP23S17_OLATA    0x14
#define MCP23S17_OLATB    0x15
#define MCP23S17_IOCONA   0x0A
#define MCP23S17_IOCONB   0x0B
#define MCP23S17_CTRL_WRITE 0x40
#define MCP23S17_CTRL_READ  0x41
/* All outputs for solenoid control */
#define MCP23S17_IODIR_A_DEFAULT  0x00
#define MCP23S17_IODIR_B_DEFAULT  0x00

static int hal_ok(const mcp23s17_hal_t *hal) {
    return hal && hal->spi_txrx && hal->cs;
}
static void cs_low(mcp23s17_t *dev)  { dev->hal.cs(dev->hal.user, false); }
static void cs_high(mcp23s17_t *dev) { dev->hal.cs(dev->hal.user, true);  }
static uint8_t spi_txrx(mcp23s17_t *dev, uint8_t b) {
    return dev->hal.spi_txrx(dev->hal.user, b);
}

static bool write_reg(mcp23s17_t *dev, uint8_t reg, uint8_t val) {
    if (!dev) return false;
    cs_low(dev);
    (void)spi_txrx(dev, MCP23S17_CTRL_WRITE);
    (void)spi_txrx(dev, reg);
    (void)spi_txrx(dev, val);
    cs_high(dev);
    return true;
}
static bool read_reg(mcp23s17_t *dev, uint8_t reg, uint8_t *val_out) {
    if (!dev || !val_out) return false;
    cs_low(dev);
    (void)spi_txrx(dev, MCP23S17_CTRL_READ);
    (void)spi_txrx(dev, reg);
    *val_out = spi_txrx(dev, 0x00);
    cs_high(dev);
    return true;
}

/* 1-12: B4,B5,B6,B7,A2,A1,A0,A3,B1,B0,A7,A6 */
static bool get_signal_info(mcp23s17_signal_t sig,
                            mcp23s17_port_t *port, uint8_t *pin) {
    if (!port || !pin) return false;
    switch (sig) {
        case MCP_SIG_SOL1:  *port = MCP23S17_PORTB; *pin = 4; return true;
        case MCP_SIG_SOL2:  *port = MCP23S17_PORTB; *pin = 5; return true;
        case MCP_SIG_SOL3:  *port = MCP23S17_PORTB; *pin = 6; return true;
        case MCP_SIG_SOL4:  *port = MCP23S17_PORTB; *pin = 7; return true;
        case MCP_SIG_SOL5:  *port = MCP23S17_PORTA; *pin = 2; return true;
        case MCP_SIG_SOL6:  *port = MCP23S17_PORTA; *pin = 1; return true;
        case MCP_SIG_SOL7:  *port = MCP23S17_PORTA; *pin = 0; return true;
        case MCP_SIG_SOL8:  *port = MCP23S17_PORTA; *pin = 3; return true;
        case MCP_SIG_SOL9:  *port = MCP23S17_PORTB; *pin = 1; return true;
        case MCP_SIG_SOL10: *port = MCP23S17_PORTB; *pin = 0; return true;
        case MCP_SIG_SOL11: *port = MCP23S17_PORTA; *pin = 7; return true;
        case MCP_SIG_SOL12: *port = MCP23S17_PORTA; *pin = 6; return true;
        default: return false;
    }
}

bool mcp23s17_init(mcp23s17_t *dev, const mcp23s17_hal_t *hal) {
    if (!dev || !hal_ok(hal)) return false;
    dev->hal = *hal;
    cs_high(dev);
    for (volatile int i = 0; i < 1000; i++) { __asm volatile ("nop"); }
    cs_low(dev);
    for (volatile int i = 0; i < 1000; i++) { __asm volatile ("nop"); }
    cs_high(dev);
    for (volatile int i = 0; i < 1000; i++) { __asm volatile ("nop"); }
    if (!write_reg(dev, MCP23S17_IOCONA, 0x00)) return false;
    if (!write_reg(dev, MCP23S17_IOCONB, 0x00)) return false;
    dev->iodir_a = MCP23S17_IODIR_A_DEFAULT;
    dev->iodir_b = MCP23S17_IODIR_B_DEFAULT;
    if (!write_reg(dev, MCP23S17_IODIRA, dev->iodir_a)) return false;
    if (!write_reg(dev, MCP23S17_IODIRB, dev->iodir_b)) return false;
    dev->olat_a = 0x00;
    dev->olat_b = 0x00;
    if (!write_reg(dev, MCP23S17_OLATA, dev->olat_a)) return false;
    if (!write_reg(dev, MCP23S17_OLATB, dev->olat_b)) return false;
    return true;
}

bool mcp23s17_write_pin(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t pin, bool level) {
    if (!dev || pin > 7) return false;
    uint8_t mask = (uint8_t)(1u << pin);
    if (port == MCP23S17_PORTA) {
        if (level) dev->olat_a |= mask; else dev->olat_a &= (uint8_t)~mask;
        return write_reg(dev, MCP23S17_OLATA, dev->olat_a);
    } else {
        if (level) dev->olat_b |= mask; else dev->olat_b &= (uint8_t)~mask;
        return write_reg(dev, MCP23S17_OLATB, dev->olat_b);
    }
}

bool mcp23s17_read_pin(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t pin, bool *level_out) {
    if (!dev || !level_out || pin > 7) return false;
    uint8_t v = 0;
    bool ok = (port == MCP23S17_PORTA) ? read_reg(dev, MCP23S17_GPIOA, &v) : read_reg(dev, MCP23S17_GPIOB, &v);
    if (!ok) return false;
    *level_out = ((v >> pin) & 0x01u) ? true : false;
    return true;
}

bool mcp23s17_write_signal(mcp23s17_t *dev, mcp23s17_signal_t sig, bool level) {
    mcp23s17_port_t port;
    uint8_t pin;
    if (!get_signal_info(sig, &port, &pin)) return false;
    return mcp23s17_write_pin(dev, port, pin, level);
}

bool mcp23s17_read_signal(mcp23s17_t *dev, mcp23s17_signal_t sig, bool *level_out) {
    mcp23s17_port_t port;
    uint8_t pin;
    if (!get_signal_info(sig, &port, &pin)) return false;
    return mcp23s17_read_pin(dev, port, pin, level_out);
}

bool mcp23s17_write_solenoid(mcp23s17_t *dev, uint8_t solenoidx_1based, bool level) {
    if (!dev || solenoidx_1based < 1 || solenoidx_1based > 12) return false;
    mcp23s17_signal_t sig = (mcp23s17_signal_t)(MCP_SIG_SOL1 + (solenoidx_1based - 1));
    return mcp23s17_write_signal(dev, sig, level);
}

bool mcp23s17_read_solenoid(mcp23s17_t *dev, uint8_t solenoidx_1based, bool *level_out) {
    if (!dev || !level_out || solenoidx_1based < 1 || solenoidx_1based > 12) return false;
    mcp23s17_signal_t sig = (mcp23s17_signal_t)(MCP_SIG_SOL1 + (solenoidx_1based - 1));
    return mcp23s17_read_signal(dev, sig, level_out);
}
