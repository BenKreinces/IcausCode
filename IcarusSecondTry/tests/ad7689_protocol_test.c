#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ad7689.h"

typedef struct
{
  bool convert;
  uint32_t delay_total_ns;
  uint8_t last_tx[2];
  bool fail;
} fake_ad7689_t;

static bool Transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length)
{
  fake_ad7689_t *fake = context;
  assert(length == 2U);
  memcpy(fake->last_tx, tx, 2U);
  if (fake->fail) return false;
  rx[0] = 0x12U;
  rx[1] = 0x34U;
  return true;
}

static void SetConvert(void *context, bool asserted)
{
  ((fake_ad7689_t *)context)->convert = asserted;
}

static void Delay(void *context, uint32_t ns)
{
  ((fake_ad7689_t *)context)->delay_total_ns += ns;
}

int main(void)
{
  fake_ad7689_t fake;
  ad7689_t adc;
  uint16_t config[AD7689_CHANNEL_COUNT] = {0x3C40U, 0U, 0U, 0U,
                                             0U, 0U, 0U, 0U};
  const ad7689_bus_t bus = {.context = &fake, .transfer = Transfer,
                             .set_convert = SetConvert, .delay_ns = Delay};
  uint16_t raw;

  memset(&fake, 0, sizeof(fake));
  assert(AD7689_Init(&adc, &bus, config, 5000U) == AD7689_OK);
  assert(AD7689_ReadChannel(&adc, 0U, &raw) == AD7689_OK);
  assert(raw == 0x1234U && adc.latest_raw[0] == raw);
  assert(fake.last_tx[0] == 0xF1U && fake.last_tx[1] == 0x00U);
  assert(fake.delay_total_ns == 5050U && !fake.convert);
  assert(AD7689_ReadChannel(&adc, 8U, &raw) == AD7689_RANGE_ERROR);
  fake.fail = true;
  assert(AD7689_ReadChannel(&adc, 0U, &raw) == AD7689_BUS_ERROR);
  assert(adc.transfer_errors == 1U);
  puts("AD7689 protocol tests passed");
  return 0;
}
