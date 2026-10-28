#include "safety.h"

static void ForceSafeOutputs(icarus_safety_t *safety)
{
  safety->target.motor_rpm = 0;
  safety->target.solenoid_open_mask = 0U;
}

static bool IsSerialOlder(uint32_t candidate, uint32_t reference)
{
  return ((int32_t)(candidate - reference) < 0);
}

void IcarusSafety_Init(icarus_safety_t *safety)
{
  if (safety == NULL)
  {
    return;
  }
  safety->state = ICARUS_SAFETY_SAFE;
  safety->fault_mask = ICARUS_FAULT_NONE;
  safety->last_command_ms = 0U;
  safety->last_fault_ms = 0U;
  safety->last_command_id = 0U;
  safety->interlocks_ready = false;
  safety->command_seen = false;
  ForceSafeOutputs(safety);
}

void IcarusSafety_SetInterlocksReady(icarus_safety_t *safety, bool ready)
{
  if (safety != NULL)
  {
    safety->interlocks_ready = ready;
    if (!ready && (safety->state == ICARUS_SAFETY_ARMED))
    {
      IcarusSafety_RaiseFault(safety, ICARUS_FAULT_SENSOR, safety->last_command_ms);
    }
  }
}

void IcarusSafety_RaiseFault(icarus_safety_t *safety, uint16_t fault_mask,
                             uint32_t now_ms)
{
  if ((safety == NULL) || (fault_mask == ICARUS_FAULT_NONE))
  {
    return;
  }
  safety->fault_mask |= fault_mask;
  safety->last_fault_ms = now_ms;
  safety->state = ICARUS_SAFETY_FAULT;
  ForceSafeOutputs(safety);
}

icarus_command_result_t IcarusSafety_ApplyCommand(icarus_safety_t *safety,
                                                    const icarus_command_t *command,
                                                    uint32_t now_ms)
{
  if ((safety == NULL) || (command == NULL))
  {
    return ICARUS_COMMAND_REJECTED_MALFORMED;
  }
  if (safety->command_seen && (command->command_id == safety->last_command_id))
  {
    return ICARUS_COMMAND_REJECTED_DUPLICATE;
  }
  if (safety->command_seen &&
      IsSerialOlder(command->command_id, safety->last_command_id))
  {
    return ICARUS_COMMAND_REJECTED_STALE;
  }

  switch (command->opcode)
  {
    case ICARUS_COMMAND_SAFE:
      safety->state = ICARUS_SAFETY_SAFE;
      ForceSafeOutputs(safety);
      break;
    case ICARUS_COMMAND_CLEAR_FAULT:
      if (!safety->interlocks_ready)
      {
        return ICARUS_COMMAND_REJECTED_INTERLOCK;
      }
      safety->fault_mask = ICARUS_FAULT_NONE;
      safety->state = ICARUS_SAFETY_SAFE;
      ForceSafeOutputs(safety);
      break;
    case ICARUS_COMMAND_ARM:
      if (!safety->interlocks_ready)
      {
        return ICARUS_COMMAND_REJECTED_INTERLOCK;
      }
      if (safety->fault_mask != ICARUS_FAULT_NONE)
      {
        return ICARUS_COMMAND_REJECTED_STATE;
      }
      safety->state = ICARUS_SAFETY_ARMED;
      ForceSafeOutputs(safety);
      break;
    case ICARUS_COMMAND_SET_MOTOR_RPM:
      if (safety->state != ICARUS_SAFETY_ARMED)
      {
        return ICARUS_COMMAND_REJECTED_STATE;
      }
      safety->target.motor_rpm = command->motor_rpm;
      break;
    case ICARUS_COMMAND_SET_SOLENOID:
      if (safety->state != ICARUS_SAFETY_ARMED)
      {
        return ICARUS_COMMAND_REJECTED_STATE;
      }
      if (command->solenoid_open != 0U)
      {
        safety->target.solenoid_open_mask |= UINT32_C(1) << command->solenoid_index;
      }
      else
      {
        safety->target.solenoid_open_mask &= ~(UINT32_C(1) << command->solenoid_index);
      }
      break;
    default:
      return ICARUS_COMMAND_REJECTED_MALFORMED;
  }
  safety->last_command_id = command->command_id;
  safety->last_command_ms = now_ms;
  safety->command_seen = true;
  return ICARUS_COMMAND_ACCEPTED;
}

void IcarusSafety_Update(icarus_safety_t *safety, uint32_t now_ms)
{
  if ((safety == NULL) || (safety->state != ICARUS_SAFETY_ARMED))
  {
    return;
  }
  if (!safety->command_seen ||
      ((uint32_t)(now_ms - safety->last_command_ms) > ICARUS_SAFETY_COMMAND_TIMEOUT_MS))
  {
    IcarusSafety_RaiseFault(safety, ICARUS_FAULT_COMMAND_TIMEOUT, now_ms);
  }
}

bool IcarusSafety_IsOutputEnabled(const icarus_safety_t *safety)
{
  return (safety != NULL) && (safety->state == ICARUS_SAFETY_ARMED) &&
         (safety->fault_mask == ICARUS_FAULT_NONE) && safety->interlocks_ready;
}
