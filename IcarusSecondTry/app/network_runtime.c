#include "network_runtime.h"

#include <string.h>

#define ICARUS_TELEMETRY_PERIOD_MS UINT32_C(100)

static bool SendPacket(icarus_network_runtime_t *runtime, uint8_t slot,
                       uint32_t address, uint16_t port, uint8_t message_type,
                       const uint8_t *payload, uint16_t payload_length,
                       uint32_t now_ms)
{
  icarus_packet_header_t header = {.message_type = message_type, .flags = 0U,
                                   .payload_length = payload_length,
                                   .sequence = runtime->next_tx_sequence++,
                                   .timestamp_ms = now_ms};
  uint8_t packet[ICARUS_UDP_MAX_DATAGRAM];
  size_t packet_length;
  if (IcarusProtocol_Encode(&header, payload, packet, sizeof(packet),
                            &packet_length) != ICARUS_PROTOCOL_OK)
  {
    return false;
  }
  return IcarusUdpService_Send(&runtime->udp, slot, address, port, packet,
                               (uint16_t)packet_length);
}

bool IcarusNetworkRuntime_Init(icarus_network_runtime_t *runtime,
                               void *actuator_context,
                               icarus_apply_target_fn apply_target)
{
  if (runtime == NULL) return false;
  memset(runtime, 0, sizeof(*runtime));
  IcarusControlRuntime_Init(&runtime->control, actuator_context, apply_target);
  runtime->next_tx_sequence = 1U;
  return IcarusUdpService_Init(&runtime->udp);
}

void IcarusNetworkRuntime_SetInterlocksReady(icarus_network_runtime_t *runtime,
                                             bool ready)
{
  if (runtime != NULL) IcarusControlRuntime_SetInterlocksReady(&runtime->control, ready);
}

void IcarusNetworkRuntime_SetTelemetry(icarus_network_runtime_t *runtime,
                                       const icarus_telemetry_snapshot_t *snapshot)
{
  if ((runtime != NULL) && (snapshot != NULL)) runtime->telemetry = *snapshot;
}

static void ProcessDatagram(icarus_network_runtime_t *runtime,
                            const icarus_udp_datagram_t *datagram,
                            uint32_t now_ms)
{
  icarus_packet_header_t header;
  const uint8_t *payload;
  icarus_protocol_result_t result;
  result = IcarusProtocol_Decode(datagram->data, datagram->length, &header, &payload);
  if (result != ICARUS_PROTOCOL_OK)
  {
    ++runtime->malformed_packets;
    return;
  }
  runtime->telemetry_peer_ipv4 = datagram->source_ipv4;
  runtime->telemetry_peer_port = datagram->source_port;
  runtime->telemetry_peer_known = true;
  if (header.message_type == ICARUS_MESSAGE_ECHO_REQUEST)
  {
    ++runtime->echo_requests;
    (void)SendPacket(runtime, 0U, datagram->source_ipv4, datagram->source_port,
                     ICARUS_MESSAGE_ECHO_RESPONSE, payload, header.payload_length,
                     now_ms);
  }
  else if (header.message_type == ICARUS_MESSAGE_COMMAND)
  {
    icarus_command_ack_t ack;
    uint8_t ack_payload[8];
    result = IcarusControlRuntime_HandleDatagram(&runtime->control, datagram->data,
                                                 datagram->length, now_ms, &ack);
    if (result == ICARUS_PROTOCOL_OK)
    {
      IcarusCommand_EncodeAck(&ack, ack_payload);
      (void)SendPacket(runtime, 0U, datagram->source_ipv4, datagram->source_port,
                       ICARUS_MESSAGE_COMMAND_ACK, ack_payload, sizeof(ack_payload),
                       now_ms);
    }
    else
    {
      ++runtime->malformed_packets;
    }
  }
  /* Heartbeats establish telemetry reachability but never modify outputs. */
}

void IcarusNetworkRuntime_Process(icarus_network_runtime_t *runtime,
                                  uint32_t now_ms)
{
  if (runtime == NULL) return;
  while (IcarusUdpService_TryReceive(&runtime->udp, &runtime->receive_scratch))
  {
    ProcessDatagram(runtime, &runtime->receive_scratch, now_ms);
  }
  IcarusControlRuntime_Update(&runtime->control, now_ms);
  if (runtime->telemetry_peer_known &&
      ((uint32_t)(now_ms - runtime->last_telemetry_ms) >= ICARUS_TELEMETRY_PERIOD_MS))
  {
    runtime->telemetry.sample_counter++;
    runtime->telemetry.safety_state = (uint8_t)runtime->control.safety.state;
    runtime->telemetry.fault_mask = runtime->control.safety.fault_mask;
    runtime->telemetry.motor_target_rpm = runtime->control.safety.target.motor_rpm;
    runtime->telemetry.solenoid_open_mask = runtime->control.safety.target.solenoid_open_mask;
    IcarusTelemetry_Encode(&runtime->telemetry, runtime->telemetry_payload);
    if (SendPacket(runtime, 1U, runtime->telemetry_peer_ipv4,
                   ICARUS_UDP_TELEMETRY_PORT, ICARUS_MESSAGE_TELEMETRY,
                   runtime->telemetry_payload,
                   sizeof(runtime->telemetry_payload), now_ms))
    {
      runtime->last_telemetry_ms = now_ms;
    }
  }
}
