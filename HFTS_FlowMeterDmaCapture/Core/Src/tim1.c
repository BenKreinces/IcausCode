#include "tim1.h"

extern TIM_HandleTypeDef htim1;

/* TIM1 prescaler is 399, counter ticks at 1 MHz (APB2 timers 200 MHz * 2). */
#define TIM1_COUNTER_HZ 1000000u

#define MASK 0xFFFFu   /* TIM1 is 16-bit */
#define ALPHA 0.15f

volatile float g_tim1_freq_hz = 0.0f;
static uint32_t last_cap = 0;
static uint8_t have_last = 0;
static volatile uint32_t s_last_edge_ms = 0;
static volatile uint32_t s_timeout_ms = 250; /* default: 250 ms */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM1)
    return;

  uint32_t cap = (uint32_t)__HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1);
  cap &= MASK;
  s_last_edge_ms = HAL_GetTick();

  if (!have_last)
  {
    last_cap = cap;
    have_last = 1;
    return;
  }

  uint32_t delta = (cap - last_cap) & MASK;
  last_cap = cap;
  if (delta == 0)
    return;

  float f = (float)TIM1_COUNTER_HZ / (float)delta;
  g_tim1_freq_hz = (g_tim1_freq_hz <= 0.0f) ? f : (1.0f - ALPHA) * g_tim1_freq_hz + ALPHA * f;
}

void TIM1_Freq_Init(void)
{
  g_tim1_freq_hz = 0.0f;
  have_last = 0;
  last_cap = 0;
  s_last_edge_ms = HAL_GetTick();

  if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1) != HAL_OK)
    Error_Handler();
}

float TIM1_Freq_GetHz(void)
{
  uint32_t now = HAL_GetTick();
  if ((uint32_t)(now - s_last_edge_ms) > s_timeout_ms)
  {
    g_tim1_freq_hz = 0.0f;
    have_last = 0;
  }
  return g_tim1_freq_hz;
}

void TIM1_Freq_SetTimeoutMs(uint32_t timeout_ms)
{
  s_timeout_ms = (timeout_ms == 0u) ? 1u : timeout_ms;
}
