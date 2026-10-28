#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "actuator_service.h"

typedef struct { uint8_t regs[0x16]; bool selected; } fake_mcp_t;
typedef struct { unsigned sends; } fake_uart_t;

static void Cs(void *context, bool asserted) { ((fake_mcp_t *)context)->selected = asserted; }
static bool Spi(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
  fake_mcp_t *fake = context;
  if (!fake->selected || length != 3U) return false;
  if (tx[0] == 0x40U) fake->regs[tx[1]] = tx[2];
  else if (tx[0] == 0x41U && rx != NULL) rx[2] = fake->regs[tx[1]];
  else return false;
  return true;
}
static bool Send(void *context, const uint8_t *data, uint16_t length)
{ (void)data; (void)length; ++((fake_uart_t *)context)->sends; return true; }

int main(void)
{
  fake_mcp_t first = {0}, second = {0};
  fake_uart_t uart = {0};
  mcp23s17_t bank_1, bank_2;
  vesc_uart_t vesc;
  icarus_actuator_service_t service;
  const mcp23s17_config_t config = {0x30U, 0x0CU, 0x30U, 0x08U};
  const mcp23s17_bus_t bus_1 = {&first, Spi, Cs};
  const mcp23s17_bus_t bus_2 = {&second, Spi, Cs};
  const icarus_actuator_target_t target = {1000, UINT32_C(1) | (UINT32_C(1) << 12U)};
  assert(MCP23S17_Init(&bank_1, &bus_1, &config) == MCP23S17_OK);
  assert(MCP23S17_Init(&bank_2, &bus_2, &config) == MCP23S17_OK);
  VescUart_Init(&vesc, &uart, Send);
  assert(IcarusActuatorService_Init(&service, &bank_1, &bank_2, &vesc));
  assert(IcarusActuatorService_Apply(&service, &target, true));
  assert((first.regs[0x15] & 0x10U) != 0U);
  assert((second.regs[0x15] & 0x10U) != 0U);
  assert(IcarusActuatorService_Apply(&service, &target, false));
  assert(first.regs[0x14] == 0U && first.regs[0x15] == 0U);
  assert(second.regs[0x14] == 0U && second.regs[0x15] == 0U);
  puts("Actuator service tests passed");
  return 0;
}
