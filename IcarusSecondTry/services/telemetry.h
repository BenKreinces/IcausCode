/** @file telemetry.h @brief Explicit telemetry and heartbeat payload encoding. */
#ifndef ICARUS_TELEMETRY_H
#define ICARUS_TELEMETRY_H

#include <stddef.h>
#include <stdint.h>

#define ICARUS_TELEMETRY_PRESSURE_COUNT 24U
#define ICARUS_TELEMETRY_FLOW_COUNT 4U
#define ICARUS_TELEMETRY_PAYLOAD_LENGTH 142U
#define ICARUS_HEARTBEAT_PAYLOAD_LENGTH 12U

typedef struct
{
  uint32_t sample_counter;
  uint8_t safety_state;
  uint8_t link_state;
  uint16_t fault_mask;
  uint32_t valid_pressure_mask;
  uint8_t valid_flow_mask;
  uint8_t task_health_mask;
  int32_t motor_target_rpm;
  int32_t motor_measured_rpm;
  uint32_t solenoid_open_mask;
  int32_t load_cell_raw_code;
  int32_t pressure_kpa_x10[ICARUS_TELEMETRY_PRESSURE_COUNT];
  int32_t flow_ml_per_second[ICARUS_TELEMETRY_FLOW_COUNT];
} icarus_telemetry_snapshot_t;

typedef struct
{
  uint32_t uptime_ms;
  uint32_t last_command_ms;
  uint16_t fault_mask;
  uint8_t safety_state;
  uint8_t task_health_mask;
} icarus_heartbeat_t;

void IcarusTelemetry_Encode(const icarus_telemetry_snapshot_t *snapshot,
                            uint8_t payload[ICARUS_TELEMETRY_PAYLOAD_LENGTH]);
void IcarusHeartbeat_Encode(const icarus_heartbeat_t *heartbeat,
                            uint8_t payload[ICARUS_HEARTBEAT_PAYLOAD_LENGTH]);

#endif /* ICARUS_TELEMETRY_H */
