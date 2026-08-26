/**
  ******************************************************************************
  * @file    ad7124_ll.c
  * @brief   Low-level AD7124-8 SPI register access implementation
  ******************************************************************************
  */

#include "ad7124_ll.h"
#include <string.h>

#define AD7124_RESET_BYTES  8U   /* 64 SCLKs with DIN high */

static void cs_assert(const ad7124_ll_ctx_t *ctx)
{
  HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);
}

static void cs_deassert(const ad7124_ll_ctx_t *ctx)
{
  HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);
}

void ad7124_ll_cs_assert(const ad7124_ll_ctx_t *ctx)
{
  if (ctx) HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);
}

void ad7124_ll_cs_deassert(const ad7124_ll_ctx_t *ctx)
{
  if (ctx) HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);
}

HAL_StatusTypeDef ad7124_reset(const ad7124_ll_ctx_t *ctx)
{
  uint8_t dummy[AD7124_RESET_BYTES];
  HAL_StatusTypeDef ret;

  memset(dummy, 0xFF, sizeof(dummy));
  cs_assert(ctx);
  ret = HAL_SPI_Transmit(ctx->hspi, dummy, AD7124_RESET_BYTES, 100);
  cs_deassert(ctx);

  if (ret == HAL_OK) {
    HAL_Delay(1);  /* Allow ADC to complete reset */
  }
  return ret;
}

HAL_StatusTypeDef ad7124_write_reg(const ad7124_ll_ctx_t *ctx, uint8_t addr,
                                  const uint8_t *value, uint8_t nbytes)
{
  uint8_t cmd = ad7124_ll_cmd_byte(0, addr);  /* 0 = write */
  HAL_StatusTypeDef ret;

  if (ctx == NULL || value == NULL || nbytes == 0 || nbytes > 3)
    return HAL_ERROR;

  cs_assert(ctx);
  ret = HAL_SPI_Transmit(ctx->hspi, &cmd, 1, 100);
  if (ret != HAL_OK) {
    cs_deassert(ctx);
    return ret;
  }
  ret = HAL_SPI_Transmit(ctx->hspi, (uint8_t *)value, nbytes, 100);
  cs_deassert(ctx);
  return ret;
}

HAL_StatusTypeDef ad7124_read_reg(const ad7124_ll_ctx_t *ctx, uint8_t addr,
                                 uint8_t *buf, uint8_t nbytes)
{
  uint8_t cmd = ad7124_ll_cmd_byte(1, addr);  /* 1 = read */
  HAL_StatusTypeDef ret;

  if (ctx == NULL || buf == NULL || nbytes == 0 || nbytes > 4)
    return HAL_ERROR;

  cs_assert(ctx);
  ret = HAL_SPI_Transmit(ctx->hspi, &cmd, 1, 100);
  if (ret != HAL_OK) {
    cs_deassert(ctx);
    return ret;
  }
  ret = HAL_SPI_Receive(ctx->hspi, buf, nbytes, 100);
  cs_deassert(ctx);
  return ret;
}
