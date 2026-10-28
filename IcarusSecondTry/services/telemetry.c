#include "telemetry.h"

static void PutBe16(uint8_t *output, uint16_t value)
{
  output[0] = (uint8_t)(value >> 8U);
  output[1] = (uint8_t)value;
}

static void PutBe32(uint8_t *output, uint32_t value)
{
  output[0] = (uint8_t)(value >> 24U);
  output[1] = (uint8_t)(value >> 16U);
  output[2] = (uint8_t)(value >> 8U);
  output[3] = (uint8_t)value;
}

void IcarusTelemetry_Encode(const icarus_telemetry_snapshot_t *snapshot,
                            uint8_t payload[ICARUS_TELEMETRY_PAYLOAD_LENGTH])
{
  size_t offset = 0U;
  size_t index;

  if ((snapshot == NULL) || (payload == NULL))
  {
    return;
  }
  PutBe32(&payload[offset], snapshot->sample_counter); offset += 4U;
  payload[offset++] = snapshot->safety_state;
  payload[offset++] = snapshot->link_state;
  PutBe16(&payload[offset], snapshot->fault_mask); offset += 2U;
  PutBe32(&payload[offset], snapshot->valid_pressure_mask); offset += 4U;
  payload[offset++] = snapshot->valid_flow_mask;
  payload[offset++] = snapshot->task_health_mask;
  PutBe32(&payload[offset], (uint32_t)snapshot->motor_target_rpm); offset += 4U;
  PutBe32(&payload[offset], (uint32_t)snapshot->motor_measured_rpm); offset += 4U;
  PutBe32(&payload[offset], snapshot->solenoid_open_mask); offset += 4U;
  PutBe32(&payload[offset], (uint32_t)snapshot->load_cell_raw_code); offset += 4U;
  for (index = 0U; index < ICARUS_TELEMETRY_PRESSURE_COUNT; ++index)
  {
    PutBe32(&payload[offset], (uint32_t)snapshot->pressure_kpa_x10[index]);
    offset += 4U;
  }
  for (index = 0U; index < ICARUS_TELEMETRY_FLOW_COUNT; ++index)
  {
    PutBe32(&payload[offset], (uint32_t)snapshot->flow_ml_per_second[index]);
    offset += 4U;
  }
}

void IcarusHeartbeat_Encode(const icarus_heartbeat_t *heartbeat,
                            uint8_t payload[ICARUS_HEARTBEAT_PAYLOAD_LENGTH])
{
  if ((heartbeat == NULL) || (payload == NULL))
  {
    return;
  }
  PutBe32(&payload[0], heartbeat->uptime_ms);
  PutBe32(&payload[4], heartbeat->last_command_ms);
  PutBe16(&payload[8], heartbeat->fault_mask);
  payload[10] = heartbeat->safety_state;
  payload[11] = heartbeat->task_health_mask;
}
