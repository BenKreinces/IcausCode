/**
  ******************************************************************************
  * @file    ad7124.h
  * @brief   Mid-level AD7124-8 configuration API
  ******************************************************************************
  */

#ifndef AD7124_H
#define AD7124_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ad7124_ll.h"
#include <stdint.h>
#include <stdbool.h>

/* AD7124 sample with 24-bit data and STATUS byte */
typedef struct {
  uint32_t data24;   /* 24-bit ADC result, right-aligned (bits 23:0) */
  uint8_t  status;  /* STATUS register byte when DATA_STATUS=1 */
} ad7124_sample_t;

/* Optional callback when a new sample is ready (called from ISR context) */
typedef void (*ad7124_sample_cb_t)(const ad7124_sample_t *sample, void *user);

/**
 * @brief Initialize AD7124 driver context and verify ID.
 * @param ctx Low-level context (must be populated with hspi2, CS port/pin)
 * @return 0 on success, -1 on ID mismatch, -2 on reset/init error
 */
int ad7124_init(ad7124_ll_ctx_t *ctx);

/**
 * @brief Disable all channels (CHANNEL_0..CHANNEL_15, bit15=0).
 *        Uses 0x0001 for each channel.
 */
int ad7124_disable_all_channels(ad7124_ll_ctx_t *ctx);

/**
 * @brief Enable CHANNEL_0 with AIN2/AIN3 differential, Setup 0.
 *        Value: 0x8043 (Enable=1, Setup=0, AINP=AIN2, AINM=AIN3)
 */
int ad7124_enable_channel_ain2_ain3_diff(ad7124_ll_ctx_t *ctx);

/**
 * @brief Configure CONFIG_0 to use REFIN1(+)/REFIN1(-) (REF_SEL=00).
 *        Writes 0x0860 (reset default with REF_SEL=00).
 */
int ad7124_use_external_refin1(ad7124_ll_ctx_t *ctx);

/**
 * @brief Set ADC_CONTROL: CONT_READ=1, DATA_STATUS=1, continuous conversion.
 *        Writes 0x0C00, then primes continuous read with command 0x42.
 */
int ad7124_set_continuous_conversion_and_read_with_status(ad7124_ll_ctx_t *ctx);

/**
 * @brief Start conversions (no-op if already in continuous mode).
 */
void ad7124_start(ad7124_ll_ctx_t *ctx);

/**
 * @brief Stop conversions (optional; would require mode change).
 */
void ad7124_stop(ad7124_ll_ctx_t *ctx);

/**
 * @brief Set sample callback (called from EXTI ISR when new sample ready).
 * @param cb Callback function (NULL to disable)
 * @param user User pointer passed to callback
 */
void ad7124_set_sample_callback(ad7124_sample_cb_t cb, void *user);

/**
 * @brief Get latest sample (volatile, updated by ISR).
 */
void ad7124_get_latest_sample(ad7124_sample_t *out);

/**
 * @brief Process raw 4-byte read into sample struct.
 *        RX: [DATA_MSB, DATA_MID, DATA_LSB, STATUS]
 *        data24 is right-aligned: (msb<<16)|(mid<<8)|lsb
 */
void ad7124_parse_sample(const uint8_t *rx, ad7124_sample_t *sample);

/**
 * @brief Read one sample from DATA register (4 bytes). Call from EXTI ISR.
 *        Uses TransmitReceive with dummy TX to clock out DATA+STATUS.
 * @param rx Buffer for 4 bytes [DATA_MSB, DATA_MID, DATA_LSB, STATUS]
 * @return HAL status
 */
HAL_StatusTypeDef ad7124_read_sample_isr(uint8_t *rx);

#ifdef __cplusplus
}
#endif

#endif /* AD7124_H */
