/** @file mcp23s17.h @brief Board-independent MCP23S17 solenoid driver. */
#ifndef ICARUS_MCP23S17_H
#define ICARUS_MCP23S17_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MCP23S17_SOLENOIDS_PER_BANK UINT8_C(12)

typedef bool (*mcp23s17_transfer_fn)(void *context, const uint8_t *tx,
                                      uint8_t *rx, size_t length);
typedef void (*mcp23s17_chip_select_fn)(void *context, bool asserted);

typedef enum
{
  MCP23S17_OK = 0,
  MCP23S17_ARGUMENT_ERROR,
  MCP23S17_BUS_ERROR,
  MCP23S17_CONFIGURATION_MISMATCH,
  MCP23S17_RANGE_ERROR
} mcp23s17_result_t;

typedef struct
{
  void *context;
  mcp23s17_transfer_fn transfer;
  mcp23s17_chip_select_fn set_chip_select;
} mcp23s17_bus_t;

typedef struct
{
  /* Inputs must be represented by 1 bits; all other pins are outputs. */
  uint8_t iodir_a;
  uint8_t iodir_b;
  /* Masks of fault inputs, returned without assuming an electrical polarity. */
  uint8_t fault_mask_a;
  uint8_t fault_mask_b;
} mcp23s17_config_t;

typedef struct
{
  mcp23s17_bus_t bus;
  mcp23s17_config_t config;
  uint8_t olat_a;
  uint8_t olat_b;
  uint32_t transaction_errors;
  uint32_t configuration_errors;
  bool initialized;
} mcp23s17_t;

typedef struct
{
  uint8_t port_a;
  uint8_t port_b;
} mcp23s17_fault_inputs_t;

mcp23s17_result_t MCP23S17_Init(mcp23s17_t *device, const mcp23s17_bus_t *bus,
                                 const mcp23s17_config_t *config);
mcp23s17_result_t MCP23S17_SetSolenoid(mcp23s17_t *device,
                                        uint8_t solenoid_index, bool open);
mcp23s17_result_t MCP23S17_SetAllSafe(mcp23s17_t *device);
mcp23s17_result_t MCP23S17_ReadFaultInputs(mcp23s17_t *device,
                                            mcp23s17_fault_inputs_t *inputs);

#endif /* ICARUS_MCP23S17_H */
