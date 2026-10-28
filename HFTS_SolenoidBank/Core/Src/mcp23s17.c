#include "mcp23s17.h"
#include <stddef.h> /* for NULL */

/* ===== MCP23S17 register map (BANK=0) ===== */
#define MCP23S17_IODIRA   0x00
#define MCP23S17_IODIRB   0x01
#define MCP23S17_GPPUA    0x0C
#define MCP23S17_GPPUB    0x0D
#define MCP23S17_IOCONA   0x0A
#define MCP23S17_IOCONB   0x0B
#define MCP23S17_GPIOA    0x12
#define MCP23S17_GPIOB    0x13
#define MCP23S17_OLATA    0x14
#define MCP23S17_OLATB    0x15

/* SPI control bytes (no address bits used) */
#define MCP23S17_CTRL_WRITE 0x40
#define MCP23S17_CTRL_READ  0x41

/* Default directions per your image (1=input, 0=output) */
#define MCP23S17_IODIR_A_DEFAULT  ((uint8_t)0x42) /* GPA1, GPA6 inputs */
#define MCP23S17_IODIR_B_DEFAULT  ((uint8_t)0x08) /* GPB3 input */

/* Optional: enable compile-time pullups on the fault pins:
 * set -DMCP23S17_ENABLE_PULLUPS=1 in your build if desired */
#ifndef MCP23S17_ENABLE_PULLUPS
#define MCP23S17_ENABLE_PULLUPS 0
#endif
#if MCP23S17_ENABLE_PULLUPS
#define MCP23S17_GPPU_A_DEFAULT ((uint8_t)0x42)
#define MCP23S17_GPPU_B_DEFAULT ((uint8_t)0x08)
#endif

/* ---------- helpers ---------- */
static bool hal_ok(const mcp23s17_hal_t *hal) {
    return hal && hal->spi_txrx && hal->cs;
}

static void cs_low(mcp23s17_t *dev)  { dev->hal.cs(dev->hal.user, false); }
static void cs_high(mcp23s17_t *dev) { dev->hal.cs(dev->hal.user, true);  }

static uint8_t spi_txrx(mcp23s17_t *dev, uint8_t b) {
    return dev->hal.spi_txrx(dev->hal.user, b);
}

/* Write one register (single-byte) */
static bool write_reg(mcp23s17_t *dev, uint8_t reg, uint8_t val) {
    if (!dev) return false;
    cs_low(dev);
    (void)spi_txrx(dev, MCP23S17_CTRL_WRITE);
    (void)spi_txrx(dev, reg);
    (void)spi_txrx(dev, val);
    cs_high(dev);
    return true;
}

/* Read one register (single-byte) */
static bool read_reg(mcp23s17_t *dev, uint8_t reg, uint8_t *val_out) {
    if (!dev || !val_out) return false;
    cs_low(dev);
    (void)spi_txrx(dev, MCP23S17_CTRL_READ);
    (void)spi_txrx(dev, reg);
    uint8_t v = spi_txrx(dev, 0x00);
    cs_high(dev);
    *val_out = v;
    return true;
}

/* Map a signal to (chip_index, port, pin). chip_index indicates which physical MCP chip the
 * signal belongs to when we use the two-chip helper. For single-chip-per-call functions we
 * ignore chip_index and expect the caller to supply the correct device instance. */
static bool get_signal_info(mcp23s17_signal_t sig,
                            uint8_t *chip_index,
                            mcp23s17_port_t *port,
                            uint8_t *pin)
{
    if (!chip_index || !port || !pin) return false;

    switch (sig) {
        /* chip 0: SOL1..SOL12 */
        case MCP_SIG_SOL1:  *chip_index = 0; *port = MCP23S17_PORTB; *pin = 2; return true; /* GPB2 */
        case MCP_SIG_SOL2:  *chip_index = 0; *port = MCP23S17_PORTB; *pin = 1; return true; /* GPB1 */
        case MCP_SIG_SOL3:  *chip_index = 0; *port = MCP23S17_PORTB; *pin = 0; return true; /* GPB0 */
        case MCP_SIG_SOL4:  *chip_index = 0; *port = MCP23S17_PORTA; *pin = 7; return true; /* GPA7 */
        case MCP_SIG_SOL5:  *chip_index = 0; *port = MCP23S17_PORTB; *pin = 7; return true; /* GPB7 */
        case MCP_SIG_SOL6:  *chip_index = 0; *port = MCP23S17_PORTB; *pin = 6; return true; /* GPB6 */
        case MCP_SIG_SOL7:  *chip_index = 0; *port = MCP23S17_PORTB; *pin = 5; return true; /* GPB5 */
        case MCP_SIG_SOL8:  *chip_index = 0; *port = MCP23S17_PORTB; *pin = 4; return true; /* GPB4 */
        case MCP_SIG_SOL9:  *chip_index = 0; *port = MCP23S17_PORTA; *pin = 5; return true; /* GPA5 */
        case MCP_SIG_SOL10: *chip_index = 0; *port = MCP23S17_PORTA; *pin = 4; return true; /* GPA4 */
        case MCP_SIG_SOL11: *chip_index = 0; *port = MCP23S17_PORTA; *pin = 3; return true; /* GPA3 */
        case MCP_SIG_SOL12: *chip_index = 0; *port = MCP23S17_PORTA; *pin = 2; return true; /* GPA2 */

        case MCP_SIG_DIAG_EN_SOL1_12: *chip_index = 0; *port = MCP23S17_PORTA; *pin = 0; return true; /* GPA0 */
        case MCP_SIG_FAULT_SOL1_4:    *chip_index = 0; *port = MCP23S17_PORTA; *pin = 6; return true; /* GPA6 */
        case MCP_SIG_FAULT_SOL5_8:    *chip_index = 0; *port = MCP23S17_PORTB; *pin = 3; return true; /* GPB3 */
        case MCP_SIG_FAULT_SOL9_12:   *chip_index = 0; *port = MCP23S17_PORTA; *pin = 1; return true; /* GPA1 */

        /* chip 1: SOL13..SOL24 mirror layout */
        case MCP_SIG_SOL13: *chip_index = 1; *port = MCP23S17_PORTB; *pin = 2; return true;
        case MCP_SIG_SOL14: *chip_index = 1; *port = MCP23S17_PORTB; *pin = 1; return true;
        case MCP_SIG_SOL15: *chip_index = 1; *port = MCP23S17_PORTB; *pin = 0; return true;
        case MCP_SIG_SOL16: *chip_index = 1; *port = MCP23S17_PORTA; *pin = 7; return true;
        case MCP_SIG_SOL17: *chip_index = 1; *port = MCP23S17_PORTB; *pin = 7; return true;
        case MCP_SIG_SOL18: *chip_index = 1; *port = MCP23S17_PORTB; *pin = 6; return true;
        case MCP_SIG_SOL19: *chip_index = 1; *port = MCP23S17_PORTB; *pin = 5; return true;
        case MCP_SIG_SOL20: *chip_index = 1; *port = MCP23S17_PORTB; *pin = 4; return true;
        case MCP_SIG_SOL21: *chip_index = 1; *port = MCP23S17_PORTA; *pin = 5; return true;
        case MCP_SIG_SOL22: *chip_index = 1; *port = MCP23S17_PORTA; *pin = 4; return true;
        case MCP_SIG_SOL23: *chip_index = 1; *port = MCP23S17_PORTA; *pin = 3; return true;
        case MCP_SIG_SOL24: *chip_index = 1; *port = MCP23S17_PORTA; *pin = 2; return true;

        case MCP_SIG_DIAG_EN_SOL13_24: *chip_index = 1; *port = MCP23S17_PORTA; *pin = 0; return true;
        case MCP_SIG_FAULT_SOL13_16:   *chip_index = 1; *port = MCP23S17_PORTA; *pin = 6; return true;
        case MCP_SIG_FAULT_SOL17_20:   *chip_index = 1; *port = MCP23S17_PORTB; *pin = 3; return true;
        case MCP_SIG_FAULT_SOL21_24:   *chip_index = 1; *port = MCP23S17_PORTA; *pin = 1; return true;

        default:
            return false;
    }
}

/* ---------- public API implementations ---------- */

bool mcp23s17_init(mcp23s17_t *dev, const mcp23s17_hal_t *hal) {
    if (!dev || !hal_ok(hal)) return false;

    dev->hal = *hal;

    /* ensure CS idle high */
    cs_high(dev);
    /* Datasheet note: if using SPI Mode 1,1, toggle CS once before first comm */
    cs_low(dev);
    for (volatile int i = 0; i < 1000; i++) { __asm__("nop"); }
    cs_high(dev);
    for (volatile int i = 0; i < 1000; i++) { __asm__("nop"); }

    /* IOCON: BANK=0, HAEN=0, SEQOP=0 -> write 0 to IOCON A/B */
    if (!write_reg(dev, MCP23S17_IOCONA, 0x00)) return false;
    if (!write_reg(dev, MCP23S17_IOCONB, 0x00)) return false;

    /* set directions */
    dev->iodir_a = MCP23S17_IODIR_A_DEFAULT;
    dev->iodir_b = MCP23S17_IODIR_B_DEFAULT;
    if (!write_reg(dev, MCP23S17_IODIRA, dev->iodir_a)) return false;
    if (!write_reg(dev, MCP23S17_IODIRB, dev->iodir_b)) return false;

#if MCP23S17_ENABLE_PULLUPS
    if (!write_reg(dev, MCP23S17_GPPUA, MCP23S17_GPPU_A_DEFAULT)) return false;
    if (!write_reg(dev, MCP23S17_GPPUB, MCP23S17_GPPU_B_DEFAULT)) return false;
#endif

    /* Initialize outputs low (OLAT) and shadow them */
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
        if (level) dev->olat_a |= mask;
        else       dev->olat_a &= (uint8_t)~mask;
        return write_reg(dev, MCP23S17_OLATA, dev->olat_a);
    } else {
        if (level) dev->olat_b |= mask;
        else       dev->olat_b &= (uint8_t)~mask;
        return write_reg(dev, MCP23S17_OLATB, dev->olat_b);
    }
}

bool mcp23s17_read_pin(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t pin, bool *level_out) {
    if (!dev || !level_out || pin > 7) return false;
    uint8_t v = 0;
    bool ok = (port == MCP23S17_PORTA) ? read_reg(dev, MCP23S17_GPIOA, &v)
                                       : read_reg(dev, MCP23S17_GPIOB, &v);
    if (!ok) return false;
    *level_out = ((v >> pin) & 0x01u) ? true : false;
    return true;
}

bool mcp23s17_write_port(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t value) {
    if (!dev) return false;
    if (port == MCP23S17_PORTA) {
        dev->olat_a = value;
        return write_reg(dev, MCP23S17_OLATA, dev->olat_a);
    } else {
        dev->olat_b = value;
        return write_reg(dev, MCP23S17_OLATB, dev->olat_b);
    }
}

bool mcp23s17_read_port(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t *value_out) {
    if (!dev || !value_out) return false;
    return (port == MCP23S17_PORTA) ? read_reg(dev, MCP23S17_GPIOA, value_out)
                                    : read_reg(dev, MCP23S17_GPIOB, value_out);
}

bool mcp23s17_write_signal(mcp23s17_t *dev, mcp23s17_signal_t sig, bool level) {
    uint8_t chip_index;
    mcp23s17_port_t port;
    uint8_t pin;
    if (!get_signal_info(sig, &chip_index, &port, &pin)) return false;
    /* This function is per-chip: caller must pass the device matching the chip_index. */
    return mcp23s17_write_pin(dev, port, pin, level);
}

bool mcp23s17_read_signal(mcp23s17_t *dev, mcp23s17_signal_t sig, bool *level_out) {
    uint8_t chip_index;
    mcp23s17_port_t port;
    uint8_t pin;
    if (!get_signal_info(sig, &chip_index, &port, &pin)) return false;
    return mcp23s17_read_pin(dev, port, pin, level_out);
}

/* Map solenoid index (1..24) to chip index (0 or 1) and signal enum */
bool mcp23s17_map_solenoid_to_signal(uint8_t solenoidx_1based,
                                     uint8_t *chip_index_out,
                                     mcp23s17_signal_t *sig_out)
{
    if (!chip_index_out || !sig_out) return false;
    if (solenoidx_1based < 1 || solenoidx_1based > 24) return false;
    if (solenoidx_1based <= 12) {
        *chip_index_out = 0;
        *sig_out = (mcp23s17_signal_t)(MCP_SIG_SOL1 + (solenoidx_1based - 1));
    } else {
        *chip_index_out = 1;
        *sig_out = (mcp23s17_signal_t)(MCP_SIG_SOL13 + (solenoidx_1based - 13));
    }
    return true;
}

/* Convenience: write solenoid by number across two devices */
bool mcp23s17_write_solenoid(mcp23s17_t *devs[2], uint8_t solenoidx_1based, bool level) {
    uint8_t chip;
    mcp23s17_signal_t sig;
    if (!mcp23s17_map_solenoid_to_signal(solenoidx_1based, &chip, &sig)) return false;
    if (!devs[chip]) return false;
    return mcp23s17_write_signal(devs[chip], sig, level);
}

bool mcp23s17_read_solenoid(mcp23s17_t *devs[2], uint8_t solenoidx_1based, bool *level_out) {
    uint8_t chip;
    mcp23s17_signal_t sig;
    if (!mcp23s17_map_solenoid_to_signal(solenoidx_1based, &chip, &sig)) return false;
    if (!devs[chip]) return false;
    return mcp23s17_read_signal(devs[chip], sig, level_out);
}

/* Read fault groups for a single chip (dev must be the correct chip instance) */
bool mcp23s17_read_fault_groups(mcp23s17_t *dev,
                               bool *fault_a_b, bool *fault_c_d, bool *fault_e_f)
{
    if (!dev) return false;
    bool tmp;
    if (fault_a_b) {
        /* FAULT_SOL1_4 or FAULT_SOL13_16 => GPA6 */
        if (!mcp23s17_read_pin(dev, MCP23S17_PORTA, 6, &tmp)) return false;
        *fault_a_b = tmp;
    }
    if (fault_c_d) {
        /* FAULT_SOL5_8 or FAULT_SOL17_20 => GPB3 */
        if (!mcp23s17_read_pin(dev, MCP23S17_PORTB, 3, &tmp)) return false;
        *fault_c_d = tmp;
    }
    if (fault_e_f) {
        /* FAULT_SOL9_12 or FAULT_SOL21_24 => GPA1 */
        if (!mcp23s17_read_pin(dev, MCP23S17_PORTA, 1, &tmp)) return false;
        *fault_e_f = tmp;
    }
    return true;
}
