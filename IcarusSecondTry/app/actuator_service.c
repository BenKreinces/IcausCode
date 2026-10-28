#include "actuator_service.h"

#include <string.h>

static bool ForceSafe(icarus_actuator_service_t *service)
{
  bool success = true;
  uint8_t index;
  if (service->vesc != NULL)
  {
    success = (VescUart_Stop(service->vesc) == VESC_UART_OK) && success;
  }
  for (index = 0U; index < 2U; ++index)
  {
    if (service->solenoid_banks[index] != NULL)
    {
      success = (MCP23S17_SetAllSafe(service->solenoid_banks[index]) == MCP23S17_OK) &&
                success;
    }
  }
  return success;
}

bool IcarusActuatorService_Init(icarus_actuator_service_t *service,
                                mcp23s17_t *bank_1_12,
                                mcp23s17_t *bank_13_24,
                                vesc_uart_t *vesc)
{
  if ((service == NULL) || (bank_1_12 == NULL) || (bank_13_24 == NULL) ||
      (vesc == NULL)) return false;
  memset(service, 0, sizeof(*service));
  service->solenoid_banks[0] = bank_1_12;
  service->solenoid_banks[1] = bank_13_24;
  service->vesc = vesc;
  service->initialized = ForceSafe(service);
  return service->initialized;
}

bool IcarusActuatorService_Apply(void *context,
                                 const icarus_actuator_target_t *target,
                                 bool outputs_enabled)
{
  icarus_actuator_service_t *service = context;
  uint8_t solenoid;
  bool success = true;
  if ((service == NULL) || !service->initialized || (target == NULL)) return false;
  if (!outputs_enabled) success = ForceSafe(service);
  else
  {
    /* Close every output before applying a new complete mask, then command
     * motor RPM only after solenoid programming succeeds. */
    success = ForceSafe(service);
    for (solenoid = 0U; success && (solenoid < 24U); ++solenoid)
    {
      const uint8_t bank = solenoid / 12U;
      const bool open = (target->solenoid_open_mask & (UINT32_C(1) << solenoid)) != 0U;
      success = MCP23S17_SetSolenoid(service->solenoid_banks[bank],
                                     solenoid % 12U, open) == MCP23S17_OK;
    }
    if (success)
    {
      success = VescUart_SetRpm(service->vesc, target->motor_rpm) == VESC_UART_OK;
    }
    if (!success) (void)ForceSafe(service);
  }
  if (!success) ++service->apply_errors;
  return success;
}
