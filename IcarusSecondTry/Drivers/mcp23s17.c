#include "mcp23s17.h"

#define MCP23S17_OPCODE_WRITE UINT8_C(0x40)
#define MCP23S17_OPCODE_READ UINT8_C(0x41)
#define MCP23S17_IODIRA UINT8_C(0x00)
#define MCP23S17_IODIRB UINT8_C(0x01)
#define MCP23S17_IOCON UINT8_C(0x0A)
#define MCP23S17_GPIOA UINT8_C(0x12)
#define MCP23S17_GPIOB UINT8_C(0x13)
#define MCP23S17_OLATA UINT8_C(0x14)
#define MCP23S17_OLATB UINT8_C(0x15)

/* Board map recovered from the solenoid bank reference.  Index zero is SOL1. */
static const uint8_t solenoid_port[12] = {1U, 1U, 1U, 1U, 0U, 0U,
                                           0U, 0U, 0U, 0U, 1U, 1U};
static const uint8_t solenoid_pin[12] = {4U, 5U, 6U, 7U, 2U, 1U,
                                          0U, 3U, 6U, 7U, 0U, 1U};

static bool HasBus(const mcp23s17_t *device)
{
  return (device != NULL) && (device->bus.transfer != NULL) &&
         (device->bus.set_chip_select != NULL);
}

static mcp23s17_result_t Transfer(mcp23s17_t *device, const uint8_t *tx,
                                  uint8_t *rx, size_t length)
{
  bool result;
  device->bus.set_chip_select(device->bus.context, true);
  result = device->bus.transfer(device->bus.context, tx, rx, length);
  device->bus.set_chip_select(device->bus.context, false);
  if (!result)
  {
    ++device->transaction_errors;
    return MCP23S17_BUS_ERROR;
  }
  return MCP23S17_OK;
}

static mcp23s17_result_t WriteRegister(mcp23s17_t *device, uint8_t address,
                                        uint8_t value)
{
  const uint8_t tx[3] = {MCP23S17_OPCODE_WRITE, address, value};
  return Transfer(device, tx, NULL, sizeof(tx));
}

static mcp23s17_result_t ReadRegister(mcp23s17_t *device, uint8_t address,
                                       uint8_t *value)
{
  const uint8_t tx[3] = {MCP23S17_OPCODE_READ, address, 0U};
  uint8_t rx[3] = {0U, 0U, 0U};
  mcp23s17_result_t result;

  if (value == NULL)
  {
    return MCP23S17_ARGUMENT_ERROR;
  }
  result = Transfer(device, tx, rx, sizeof(tx));
  if (result == MCP23S17_OK)
  {
    *value = rx[2];
  }
  return result;
}

static mcp23s17_result_t CheckRegister(mcp23s17_t *device, uint8_t address,
                                        uint8_t expected)
{
  uint8_t observed;
  mcp23s17_result_t result = ReadRegister(device, address, &observed);
  if ((result == MCP23S17_OK) && (observed != expected))
  {
    ++device->configuration_errors;
    return MCP23S17_CONFIGURATION_MISMATCH;
  }
  return result;
}

static mcp23s17_result_t WriteOutputs(mcp23s17_t *device)
{
  mcp23s17_result_t result = WriteRegister(device, MCP23S17_OLATA,
                                            device->olat_a);
  if (result == MCP23S17_OK)
  {
    result = WriteRegister(device, MCP23S17_OLATB, device->olat_b);
  }
  return result;
}

mcp23s17_result_t MCP23S17_Init(mcp23s17_t *device, const mcp23s17_bus_t *bus,
                                 const mcp23s17_config_t *config)
{
  mcp23s17_result_t result;

  if ((device == NULL) || (bus == NULL) || (config == NULL) ||
      (bus->transfer == NULL) || (bus->set_chip_select == NULL))
  {
    return MCP23S17_ARGUMENT_ERROR;
  }
  device->bus = *bus;
  device->config = *config;
  device->olat_a = 0U;
  device->olat_b = 0U;
  device->transaction_errors = 0U;
  device->configuration_errors = 0U;
  device->initialized = false;
  device->bus.set_chip_select(device->bus.context, false);

  /* BANK=0 and sequential addressing make the register map explicit. */
  result = WriteRegister(device, MCP23S17_IOCON, 0U);
  if (result != MCP23S17_OK) return result;
  result = WriteRegister(device, MCP23S17_IODIRA, config->iodir_a);
  if (result != MCP23S17_OK) return result;
  result = WriteRegister(device, MCP23S17_IODIRB, config->iodir_b);
  if (result != MCP23S17_OK) return result;
  result = WriteOutputs(device);
  if (result != MCP23S17_OK) return result;
  result = CheckRegister(device, MCP23S17_IODIRA, config->iodir_a);
  if (result != MCP23S17_OK) return result;
  result = CheckRegister(device, MCP23S17_IODIRB, config->iodir_b);
  if (result != MCP23S17_OK) return result;
  result = CheckRegister(device, MCP23S17_OLATA, 0U);
  if (result != MCP23S17_OK) return result;
  result = CheckRegister(device, MCP23S17_OLATB, 0U);
  if (result != MCP23S17_OK) return result;
  device->initialized = true;
  return MCP23S17_OK;
}

mcp23s17_result_t MCP23S17_SetSolenoid(mcp23s17_t *device,
                                        uint8_t solenoid_index, bool open)
{
  uint8_t mask;

  if (!HasBus(device) || !device->initialized)
  {
    return MCP23S17_ARGUMENT_ERROR;
  }
  if (solenoid_index >= MCP23S17_SOLENOIDS_PER_BANK)
  {
    return MCP23S17_RANGE_ERROR;
  }
  mask = (uint8_t)(UINT8_C(1) << solenoid_pin[solenoid_index]);
  if (solenoid_port[solenoid_index] == 0U)
  {
    device->olat_a = open ? (uint8_t)(device->olat_a | mask)
                          : (uint8_t)(device->olat_a & (uint8_t)~mask);
  }
  else
  {
    device->olat_b = open ? (uint8_t)(device->olat_b | mask)
                          : (uint8_t)(device->olat_b & (uint8_t)~mask);
  }
  return WriteOutputs(device);
}

mcp23s17_result_t MCP23S17_SetAllSafe(mcp23s17_t *device)
{
  if (!HasBus(device) || !device->initialized)
  {
    return MCP23S17_ARGUMENT_ERROR;
  }
  device->olat_a = 0U;
  device->olat_b = 0U;
  return WriteOutputs(device);
}

mcp23s17_result_t MCP23S17_ReadFaultInputs(mcp23s17_t *device,
                                            mcp23s17_fault_inputs_t *inputs)
{
  mcp23s17_result_t result;
  if (!HasBus(device) || !device->initialized || (inputs == NULL))
  {
    return MCP23S17_ARGUMENT_ERROR;
  }
  result = ReadRegister(device, MCP23S17_GPIOA, &inputs->port_a);
  if (result == MCP23S17_OK)
  {
    result = ReadRegister(device, MCP23S17_GPIOB, &inputs->port_b);
  }
  if (result == MCP23S17_OK)
  {
    inputs->port_a &= device->config.fault_mask_a;
    inputs->port_b &= device->config.fault_mask_b;
  }
  return result;
}
