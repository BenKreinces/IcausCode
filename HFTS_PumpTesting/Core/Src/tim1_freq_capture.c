/**
 ******************************************************************************
 * @file    tim1_freq_capture.c
 * @brief   TIM1 input-capture frequency measurement (DMA, CH1).
 ******************************************************************************
 */

#include "tim1_freq_capture.h"
#include "main.h"
#include <string.h>

extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_tim1_ch1;

#define CAP_BUF_LEN  128u
#define NUM_CH       4
#define MASK_16      0xFFFFu
#define ALPHA        0.15f

__attribute__((aligned(32))) static uint32_t cap_buf[NUM_CH][CAP_BUF_LEN];
static uint32_t last_cap[NUM_CH];
static uint8_t  have_last[NUM_CH];
static volatile float freq_hz[NUM_CH];
static volatile uint8_t s_last_dma_channel = 0xFFu;

static uint32_t tim1_get_timclk_hz(void) { return 200000000u; }
static uint32_t tim1_get_counter_hz(void)
{
  uint32_t psc = __HAL_TIM_GET_PRESCALER(&htim1);
  return tim1_get_timclk_hz() / (psc + 1u);
}

static void process_captures(uint8_t ch, const uint32_t *buf, uint32_t n)
{
  float f_cnt = (float)tim1_get_counter_hz();
  uint32_t *p_last = &last_cap[ch];
  uint8_t *p_have = &have_last[ch];
  float *p_freq = (float *)&freq_hz[ch];
  for (uint32_t i = 0; i < n; i++) {
    uint32_t cap = buf[i] & MASK_16;
    if (!*p_have) { *p_last = cap; *p_have = 1; continue; }
    uint32_t delta = (cap - *p_last) & MASK_16;
    *p_last = cap;
    if (delta == 0) continue;
    float f = f_cnt / (float)delta;
    *p_freq = (*p_freq <= 0.0f) ? f : (1.0f - ALPHA) * (*p_freq) + ALPHA * f;
  }
}

void tim1_freq_capture_set_dma_channel(uint8_t ch_index)
{
  if (ch_index < NUM_CH) s_last_dma_channel = ch_index;
}

void start_tim1(void)
{
  if (HAL_TIM_Base_Start(&htim1) != HAL_OK) Error_Handler();
}

static void start_ch(uint8_t ch_index, uint32_t tim_channel, uint32_t *buf)
{
  if (ch_index >= NUM_CH) return;
  memset((void *)buf, 0, CAP_BUF_LEN * sizeof(uint32_t));
  freq_hz[ch_index] = 0.0f;
  have_last[ch_index] = 0;
  if (HAL_TIM_IC_Start_DMA(&htim1, tim_channel, buf, CAP_BUF_LEN) != HAL_OK)
    Error_Handler();
}

void start_ch1(void) { start_ch(0, TIM_CHANNEL_1, cap_buf[0]); }

float get_freq_hz(uint8_t ch)
{
  if (ch < 1 || ch > NUM_CH) return 0.0f;
  return (float)freq_hz[ch - 1];
}

void HAL_TIM_IC_CaptureHalfCpltCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM1 || s_last_dma_channel >= NUM_CH) return;
  uint8_t ch = s_last_dma_channel;
  process_captures(ch, cap_buf[ch], CAP_BUF_LEN / 2u);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM1 || s_last_dma_channel >= NUM_CH) return;
  uint8_t ch = s_last_dma_channel;
  process_captures(ch, &cap_buf[ch][CAP_BUF_LEN / 2u], CAP_BUF_LEN / 2u);
}
