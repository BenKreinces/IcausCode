#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "command.h"
#include "protocol.h"
#include "safety.h"

static void TestPacketRoundTripAndRejection(void)
{
  const uint8_t payload[] = {0x10U, 0x20U, 0x30U};
  const icarus_packet_header_t header = {
      .message_type = ICARUS_MESSAGE_ECHO_REQUEST,
      .flags = 0x5AU,
      .payload_length = sizeof(payload),
      .sequence = 0x10203040U,
      .timestamp_ms = 0x50607080U};
  uint8_t packet[64];
  size_t packet_length = 0U;
  icarus_packet_header_t decoded;
  const uint8_t *decoded_payload = NULL;

  assert(IcarusProtocol_Encode(&header, payload, packet, sizeof(packet),
                               &packet_length) == ICARUS_PROTOCOL_OK);
  assert(packet_length == ICARUS_PROTOCOL_HEADER_LENGTH + sizeof(payload));
  assert(packet[0] == 0x49U && packet[1] == 0x43U && packet[2] == 1U &&
         packet[3] == 5U && packet[4] == 20U && packet[5] == 0x5AU &&
         packet[16] == 0xDEU && packet[17] == 0xDDU &&
         packet[18] == 0xF8U && packet[19] == 0x54U);
  assert(IcarusProtocol_Decode(packet, packet_length, &decoded,
                               &decoded_payload) == ICARUS_PROTOCOL_OK);
  assert(decoded.message_type == header.message_type);
  assert(decoded.flags == header.flags);
  assert(decoded.payload_length == sizeof(payload));
  assert(decoded.sequence == header.sequence);
  assert(decoded.timestamp_ms == header.timestamp_ms);
  assert(memcmp(decoded_payload, payload, sizeof(payload)) == 0);

  packet[20] ^= 0x01U;
  assert(IcarusProtocol_Decode(packet, packet_length, &decoded,
                               &decoded_payload) == ICARUS_PROTOCOL_BAD_CRC);
  packet[20] ^= 0x01U;
  assert(IcarusProtocol_Decode(packet, packet_length - 1U, &decoded,
                               &decoded_payload) == ICARUS_PROTOCOL_BAD_LENGTH);
  packet[0] = 0U;
  assert(IcarusProtocol_Decode(packet, packet_length, &decoded,
                               &decoded_payload) == ICARUS_PROTOCOL_BAD_MAGIC);
}

static icarus_command_t Command(uint32_t id, uint8_t opcode)
{
  icarus_command_t command = {.command_id = id, .opcode = opcode,
                               .motor_rpm = 0, .solenoid_index = 0,
                               .solenoid_open = 0};
  return command;
}

static void TestCommandAndSafety(void)
{
  icarus_safety_t safety;
  icarus_command_t command;
  uint8_t payload[16];
  size_t payload_length = 0U;
  icarus_command_t decoded;

  IcarusSafety_Init(&safety);
  command = Command(1U, ICARUS_COMMAND_ARM);
  assert(IcarusSafety_ApplyCommand(&safety, &command, 10U) ==
         ICARUS_COMMAND_REJECTED_INTERLOCK);

  IcarusSafety_SetInterlocksReady(&safety, true);
  assert(IcarusSafety_ApplyCommand(&safety, &command, 20U) ==
         ICARUS_COMMAND_ACCEPTED);
  assert(IcarusSafety_IsOutputEnabled(&safety));

  command = Command(2U, ICARUS_COMMAND_SET_MOTOR_RPM);
  command.motor_rpm = 4500;
  assert(IcarusCommand_Encode(&command, payload, sizeof(payload),
                              &payload_length) == ICARUS_COMMAND_ACCEPTED);
  assert(IcarusCommand_Decode(payload, payload_length, &decoded) ==
         ICARUS_COMMAND_ACCEPTED);
  assert(decoded.motor_rpm == 4500);
  assert(IcarusSafety_ApplyCommand(&safety, &decoded, 30U) ==
         ICARUS_COMMAND_ACCEPTED);
  assert(safety.target.motor_rpm == 4500);

  command = Command(3U, ICARUS_COMMAND_SET_SOLENOID);
  command.solenoid_index = 23U;
  command.solenoid_open = 1U;
  assert(IcarusSafety_ApplyCommand(&safety, &command, 40U) ==
         ICARUS_COMMAND_ACCEPTED);
  assert((safety.target.solenoid_open_mask & (UINT32_C(1) << 23U)) != 0U);

  assert(IcarusSafety_ApplyCommand(&safety, &command, 50U) ==
         ICARUS_COMMAND_REJECTED_DUPLICATE);
  IcarusSafety_Update(&safety, 600U);
  assert(safety.state == ICARUS_SAFETY_FAULT);
  assert((safety.fault_mask & ICARUS_FAULT_COMMAND_TIMEOUT) != 0U);
  assert(safety.target.motor_rpm == 0);
  assert(safety.target.solenoid_open_mask == 0U);
}

int main(void)
{
  TestPacketRoundTripAndRejection();
  TestCommandAndSafety();
  puts("Protocol and safety tests passed");
  return 0;
}
