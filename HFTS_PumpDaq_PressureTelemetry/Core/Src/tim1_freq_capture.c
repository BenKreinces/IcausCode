/**
 * @file tim1_freq_capture.c
 * @brief TIM1 CH1 input-capture frequency (DMA). CH1 only.
 */
#include "tim1_freq_capture.h"
#include "main.h"
#include <string.h>

extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_tim1_ch1;

#define CAP_BUF_LEN  128u
#define NUM_CH       1u
#define MASK_16      0xFFFFu
#define ALPHA        0.15f

__attribute__((aligned(32))) static uint32_t cap_buf[CAP_BUF_LEN];
static uint32_t last_cap;
static uint8_t  have_last;
static volatile float freq_hz;

static volatile uint8_t s_last_dma_channel = 0xFFu;

static uint32_t tim1_get_timclk_hz(void)
{
  return 200000000u;
}

static uint32_t tim1_get_counter_hz(void)
{
  uint32_t psc = (uint32_t)(htim1.Instance->PSC);
  return tim1_get_timclk_hz() / (psc + 1u);
}

static void process_captures(const uint32_t *buf, uint32_t n)
{
  float f_cnt = (float)tim1_get_counter_hz();
  float *p_freq = (float *)&freq_hz;

  for (uint32_t i = 0; i < n; i++)
  {
    uint32_t cap = buf[i] & MASK_16;
    if (!have_last)
    {
      last_cap = cap;
      have_last = 1;
      continue;
    }
    uint32_t delta = (cap - last_cap) & MASK_16;
    last_cap = cap;
    if (delta == 0) continue;
    float f = f_cnt / (float)delta;
    *p_freq = (freq_hz <= 0.0f) ? f : (1.0f - ALPHA) * (*p_freq) + ALPHA * f;
  }
}

void tim1_freq_capture_set_dma_channel(uint8_t ch_index)
{
  if (ch_index < NUM_CH)
    s_last_dma_channel = ch_index;
}

void start_tim1(void)
{
  if (HAL_TIM_Base_Start(&htim1) != HAL_OK)
    Error_Handler();
}

void start_ch1(void)
{
  memset((void *)cap_buf, 0, CAP_BUF_LEN * sizeof(uint32_t));
  freq_hz = 0.0f;
  have_last = 0;
  if (HAL_TIM_IC_Start_DMA(&htim1, TIM_CHANNEL_1, cap_buf, CAP_BUF_LEN) != HAL_OK)
    Error_Handler();
}

float get_freq_hz(uint8_t ch)
{
  if (ch != 1) return 0.0f;
  return (float)freq_hz;
}

void HAL_TIM_IC_CaptureHalfCpltCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM1 || s_last_dma_channel >= NUM_CH) return;
  process_captures(cap_buf, CAP_BUF_LEN / 2u);
}

void HAL_TIM_IC_CaptureCpltCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM1 || s_last_dma_channel >= NUM_CH) return;
  process_captures(&cap_buf[CAP_BUF_LEN / 2u], CAP_BUF_LEN / 2u);
}
