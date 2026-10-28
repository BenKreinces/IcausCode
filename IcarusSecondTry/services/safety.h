/** @file safety.h @brief Deterministic actuator safety-state policy. */
#ifndef ICARUS_SAFETY_H
#define ICARUS_SAFETY_H

#include <stdbool.h>
#include <stdint.h>

#include "command.h"

#define ICARUS_SAFETY_COMMAND_TIMEOUT_MS UINT32_C(500)

typedef enum
{
  ICARUS_SAFETY_BOOT = 0,
  ICARUS_SAFETY_SAFE = 1,
  ICARUS_SAFETY_ARMED = 2,
  ICARUS_SAFETY_FAULT = 3
} icarus_safety_state_t;

typedef enum
{
  ICARUS_FAULT_NONE = 0U,
  ICARUS_FAULT_COMMAND_TIMEOUT = 1U << 0,
  ICARUS_FAULT_SENSOR = 1U << 1,
  ICARUS_FAULT_ACTUATOR = 1U << 2,
  ICARUS_FAULT_NETWORK = 1U << 3,
  ICARUS_FAULT_TASK_LIVENESS = 1U << 4,
  ICARUS_FAULT_WATCHDOG = 1U << 5
} icarus_fault_mask_t;

typedef struct
{
  int32_t motor_rpm;
  uint32_t solenoid_open_mask;
} icarus_actuator_target_t;

typedef struct
{
  icarus_safety_state_t state;
  uint16_t fault_mask;
  uint32_t last_command_ms;
  uint32_t last_fault_ms;
  uint32_t last_command_id;
  bool interlocks_ready;
  bool command_seen;
  icarus_actuator_target_t target;
} icarus_safety_t;

void IcarusSafety_Init(icarus_safety_t *safety);
void IcarusSafety_SetInterlocksReady(icarus_safety_t *safety, bool ready);
void IcarusSafety_RaiseFault(icarus_safety_t *safety, uint16_t fault_mask,
                             uint32_t now_ms);
icarus_command_result_t IcarusSafety_ApplyCommand(icarus_safety_t *safety,
                                                    const icarus_command_t *command,
                                                    uint32_t now_ms);
void IcarusSafety_Update(icarus_safety_t *safety, uint32_t now_ms);
bool IcarusSafety_IsOutputEnabled(const icarus_safety_t *safety);

#endif /* ICARUS_SAFETY_H */
