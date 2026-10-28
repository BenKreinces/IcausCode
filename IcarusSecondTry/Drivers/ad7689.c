#include "ad7689.h"

#include <string.h>

#define AD7689_CONVERT_PULSE_NS UINT32_C(50)

ad7689_result_t AD7689_Init(ad7689_t *device, const ad7689_bus_t *bus,
                             const uint16_t channel_config[AD7689_CHANNEL_COUNT],
                             uint32_t conversion_time_ns)
{
  if ((device == NULL) || (bus == NULL) || (channel_config == NULL) ||
      (bus->transfer == NULL) || (bus->set_convert == NULL) ||
      (bus->delay_ns == NULL) || (conversion_time_ns == 0U))
  {
    return AD7689_ARGUMENT_ERROR;
  }
  memset(device, 0, sizeof(*device));
  device->bus = *bus;
  memcpy(device->channel_config, channel_config, sizeof(device->channel_config));
  device->conversion_time_ns = conversion_time_ns;
  device->bus.set_convert(device->bus.context, false);
  return AD7689_OK;
}

ad7689_result_t AD7689_ReadChannel(ad7689_t *device, uint8_t channel,
                                    uint16_t *raw_code)
{
  uint8_t tx[2];
  uint8_t rx[2] = {0U, 0U};
  uint16_t config;

  if ((device == NULL) || (raw_code == NULL))
  {
    return AD7689_ARGUMENT_ERROR;
  }
  if (channel >= AD7689_CHANNEL_COUNT)
  {
    return AD7689_RANGE_ERROR;
  }
  config = (uint16_t)(device->channel_config[channel] & UINT16_C(0x3FFF));
  /* AD7689 configuration words are 14 bits, left-aligned on SDI. */
  config = (uint16_t)(config << 2U);
  tx[0] = (uint8_t)(config >> 8U);
  tx[1] = (uint8_t)config;
  device->bus.set_convert(device->bus.context, true);
  device->bus.delay_ns(device->bus.context, AD7689_CONVERT_PULSE_NS);
  device->bus.set_convert(device->bus.context, false);
  device->bus.delay_ns(device->bus.context, device->conversion_time_ns);
  if (!device->bus.transfer(device->bus.context, tx, rx, sizeof(tx)))
  {
    ++device->transfer_errors;
    return AD7689_BUS_ERROR;
  }
  *raw_code = (uint16_t)(((uint16_t)rx[0] << 8U) | rx[1]);
  device->latest_raw[channel] = *raw_code;
  ++device->sample_count;
  return AD7689_OK;
}
