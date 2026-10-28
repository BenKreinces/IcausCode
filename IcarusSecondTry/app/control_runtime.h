/**
 * @file control_runtime.h
 * @brief Command admission boundary between network packets and actuators.
 */
#ifndef ICARUS_CONTROL_RUNTIME_H
#define ICARUS_CONTROL_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "command.h"
#include "protocol.h"
#include "safety.h"

typedef bool (*icarus_apply_target_fn)(void *context,
                                       const icarus_actuator_target_t *target,
                                       bool outputs_enabled);

typedef struct
{
  icarus_safety_t safety;
  void *actuator_context;
  icarus_apply_target_fn apply_target;
  uint32_t last_packet_sequence;
  uint32_t accepted_commands;
  uint32_t rejected_commands;
  uint32_t malformed_packets;
  bool packet_sequence_seen;
} icarus_control_runtime_t;

void IcarusControlRuntime_Init(icarus_control_runtime_t *runtime,
                               void *actuator_context,
                               icarus_apply_target_fn apply_target);
void IcarusControlRuntime_SetInterlocksReady(icarus_control_runtime_t *runtime,
                                             bool ready);
icarus_protocol_result_t IcarusControlRuntime_HandleDatagram(
    icarus_control_runtime_t *runtime, const uint8_t *datagram,
    size_t datagram_length, uint32_t now_ms, icarus_command_ack_t *ack);
void IcarusControlRuntime_Update(icarus_control_runtime_t *runtime,
                                 uint32_t now_ms);

#endif /* ICARUS_CONTROL_RUNTIME_H */
