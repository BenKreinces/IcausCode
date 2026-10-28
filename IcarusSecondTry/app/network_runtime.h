/** @file network_runtime.h @brief Network-owner task policy above the raw UDP bridge. */
#ifndef ICARUS_NETWORK_RUNTIME_H
#define ICARUS_NETWORK_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "control_runtime.h"
#include "telemetry.h"
#include "udp_service.h"

typedef struct
{
  icarus_udp_service_t udp;
  icarus_control_runtime_t control;
  icarus_telemetry_snapshot_t telemetry;
  icarus_udp_datagram_t receive_scratch;
  uint8_t telemetry_payload[ICARUS_TELEMETRY_PAYLOAD_LENGTH];
  uint32_t next_tx_sequence;
  uint32_t last_telemetry_ms;
  uint32_t telemetry_peer_ipv4;
  uint16_t telemetry_peer_port;
  bool telemetry_peer_known;
  uint32_t echo_requests;
  uint32_t malformed_packets;
} icarus_network_runtime_t;

bool IcarusNetworkRuntime_Init(icarus_network_runtime_t *runtime,
                               void *actuator_context,
                               icarus_apply_target_fn apply_target);
void IcarusNetworkRuntime_SetInterlocksReady(icarus_network_runtime_t *runtime,
                                             bool ready);
void IcarusNetworkRuntime_SetTelemetry(icarus_network_runtime_t *runtime,
                                       const icarus_telemetry_snapshot_t *snapshot);
void IcarusNetworkRuntime_Process(icarus_network_runtime_t *runtime,
                                  uint32_t now_ms);

#endif /* ICARUS_NETWORK_RUNTIME_H */
