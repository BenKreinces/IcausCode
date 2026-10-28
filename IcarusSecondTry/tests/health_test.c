#include <assert.h>
#include <stdio.h>

#include "health.h"

int main(void)
{
  icarus_health_t health;
  IcarusHealth_Init(&health, ICARUS_TASK_NETWORK | ICARUS_TASK_ACTUATOR |
                     ICARUS_TASK_DIAGNOSTICS,
                     ICARUS_TASK_NETWORK | ICARUS_TASK_ACTUATOR, 100U);
  assert(!IcarusHealth_WatchdogEligible(&health));
  IcarusHealth_Beat(&health, ICARUS_TASK_NETWORK, 10U);
  IcarusHealth_Beat(&health, ICARUS_TASK_ACTUATOR, 10U);
  IcarusHealth_Beat(&health, ICARUS_TASK_DIAGNOSTICS, 10U);
  IcarusHealth_Update(&health, 100U);
  assert(IcarusHealth_WatchdogEligible(&health));
  IcarusHealth_Update(&health, 111U);
  assert(!IcarusHealth_WatchdogEligible(&health));
  assert((health.unhealthy_task_mask & ICARUS_TASK_NETWORK) != 0U);
  puts("Health tests passed");
  return 0;
}
