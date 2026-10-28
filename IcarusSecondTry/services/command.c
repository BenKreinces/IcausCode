#include "command.h"

static void PutBe32(uint8_t *output, uint32_t value)
{
  output[0] = (uint8_t)(value >> 24U);
  output[1] = (uint8_t)(value >> 16U);
  output[2] = (uint8_t)(value >> 8U);
  output[3] = (uint8_t)value;
}

static uint32_t GetBe32(const uint8_t *input)
{
  return ((uint32_t)input[0] << 24U) | ((uint32_t)input[1] << 16U) |
         ((uint32_t)input[2] << 8U) | input[3];
}

icarus_command_result_t IcarusCommand_Decode(const uint8_t *payload,
                                              size_t payload_length,
                                              icarus_command_t *command)
{
  uint8_t argument_length;

  if ((payload == NULL) || (command == NULL) || (payload_length < 6U))
  {
    return ICARUS_COMMAND_REJECTED_MALFORMED;
  }
  argument_length = payload[5];
  if (payload_length != 6U + argument_length)
  {
    return ICARUS_COMMAND_REJECTED_MALFORMED;
  }

  command->command_id = GetBe32(payload);
  command->opcode = payload[4];
  command->motor_rpm = 0;
  command->solenoid_index = 0U;
  command->solenoid_open = 0U;
  switch (command->opcode)
  {
    case ICARUS_COMMAND_SAFE:
    case ICARUS_COMMAND_ARM:
    case ICARUS_COMMAND_CLEAR_FAULT:
      return (argument_length == 0U) ? ICARUS_COMMAND_ACCEPTED
                                     : ICARUS_COMMAND_REJECTED_MALFORMED;
    case ICARUS_COMMAND_SET_MOTOR_RPM:
      if (argument_length != 4U)
      {
        return ICARUS_COMMAND_REJECTED_MALFORMED;
      }
      command->motor_rpm = (int32_t)GetBe32(&payload[6]);
      return ((command->motor_rpm >= -ICARUS_COMMAND_MAX_MOTOR_RPM) &&
              (command->motor_rpm <= ICARUS_COMMAND_MAX_MOTOR_RPM))
                 ? ICARUS_COMMAND_ACCEPTED
                 : ICARUS_COMMAND_REJECTED_RANGE;
    case ICARUS_COMMAND_SET_SOLENOID:
      if ((argument_length != 2U) ||
          (payload[6] >= ICARUS_COMMAND_MAX_SOLENOIDS) || (payload[7] > 1U))
      {
        return ICARUS_COMMAND_REJECTED_RANGE;
      }
      command->solenoid_index = payload[6];
      command->solenoid_open = payload[7];
      return ICARUS_COMMAND_ACCEPTED;
    default:
      return ICARUS_COMMAND_REJECTED_MALFORMED;
  }
}

icarus_command_result_t IcarusCommand_Encode(const icarus_command_t *command,
                                              uint8_t *payload,
                                              size_t payload_capacity,
                                              size_t *payload_length)
{
  size_t required_length = 6U;

  if ((command == NULL) || (payload == NULL) || (payload_length == NULL))
  {
    return ICARUS_COMMAND_REJECTED_MALFORMED;
  }
  switch (command->opcode)
  {
    case ICARUS_COMMAND_SAFE:
    case ICARUS_COMMAND_ARM:
    case ICARUS_COMMAND_CLEAR_FAULT:
      break;
    case ICARUS_COMMAND_SET_MOTOR_RPM:
      if ((command->motor_rpm < -ICARUS_COMMAND_MAX_MOTOR_RPM) ||
          (command->motor_rpm > ICARUS_COMMAND_MAX_MOTOR_RPM))
      {
        return ICARUS_COMMAND_REJECTED_RANGE;
      }
      required_length += 4U;
      break;
    case ICARUS_COMMAND_SET_SOLENOID:
      if ((command->solenoid_index >= ICARUS_COMMAND_MAX_SOLENOIDS) ||
          (command->solenoid_open > 1U))
      {
        return ICARUS_COMMAND_REJECTED_RANGE;
      }
      required_length += 2U;
      break;
    default:
      return ICARUS_COMMAND_REJECTED_MALFORMED;
  }
  if (payload_capacity < required_length)
  {
    return ICARUS_COMMAND_REJECTED_MALFORMED;
  }

  PutBe32(payload, command->command_id);
  payload[4] = command->opcode;
  payload[5] = (uint8_t)(required_length - 6U);
  if (command->opcode == ICARUS_COMMAND_SET_MOTOR_RPM)
  {
    PutBe32(&payload[6], (uint32_t)command->motor_rpm);
  }
  else if (command->opcode == ICARUS_COMMAND_SET_SOLENOID)
  {
    payload[6] = command->solenoid_index;
    payload[7] = command->solenoid_open;
  }
  *payload_length = required_length;
  return ICARUS_COMMAND_ACCEPTED;
}

void IcarusCommand_EncodeAck(const icarus_command_ack_t *ack,
                             uint8_t payload[8])
{
  PutBe32(payload, ack->command_id);
  payload[4] = ack->result;
  payload[5] = ack->safety_state;
  payload[6] = (uint8_t)(ack->fault_mask >> 8U);
  payload[7] = (uint8_t)ack->fault_mask;
}
