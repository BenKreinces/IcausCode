#include "flowmeter.h"

#include <limits.h>
#include <stddef.h>

bool Flowmeter_Init(flowmeter_t *meter, uint32_t timer_hz, uint32_t counter_mask,
                    uint32_t microliters_per_pulse, uint32_t stale_after_ms)
{
  if ((meter == NULL) || (timer_hz == 0U) || (counter_mask == 0U) ||
      (microliters_per_pulse == 0U) || (stale_after_ms == 0U))
  {
    return false;
  }
  meter->timer_hz = timer_hz;
  meter->counter_mask = counter_mask;
  meter->stale_after_ms = stale_after_ms;
  meter->microliters_per_pulse = microliters_per_pulse;
  meter->last_capture_tick = 0U;
  meter->last_capture_ms = 0U;
  meter->frequency_millihz = 0U;
  meter->capture_count = 0U;
  meter->invalid_capture_count = 0U;
  meter->have_capture = false;
  meter->valid = false;
  return true;
}

void Flowmeter_OnCapture(flowmeter_t *meter, uint32_t captured_tick,
                         uint32_t now_ms)
{
  uint32_t delta;
  uint64_t frequency_millihz;
  if (meter == NULL) return;
  captured_tick &= meter->counter_mask;
  if (!meter->have_capture)
  {
    meter->last_capture_tick = captured_tick;
    meter->last_capture_ms = now_ms;
    meter->have_capture = true;
    return;
  }
  delta = (captured_tick - meter->last_capture_tick) & meter->counter_mask;
  meter->last_capture_tick = captured_tick;
  meter->last_capture_ms = now_ms;
  if (delta == 0U)
  {
    ++meter->invalid_capture_count;
    return;
  }
  frequency_millihz = ((uint64_t)meter->timer_hz * 1000U) / delta;
  meter->frequency_millihz = (frequency_millihz > UINT32_MAX)
                               ? UINT32_MAX : (uint32_t)frequency_millihz;
  ++meter->capture_count;
  meter->valid = true;
}

void Flowmeter_Update(flowmeter_t *meter, uint32_t now_ms)
{
  if ((meter == NULL) || !meter->have_capture) return;
  if ((uint32_t)(now_ms - meter->last_capture_ms) > meter->stale_after_ms)
  {
    meter->frequency_millihz = 0U;
    meter->valid = false;
  }
}

uint32_t Flowmeter_GetMlPerSecond(const flowmeter_t *meter)
{
  uint64_t flow;
  if ((meter == NULL) || !meter->valid) return 0U;
  flow = ((uint64_t)meter->frequency_millihz * meter->microliters_per_pulse) /
         UINT32_C(1000000);
  return (flow > UINT32_MAX) ? UINT32_MAX : (uint32_t)flow;
}
