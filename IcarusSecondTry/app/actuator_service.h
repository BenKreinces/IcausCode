/** @file actuator_service.h @brief Single-owner VESC and solenoid actuation. */
#ifndef ICARUS_ACTUATOR_SERVICE_H
#define ICARUS_ACTUATOR_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "mcp23s17.h"
#include "safety.h"
#include "vesc_uart.h"

typedef struct
{
  mcp23s17_t *solenoid_banks[2];
  vesc_uart_t *vesc;
  uint32_t apply_errors;
  bool initialized;
} icarus_actuator_service_t;

bool IcarusActuatorService_Init(icarus_actuator_service_t *service,
                                mcp23s17_t *bank_1_12,
                                mcp23s17_t *bank_13_24,
                                vesc_uart_t *vesc);
bool IcarusActuatorService_Apply(void *context,
                                 const icarus_actuator_target_t *target,
                                 bool outputs_enabled);

#endif /* ICARUS_ACTUATOR_SERVICE_H */
