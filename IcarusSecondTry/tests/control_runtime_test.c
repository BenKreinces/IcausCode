#include <assert.h>
#include <stdio.h>

#include "control_runtime.h"

typedef struct
{
  icarus_actuator_target_t last_target;
  bool enabled;
  unsigned calls;
} fake_actuator_t;

static bool Apply(void *context, const icarus_actuator_target_t *target, bool enabled)
{
  fake_actuator_t *actuator = context;
  actuator->last_target = *target;
  actuator->enabled = enabled;
  ++actuator->calls;
  return true;
}

static size_t MakeCommandPacket(uint32_t packet_sequence, uint32_t command_id,
                                uint8_t opcode, int32_t rpm, uint8_t *packet)
{
  icarus_command_t command = {.command_id = command_id, .opcode = opcode,
                               .motor_rpm = rpm, .solenoid_index = 0U,
                               .solenoid_open = 0U};
  icarus_packet_header_t header = {.message_type = ICARUS_MESSAGE_COMMAND,
                                   .flags = 0U, .payload_length = 0U,
                                   .sequence = packet_sequence, .timestamp_ms = 0U};
  uint8_t payload[16];
  size_t payload_length;
  size_t packet_length;
  assert(IcarusCommand_Encode(&command, payload, sizeof(payload), &payload_length) ==
         ICARUS_COMMAND_ACCEPTED);
  header.payload_length = (uint16_t)payload_length;
  assert(IcarusProtocol_Encode(&header, payload, packet, 64U, &packet_length) ==
         ICARUS_PROTOCOL_OK);
  return packet_length;
}

int main(void)
{
  icarus_control_runtime_t runtime;
  fake_actuator_t actuator = {0};
  icarus_command_ack_t ack;
  uint8_t packet[64];
  size_t length;

  IcarusControlRuntime_Init(&runtime, &actuator, Apply);
  IcarusControlRuntime_SetInterlocksReady(&runtime, true);
  length = MakeCommandPacket(1U, 1U, ICARUS_COMMAND_ARM, 0, packet);
  assert(IcarusControlRuntime_HandleDatagram(&runtime, packet, length, 10U, &ack) ==
         ICARUS_PROTOCOL_OK && ack.result == ICARUS_COMMAND_ACCEPTED);
  assert(actuator.enabled);
  length = MakeCommandPacket(2U, 2U, ICARUS_COMMAND_SET_MOTOR_RPM, 5000, packet);
  assert(IcarusControlRuntime_HandleDatagram(&runtime, packet, length, 20U, &ack) ==
         ICARUS_PROTOCOL_OK && actuator.last_target.motor_rpm == 5000);
  {
    const unsigned calls_before = actuator.calls;
    packet[20] ^= 1U;
    assert(IcarusControlRuntime_HandleDatagram(&runtime, packet, length, 25U, &ack) ==
           ICARUS_PROTOCOL_BAD_CRC);
    assert(actuator.calls == calls_before);
    packet[20] ^= 1U;
  }
  IcarusControlRuntime_Update(&runtime, 521U);
  assert(runtime.safety.state == ICARUS_SAFETY_FAULT);
  assert(!actuator.enabled && actuator.last_target.motor_rpm == 0);
  puts("Control runtime tests passed");
  return 0;
}
