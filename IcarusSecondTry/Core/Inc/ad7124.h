/**
 * @file ad7124.h
 * @brief Bus-independent AD7124-8 configuration and sample driver.
 *
 * This driver owns the AD7124 register protocol and raw conversion samples.
 * It does not know about STM32 HAL, FreeRTOS, load-cell units, calibration,
 * or application policy.
 * A caller supplies the SPI/CS operations and must give one task ownership of
 * each AD7124 instance.
 */
#ifndef AD7124_H
#define AD7124_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  AD7124_REG_STATUS       = 0x00U,
  AD7124_REG_ADC_CONTROL  = 0x01U,
  AD7124_REG_DATA         = 0x02U,
  AD7124_REG_ID           = 0x05U,
  AD7124_REG_ERROR        = 0x06U,
  AD7124_REG_ERROR_ENABLE = 0x07U,
  AD7124_REG_CHANNEL_0    = 0x09U,
  AD7124_REG_CHANNEL_15   = 0x18U,
  AD7124_REG_CONFIG_0     = 0x19U,
  AD7124_REG_FILTER_0     = 0x21U,
  AD7124_ID_EXPECTED      = 0x17U
};

typedef enum
{
  AD7124_RESULT_OK = 0,
  AD7124_RESULT_ARGUMENT,
  AD7124_RESULT_BUS_ERROR,
  AD7124_RESULT_ID_MISMATCH,
  AD7124_RESULT_NOT_READY,
  AD7124_RESULT_CONFIGURATION_MISMATCH
} ad7124_result_t;

/**
 * @brief Hardware operations required by the register protocol.
 *
 * transfer() must complete exactly @p length full-duplex SPI bytes within
 * @p timeout_ms and return true only on success. The caller owns the buffer
 * storage for the duration of the call. set_chip_select(true) asserts CS.
 */
typedef struct
{
  void *context;
  void (*set_chip_select)(void *context, bool asserted);
  bool (*transfer)(void *context,
                   const uint8_t *tx,
                   uint8_t *rx,
                   size_t length,
                   uint32_t timeout_ms);
  void (*delay_ms)(void *context, uint32_t delay_ms);
} ad7124_bus_t;

typedef struct
{
  ad7124_bus_t bus;
  uint32_t transfer_timeout_ms;
  uint32_t reset_count;
  uint32_t register_read_count;
  uint32_t register_write_count;
  uint32_t sample_read_count;
  uint32_t not_ready_count;
  uint32_t transport_error_count;
  uint32_t id_mismatch_count;
  uint32_t configuration_mismatch_count;
} ad7124_t;

/** Read-only diagnostic snapshot. All multi-byte register values are big endian. */
typedef struct
{
  uint8_t status;
  uint8_t id;
  uint32_t error;
  uint16_t adc_control;
  uint16_t channel_0;
  uint16_t config_0;
  uint32_t filter_0;
} ad7124_diagnostics_t;

/**
 * Raw configuration for one differential channel using setup 0.
 *
 * adc_control, config_0, and filter_0 are literal AD7124 register values.
 * Keeping them explicit avoids burying unverified board assumptions in the
 * device driver. input_positive and input_negative are AIN numbers 0 through
 * 15; setup must be 0 through 7.
 */
typedef struct
{
  uint16_t adc_control;
  uint16_t config_0;
  uint32_t filter_0;
  uint8_t setup;
  uint8_t input_positive;
  uint8_t input_negative;
} ad7124_single_channel_config_t;

/** A conversion sample in native ADC code; no engineering-unit conversion. */
typedef struct
{
  uint32_t raw_code; /**< 24-bit offset-binary DATA register value. */
  int32_t signed_code; /**< raw_code relative to midscale (0x800000). */
  uint8_t status; /**< STATUS read immediately before DATA. */
} ad7124_sample_t;

/** Validate and store the bus callbacks. No transaction occurs. */
ad7124_result_t AD7124_Init(ad7124_t *device,
                             const ad7124_bus_t *bus,
                             uint32_t transfer_timeout_ms);

/** Clock 64 one bits, resetting the AD7124 interface and registers to defaults. */
ad7124_result_t AD7124_Reset(ad7124_t *device);

/** Read one 1-, 2-, or 3-byte register value, MSB first. */
ad7124_result_t AD7124_ReadRegister(ad7124_t *device,
                                     uint8_t address,
                                     uint8_t *value,
                                     size_t value_length);

/** Write one 1-, 2-, or 3-byte register value, MSB first. */
ad7124_result_t AD7124_WriteRegister(ad7124_t *device,
                                      uint8_t address,
                                      const uint8_t *value,
                                      size_t value_length);

/** Reset the device and confirm the immutable AD7124-8 ID register is 0x17. */
ad7124_result_t AD7124_Probe(ad7124_t *device, uint8_t *observed_id);

/** Read status and selected configuration/error registers without writing them. */
ad7124_result_t AD7124_ReadDiagnostics(ad7124_t *device,
                                        ad7124_diagnostics_t *diagnostics);

/**
 * Reset, identify, configure, and read back a single differential channel.
 * All 16 channel registers are explicitly disabled before channel 0 is
 * enabled, so reset state cannot leave an unintended input converting.
 */
ad7124_result_t AD7124_ConfigureSingleChannel(
    ad7124_t *device,
    const ad7124_single_channel_config_t *configuration);

/**
 * Return one new raw sample if STATUS reports data ready. This never waits for
 * data-ready; AD7124_RESULT_NOT_READY is the normal polling result.
 */
ad7124_result_t AD7124_ReadSample(ad7124_t *device, ad7124_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* AD7124_H */
