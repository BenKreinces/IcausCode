#include <assert.h>
#include <stdio.h>

#include "flowmeter.h"

int main(void)
{
  flowmeter_t meter;
  assert(Flowmeter_Init(&meter, 1000000U, 0xFFFFU, 2500U, 500U));
  Flowmeter_OnCapture(&meter, 65000U, 10U);
  assert(!meter.valid);
  Flowmeter_OnCapture(&meter, 464U, 20U); /* wraps: 1000 timer ticks */
  assert(meter.valid && meter.frequency_millihz == 1000000U);
  assert(Flowmeter_GetMlPerSecond(&meter) == 2500U);
  Flowmeter_Update(&meter, 521U);
  assert(!meter.valid && Flowmeter_GetMlPerSecond(&meter) == 0U);
  puts("Flowmeter tests passed");
  return 0;
}
