/** @file flowmeter.h @brief Pure timer-capture flowmeter conversion. */
#ifndef ICARUS_FLOWMETER_H
#define ICARUS_FLOWMETER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint32_t timer_hz;
  uint32_t counter_mask;
  uint32_t stale_after_ms;
  uint32_t microliters_per_pulse;
  uint32_t last_capture_tick;
  uint32_t last_capture_ms;
  uint32_t frequency_millihz;
  uint32_t capture_count;
  uint32_t invalid_capture_count;
  bool have_capture;
  bool valid;
} flowmeter_t;

bool Flowmeter_Init(flowmeter_t *meter, uint32_t timer_hz, uint32_t counter_mask,
                    uint32_t microliters_per_pulse, uint32_t stale_after_ms);
void Flowmeter_OnCapture(flowmeter_t *meter, uint32_t captured_tick,
                         uint32_t now_ms);
void Flowmeter_Update(flowmeter_t *meter, uint32_t now_ms);
uint32_t Flowmeter_GetMlPerSecond(const flowmeter_t *meter);

#endif /* ICARUS_FLOWMETER_H */
