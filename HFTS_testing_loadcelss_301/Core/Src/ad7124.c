/**
  ******************************************************************************
  * @file    ad7124.c
  * @brief   Mid-level AD7124-8 configuration API implementation
  ******************************************************************************
  */

#include "ad7124.h"
#include <string.h>

/* CHANNEL_0..CHANNEL_15 base address */
#define AD7124_CHANNEL_BASE  0x09U

/* Disabled channel value: Enable=0, Setup=0, AINP=0, AINM=1 */
#define AD7124_CH_DISABLED   0x0001U

/* CHANNEL_0 for AIN2/AIN3 diff: Enable=1, Setup=0, AINP=2, AINM=3 */
#define AD7124_CH0_AIN2_AIN3 0x8043U

/* CONFIG_0: REF_SEL=00 (REFIN1), other defaults (reset = 0x0860) */
#define AD7124_CONFIG0_REFIN1 0x0860U

/* ADC_CONTROL: CONT_READ=1, DATA_STATUS=1, Mode=0000 (continuous) */
#define AD7124_ADC_CTRL_CONT_READ_STATUS 0x0C00U

/* Prime continuous read: read DATA register, cmd = 0x42 */
#define AD7124_CMD_READ_DATA  0x42U

static volatile ad7124_sample_t s_latest_sample;
static ad7124_sample_cb_t s_sample_cb;
static void *s_sample_cb_user;
static ad7124_ll_ctx_t *s_ctx;  /* For ISR to access SPI/CS */

void ad7124_set_sample_callback(ad7124_sample_cb_t cb, void *user)
{
  s_sample_cb = cb;
  s_sample_cb_user = user;
}

void ad7124_get_latest_sample(ad7124_sample_t *out)
{
  if (out) {
    out->data24 = s_latest_sample.data24;
    out->status = s_latest_sample.status;
  }
}

void ad7124_parse_sample(const uint8_t *rx, ad7124_sample_t *sample)
{
  if (sample == NULL || rx == NULL) return;
  /* RX: [DATA_MSB, DATA_MID, DATA_LSB, STATUS] - MSB first, right-aligned */
  sample->data24 = ((uint32_t)rx[0] << 16) | ((uint32_t)rx[1] << 8) | rx[2];
  sample->status = rx[3];
}

int ad7124_init(ad7124_ll_ctx_t *ctx)
{
  uint8_t id_buf[1];
  HAL_StatusTypeDef ret;

  if (ctx == NULL) return -2;
  s_ctx = ctx;

  ret = ad7124_reset(ctx);
  if (ret != HAL_OK) return -2;

  HAL_Delay(2);  /* Allow ADC to stabilize after reset */

  ret = ad7124_read_reg(ctx, AD7124_REG_ID, id_buf, 1);
  if (ret != HAL_OK) return -2;
  if (id_buf[0] != AD7124_ID_EXPECTED) return -1;

  return 0;
}

int ad7124_disable_all_channels(ad7124_ll_ctx_t *ctx)
{
  uint8_t val[2];
  HAL_StatusTypeDef ret;

  if (ctx == NULL) return -1;

  val[0] = (uint8_t)(AD7124_CH_DISABLED >> 8);
  val[1] = (uint8_t)(AD7124_CH_DISABLED & 0xFF);

  for (uint8_t ch = 0; ch <= 15; ch++) {
    ret = ad7124_write_reg(ctx, AD7124_CHANNEL_BASE + ch, val, 2);
    if (ret != HAL_OK) return -1;
  }
  return 0;
}

int ad7124_enable_channel_ain2_ain3_diff(ad7124_ll_ctx_t *ctx)
{
  uint8_t val[2];
  HAL_StatusTypeDef ret;

  if (ctx == NULL) return -1;

  val[0] = (uint8_t)(AD7124_CH0_AIN2_AIN3 >> 8);
  val[1] = (uint8_t)(AD7124_CH0_AIN2_AIN3 & 0xFF);
  ret = ad7124_write_reg(ctx, AD7124_REG_CHANNEL_0, val, 2);
  return (ret == HAL_OK) ? 0 : -1;
}

int ad7124_use_external_refin1(ad7124_ll_ctx_t *ctx)
{
  uint8_t val[2];
  HAL_StatusTypeDef ret;

  if (ctx == NULL) return -1;

  val[0] = (uint8_t)(AD7124_CONFIG0_REFIN1 >> 8);
  val[1] = (uint8_t)(AD7124_CONFIG0_REFIN1 & 0xFF);
  ret = ad7124_write_reg(ctx, AD7124_REG_CONFIG_0, val, 2);
  return (ret == HAL_OK) ? 0 : -1;
}

int ad7124_set_continuous_conversion_and_read_with_status(ad7124_ll_ctx_t *ctx)
{
  uint8_t val[2];
  uint8_t cmd = AD7124_CMD_READ_DATA;
  HAL_StatusTypeDef ret;

  if (ctx == NULL) return -1;

  val[0] = (uint8_t)(AD7124_ADC_CTRL_CONT_READ_STATUS >> 8);
  val[1] = (uint8_t)(AD7124_ADC_CTRL_CONT_READ_STATUS & 0xFF);
  ret = ad7124_write_reg(ctx, AD7124_REG_ADC_CONTROL, val, 2);
  if (ret != HAL_OK) return -1;

  /* Prime continuous read: CS low, send 0x42 (read DATA), no data bytes, CS high */
  ad7124_ll_cs_assert(ctx);
  ret = HAL_SPI_Transmit(ctx->hspi, &cmd, 1, 100);
  ad7124_ll_cs_deassert(ctx);
  return (ret == HAL_OK) ? 0 : -1;
}

void ad7124_start(ad7124_ll_ctx_t *ctx)
{
  (void)ctx;
  /* Already in continuous mode after set_continuous_conversion_and_read_with_status */
}

void ad7124_stop(ad7124_ll_ctx_t *ctx)
{
  (void)ctx;
  /* Would require writing ADC_CONTROL with Mode != 0000 */
}

HAL_StatusTypeDef ad7124_read_sample_isr(uint8_t *rx)
{
  uint8_t tx[4] = {0, 0, 0, 0};
  HAL_StatusTypeDef ret;

  if (s_ctx == NULL || rx == NULL) return HAL_ERROR;

  ad7124_ll_cs_assert(s_ctx);
  ret = HAL_SPI_TransmitReceive(s_ctx->hspi, tx, rx, 4, 50);
  ad7124_ll_cs_deassert(s_ctx);

  if (ret == HAL_OK) {
    ad7124_parse_sample(rx, (ad7124_sample_t *)&s_latest_sample);
    if (s_sample_cb) {
      s_sample_cb((const ad7124_sample_t *)&s_latest_sample, s_sample_cb_user);
    }
  }
  return ret;
}
