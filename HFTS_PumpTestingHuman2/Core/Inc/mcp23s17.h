/**
 * @file mcp23s17.h
 * @brief MCP23S17 GPIO expander over SPI. Single chip: solenoids 1-12.
 * Mapping: 1-12 -> B4,B5,B6,B7,A2,A1,A0,A3,B1,B0,A7,A6 (single 1-8, DSOL1=9,10, DSOL2=11,12).
 */
#ifndef MCP23S17_H
#define MCP23S17_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t (*mcp23s17_spi_txrx_fn)(void *user, uint8_t byte);
typedef void    (*mcp23s17_cs_fn)(void *user, bool level);

typedef struct {
    void *user;
    mcp23s17_spi_txrx_fn spi_txrx;
    mcp23s17_cs_fn cs;
} mcp23s17_hal_t;

typedef enum { MCP23S17_PORTA = 0, MCP23S17_PORTB = 1 } mcp23s17_port_t;

typedef struct {
    mcp23s17_hal_t hal;
    uint8_t olat_a;
    uint8_t olat_b;
    uint8_t iodir_a;
    uint8_t iodir_b;
} mcp23s17_t;

/** Solenoid 1-8 = single; 9,10 = DSOL1 (LOX); 11,12 = DSOL2 (LNG). */
typedef enum {
    MCP_SIG_SOL1, MCP_SIG_SOL2, MCP_SIG_SOL3, MCP_SIG_SOL4,
    MCP_SIG_SOL5, MCP_SIG_SOL6, MCP_SIG_SOL7, MCP_SIG_SOL8,
    MCP_SIG_SOL9, MCP_SIG_SOL10, MCP_SIG_SOL11, MCP_SIG_SOL12,
} mcp23s17_signal_t;

bool mcp23s17_init(mcp23s17_t *dev, const mcp23s17_hal_t *hal);
bool mcp23s17_write_pin(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t pin, bool level);
bool mcp23s17_read_pin(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t pin, bool *level_out);
bool mcp23s17_write_signal(mcp23s17_t *dev, mcp23s17_signal_t sig, bool level);
bool mcp23s17_read_signal(mcp23s17_t *dev, mcp23s17_signal_t sig, bool *level_out);

/** solenoidx_1based 1..12. */
bool mcp23s17_write_solenoid(mcp23s17_t *dev, uint8_t solenoidx_1based, bool level);
bool mcp23s17_read_solenoid(mcp23s17_t *dev, uint8_t solenoidx_1based, bool *level_out);

#endif /* MCP23S17_H */
