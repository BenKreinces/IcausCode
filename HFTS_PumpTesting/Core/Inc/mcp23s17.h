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

typedef enum {
    MCP_SIG_SOL1 = 0, MCP_SIG_SOL2, MCP_SIG_SOL3, MCP_SIG_SOL4,
    MCP_SIG_SOL5, MCP_SIG_SOL6, MCP_SIG_SOL7, MCP_SIG_SOL8,
    MCP_SIG_SOL9, MCP_SIG_SOL10, MCP_SIG_SOL11, MCP_SIG_SOL12,
    MCP_SIG_SOL13, MCP_SIG_SOL14, MCP_SIG_SOL15, MCP_SIG_SOL16,
    MCP_SIG_SOL17, MCP_SIG_SOL18, MCP_SIG_SOL19, MCP_SIG_SOL20,
    MCP_SIG_SOL21, MCP_SIG_SOL22, MCP_SIG_SOL23, MCP_SIG_SOL24,
    MCP_SIG_DIAG_EN_SOL1_12, MCP_SIG_DIAG_EN_SOL13_24,
    MCP_SIG_FAULT_SOL1_4, MCP_SIG_FAULT_SOL5_8, MCP_SIG_FAULT_SOL9_12,
    MCP_SIG_FAULT_SOL13_16, MCP_SIG_FAULT_SOL17_20, MCP_SIG_FAULT_SOL21_24,
} mcp23s17_signal_t;

bool mcp23s17_init(mcp23s17_t *dev, const mcp23s17_hal_t *hal);
bool mcp23s17_write_pin(mcp23s17_t *dev, mcp23s17_port_t port, uint8_t pin, bool level);
bool mcp23s17_read_pin (mcp23s17_t *dev, mcp23s17_port_t port, uint8_t pin, bool *level_out);
bool mcp23s17_write_signal(mcp23s17_t *dev, mcp23s17_signal_t sig, bool level);
bool mcp23s17_read_signal (mcp23s17_t *dev, mcp23s17_signal_t sig, bool *level_out);
bool mcp23s17_map_solenoid_to_signal(uint8_t solenoidx_1based,
                                     uint8_t *chip_index_out,
                                     mcp23s17_signal_t *sig_out);
bool mcp23s17_write_solenoid(mcp23s17_t *devs[2], uint8_t solenoidx_1based, bool level);
bool mcp23s17_read_solenoid (mcp23s17_t *devs[2], uint8_t solenoidx_1based, bool *level_out);

#endif /* MCP23S17_H */
