/** @file ad7689.h @brief Bounded AD7689 conversion driver. */
#ifndef ICARUS_AD7689_H
#define ICARUS_AD7689_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AD7689_CHANNEL_COUNT UINT8_C(8)

typedef bool (*ad7689_transfer_fn)(void *context, const uint8_t *tx,
                                   uint8_t *rx, size_t length);
typedef void (*ad7689_convert_fn)(void *context, bool asserted);
typedef void (*ad7689_delay_ns_fn)(void *context, uint32_t nanoseconds);

typedef enum
{
  AD7689_OK = 0,
  AD7689_ARGUMENT_ERROR,
  AD7689_BUS_ERROR,
  AD7689_RANGE_ERROR
} ad7689_result_t;

typedef struct
{
  void *context;
  ad7689_transfer_fn transfer;
  ad7689_convert_fn set_convert;
  ad7689_delay_ns_fn delay_ns;
} ad7689_bus_t;

typedef struct
{
  ad7689_bus_t bus;
  uint16_t channel_config[AD7689_CHANNEL_COUNT];
  uint32_t conversion_time_ns;
  uint16_t latest_raw[AD7689_CHANNEL_COUNT];
  uint32_t sample_count;
  uint32_t transfer_errors;
} ad7689_t;

ad7689_result_t AD7689_Init(ad7689_t *device, const ad7689_bus_t *bus,
                             const uint16_t channel_config[AD7689_CHANNEL_COUNT],
                             uint32_t conversion_time_ns);
ad7689_result_t AD7689_ReadChannel(ad7689_t *device, uint8_t channel,
                                    uint16_t *raw_code);

#endif /* ICARUS_AD7689_H */
