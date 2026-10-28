#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mcp23s17.h"

typedef struct
{
  uint8_t registers[0x16];
  bool selected;
  bool fail;
} fake_mcp_t;

static void SetChipSelect(void *context, bool asserted)
{
  ((fake_mcp_t *)context)->selected = asserted;
}

static bool Transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
  fake_mcp_t *fake = context;
  assert(fake->selected);
  assert(length == 3U);
  if (fake->fail) return false;
  if (tx[0] == 0x40U)
  {
    fake->registers[tx[1]] = tx[2];
  }
  else if (tx[0] == 0x41U)
  {
    if (rx != NULL) rx[2] = fake->registers[tx[1]];
  }
  else
  {
    return false;
  }
  return true;
}

int main(void)
{
  fake_mcp_t fake;
  mcp23s17_t device;
  const mcp23s17_bus_t bus = {.context = &fake, .transfer = Transfer,
                               .set_chip_select = SetChipSelect};
  const mcp23s17_config_t config = {.iodir_a = 0x30U, .iodir_b = 0x0CU,
                                     .fault_mask_a = 0x30U,
                                     .fault_mask_b = 0x08U};
  mcp23s17_fault_inputs_t faults;

  memset(&fake, 0, sizeof(fake));
  assert(MCP23S17_Init(&device, &bus, &config) == MCP23S17_OK);
  assert(!fake.selected);
  assert(fake.registers[0] == 0x30U && fake.registers[1] == 0x0CU);
  assert(MCP23S17_SetSolenoid(&device, 0U, true) == MCP23S17_OK);
  assert((fake.registers[0x15] & 0x10U) != 0U);
  assert(MCP23S17_SetSolenoid(&device, 7U, true) == MCP23S17_OK);
  assert((fake.registers[0x14] & 0x08U) != 0U);
  assert(MCP23S17_SetSolenoid(&device, 12U, true) == MCP23S17_RANGE_ERROR);
  fake.registers[0x12] = 0xFFU;
  fake.registers[0x13] = 0xFFU;
  assert(MCP23S17_ReadFaultInputs(&device, &faults) == MCP23S17_OK);
  assert(faults.port_a == 0x30U && faults.port_b == 0x08U);
  assert(MCP23S17_SetAllSafe(&device) == MCP23S17_OK);
  assert(fake.registers[0x14] == 0U && fake.registers[0x15] == 0U);
  puts("MCP23S17 protocol tests passed");
  return 0;
}
