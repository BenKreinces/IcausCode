/** @file health.h @brief Task-liveness and bounded-resource health service. */
#ifndef ICARUS_HEALTH_H
#define ICARUS_HEALTH_H

#include <stdbool.h>
#include <stdint.h>

enum
{
  ICARUS_TASK_NETWORK = 1U << 0,
  ICARUS_TASK_COMMAND = 1U << 1,
  ICARUS_TASK_SENSOR = 1U << 2,
  ICARUS_TASK_ACTUATOR = 1U << 3,
  ICARUS_TASK_TELEMETRY = 1U << 4,
  ICARUS_TASK_DIAGNOSTICS = 1U << 5
};

typedef struct
{
  uint32_t required_task_mask;
  uint32_t critical_task_mask;
  uint32_t task_timeout_ms;
  uint32_t last_beat_ms[6];
  uint32_t seen_task_mask;
  uint32_t unhealthy_task_mask;
  uint32_t queue_drops;
  uint32_t malformed_packets;
  uint32_t peripheral_errors;
} icarus_health_t;

void IcarusHealth_Init(icarus_health_t *health, uint32_t required_task_mask,
                       uint32_t critical_task_mask, uint32_t task_timeout_ms);
void IcarusHealth_Beat(icarus_health_t *health, uint32_t task_bit,
                       uint32_t now_ms);
void IcarusHealth_Update(icarus_health_t *health, uint32_t now_ms);
bool IcarusHealth_WatchdogEligible(const icarus_health_t *health);

#endif /* ICARUS_HEALTH_H */
