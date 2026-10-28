#include "control_runtime.h"

#include <string.h>

static bool IsSerialOlder(uint32_t candidate, uint32_t reference)
{
  return ((int32_t)(candidate - reference) < 0);
}

static void ApplySafeIfNeeded(icarus_control_runtime_t *runtime, uint32_t now_ms)
{
  if ((runtime->apply_target != NULL) &&
      !runtime->apply_target(runtime->actuator_context, &runtime->safety.target,
                             IcarusSafety_IsOutputEnabled(&runtime->safety)))
  {
    IcarusSafety_RaiseFault(&runtime->safety, ICARUS_FAULT_ACTUATOR, now_ms);
    (void)runtime->apply_target(runtime->actuator_context, &runtime->safety.target,
                                false);
  }
}

void IcarusControlRuntime_Init(icarus_control_runtime_t *runtime,
                               void *actuator_context,
                               icarus_apply_target_fn apply_target)
{
  if (runtime == NULL) return;
  memset(runtime, 0, sizeof(*runtime));
  runtime->actuator_context = actuator_context;
  runtime->apply_target = apply_target;
  IcarusSafety_Init(&runtime->safety);
}

void IcarusControlRuntime_SetInterlocksReady(icarus_control_runtime_t *runtime,
                                             bool ready)
{
  if (runtime != NULL) IcarusSafety_SetInterlocksReady(&runtime->safety, ready);
}

icarus_protocol_result_t IcarusControlRuntime_HandleDatagram(
    icarus_control_runtime_t *runtime, const uint8_t *datagram,
    size_t datagram_length, uint32_t now_ms, icarus_command_ack_t *ack)
{
  icarus_packet_header_t header;
  const uint8_t *payload;
  icarus_command_t command;
  icarus_command_result_t command_result;
  icarus_protocol_result_t protocol_result;

  if ((runtime == NULL) || (ack == NULL)) return ICARUS_PROTOCOL_ARGUMENT_ERROR;
  memset(ack, 0, sizeof(*ack));
  protocol_result = IcarusProtocol_Decode(datagram, datagram_length, &header,
                                          &payload);
  if (protocol_result != ICARUS_PROTOCOL_OK)
  {
    ++runtime->malformed_packets;
    return protocol_result;
  }
  if (header.message_type != ICARUS_MESSAGE_COMMAND)
  {
    ++runtime->malformed_packets;
    return ICARUS_PROTOCOL_BAD_TYPE;
  }
  command_result = IcarusCommand_Decode(payload, header.payload_length, &command);
  if (command_result != ICARUS_COMMAND_ACCEPTED)
  {
    ++runtime->rejected_commands;
    ack->result = command_result;
    ack->safety_state = (uint8_t)runtime->safety.state;
    ack->fault_mask = runtime->safety.fault_mask;
    return ICARUS_PROTOCOL_OK;
  }
  ack->command_id = command.command_id;
  if (runtime->packet_sequence_seen &&
      IsSerialOlder(header.sequence, runtime->last_packet_sequence))
  {
    ++runtime->rejected_commands;
    ack->result = ICARUS_COMMAND_REJECTED_STALE;
  }
  else
  {
    command_result = IcarusSafety_ApplyCommand(&runtime->safety, &command, now_ms);
    ack->result = command_result;
    if (command_result == ICARUS_COMMAND_ACCEPTED)
    {
      runtime->last_packet_sequence = header.sequence;
      runtime->packet_sequence_seen = true;
      ++runtime->accepted_commands;
      ApplySafeIfNeeded(runtime, now_ms);
    }
    else
    {
      ++runtime->rejected_commands;
    }
  }
  ack->safety_state = (uint8_t)runtime->safety.state;
  ack->fault_mask = runtime->safety.fault_mask;
  return ICARUS_PROTOCOL_OK;
}

void IcarusControlRuntime_Update(icarus_control_runtime_t *runtime,
                                 uint32_t now_ms)
{
  icarus_safety_state_t state_before;
  if (runtime == NULL) return;
  state_before = runtime->safety.state;
  IcarusSafety_Update(&runtime->safety, now_ms);
  if ((state_before != runtime->safety.state) ||
      !IcarusSafety_IsOutputEnabled(&runtime->safety))
  {
    ApplySafeIfNeeded(runtime, now_ms);
  }
}
