#include "health.h"

#include <stddef.h>

static int TaskIndex(uint32_t task_bit)
{
  int index = 0;
  if ((task_bit == 0U) || ((task_bit & (task_bit - 1U)) != 0U)) return -1;
  while (task_bit > 1U)
  {
    task_bit >>= 1U;
    ++index;
  }
  return (index < 6) ? index : -1;
}

void IcarusHealth_Init(icarus_health_t *health, uint32_t required_task_mask,
                       uint32_t critical_task_mask, uint32_t task_timeout_ms)
{
  size_t index;
  if ((health == NULL) || (task_timeout_ms == 0U)) return;
  health->required_task_mask = required_task_mask;
  health->critical_task_mask = critical_task_mask & required_task_mask;
  health->task_timeout_ms = task_timeout_ms;
  for (index = 0U; index < 6U; ++index) health->last_beat_ms[index] = 0U;
  health->seen_task_mask = 0U;
  health->unhealthy_task_mask = required_task_mask;
  health->queue_drops = 0U;
  health->malformed_packets = 0U;
  health->peripheral_errors = 0U;
}

void IcarusHealth_Beat(icarus_health_t *health, uint32_t task_bit,
                       uint32_t now_ms)
{
  const int index = TaskIndex(task_bit);
  if ((health == NULL) || (index < 0) ||
      ((health->required_task_mask & task_bit) == 0U)) return;
  health->last_beat_ms[index] = now_ms;
  health->seen_task_mask |= task_bit;
}

void IcarusHealth_Update(icarus_health_t *health, uint32_t now_ms)
{
  uint32_t task_bit;
  if (health == NULL) return;
  health->unhealthy_task_mask = 0U;
  for (task_bit = 1U; task_bit <= ICARUS_TASK_DIAGNOSTICS; task_bit <<= 1U)
  {
    const int index = TaskIndex(task_bit);
    if ((health->required_task_mask & task_bit) == 0U) continue;
    if (((health->seen_task_mask & task_bit) == 0U) ||
        ((uint32_t)(now_ms - health->last_beat_ms[index]) > health->task_timeout_ms))
    {
      health->unhealthy_task_mask |= task_bit;
    }
  }
}

bool IcarusHealth_WatchdogEligible(const icarus_health_t *health)
{
  return (health != NULL) &&
         ((health->unhealthy_task_mask & health->critical_task_mask) == 0U);
}
