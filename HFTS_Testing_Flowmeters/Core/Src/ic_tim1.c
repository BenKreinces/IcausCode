#include "ic_tim1.h"

extern TIM_HandleTypeDef htim1;

typedef struct
{
  volatile uint16_t last_cap;
  volatile uint32_t period_ticks;
  volatile float    period_s;
  volatile float    freq_hz;
  volatile uint8_t  have_last;
  volatile uint32_t captures;   // debug: increments on each capture
} ic_meas_t;

static ic_meas_t s_ic1 = {0};
static ic_meas_t s_ic2 = {0};
static ic_meas_t s_ic3 = {0};
static ic_meas_t s_ic4 = {0};

volatile uint32_t g_tim1_ic_cb_count = 0;
volatile uint32_t g_tim1_ic_last_channel = 0;

/* If you see jitter/noise, increase this to ignore tiny deltas */
#define IC_MIN_DT_TICKS   1U

/* ================= Clock ================= */

static uint32_t tim1_kernel_clk_hz(void)
{
  uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
  uint32_t d2ppre2 =
    (RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) >> RCC_D2CFGR_D2PPRE2_Pos;

  /* APB prescaler != 1 => timer clock doubles */
  if (d2ppre2 >= 4U) return pclk2 * 2U;
  return pclk2;
}

uint32_t IC_TIM1_GetTickHz(void)
{
  uint32_t timclk = tim1_kernel_clk_hz();
  return timclk / ((uint32_t)TIM1->PSC + 1U);
}

static inline uint32_t delta16(uint16_t now, uint16_t last)
{
  return (uint32_t)((uint16_t)(now - last));
}

static ic_meas_t* get_meas(uint32_t ch)
{
  switch (ch)
  {
    case IC_CH1: return &s_ic1;
    case IC_CH2: return &s_ic2;
    case IC_CH3: return &s_ic3;
    case IC_CH4: return &s_ic4;
    default:     return 0;
  }
}

static uint32_t get_hal_channel(uint32_t ch)
{
  switch (ch)
  {
    case IC_CH1: return TIM_CHANNEL_1;
    case IC_CH2: return TIM_CHANNEL_2;
    case IC_CH3: return TIM_CHANNEL_3;
    case IC_CH4: return TIM_CHANNEL_4;
    default:     return 0;
  }
}

static void ic_reset(ic_meas_t *m)
{
  m->last_cap = 0;
  m->period_ticks = 0;
  m->period_s = 0.0f;
  m->freq_hz = 0.0f;
  m->have_last = 0;
  m->captures = 0;
}

/* Clear capture flags + overcapture flags for a channel */
static void clear_ch_flags(uint32_t hal_ch)
{
  switch (hal_ch)
  {
    case TIM_CHANNEL_1:
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1);
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1OF);
      break;
    case TIM_CHANNEL_2:
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC2);
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC2OF);
      break;
    case TIM_CHANNEL_3:
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC3);
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC3OF);
      break;
    case TIM_CHANNEL_4:
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC4);
      __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC4OF);
      break;
    default:
      break;
  }
}

void IC_TIM1_Start(uint32_t ch_mask)
{
  /* Ensure the counter is running (deterministic) */
  HAL_TIM_Base_Start(&htim1);

  for (uint32_t bit = IC_CH1; bit <= IC_CH4; bit <<= 1)
  {
    if (ch_mask & bit)
    {
    	ic_meas_t *m = get_meas(bit);
		uint32_t hal_ch = get_hal_channel(bit);
		if (!m || !hal_ch) continue;

		ic_reset(m);

		/* Clear capture + overcapture flags before starting */
		if (hal_ch == TIM_CHANNEL_1) { __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1); __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1OF); }
		if (hal_ch == TIM_CHANNEL_2) { __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC2); __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC2OF); }
		if (hal_ch == TIM_CHANNEL_3) { __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC3); __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC3OF); }
		if (hal_ch == TIM_CHANNEL_4) { __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC4); __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC4OF); }

		/* Start capture interrupt */
		HAL_TIM_IC_Start_IT(&htim1, hal_ch);
    }
  }
}

void IC_TIM1_Stop(uint32_t ch_mask)
{
  for (uint32_t bit = IC_CH1; bit <= IC_CH4; bit <<= 1)
  {
    if (ch_mask & bit)
    {
      uint32_t hal_ch = get_hal_channel(bit);
      if (hal_ch)
        HAL_TIM_IC_Stop_IT(&htim1, hal_ch);
    }
  }
}

/* ================= Capture callback ================= */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM1) return;
  g_tim1_ic_cb_count++;
  g_tim1_ic_last_channel = htim->Channel;
  uint16_t cap = 0;
  ic_meas_t *m = 0;

  switch (htim->Channel)
  {
    case HAL_TIM_ACTIVE_CHANNEL_1:
      cap = (uint16_t)__HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1);
      m = &s_ic1;
      break;

    case HAL_TIM_ACTIVE_CHANNEL_2:
      cap = (uint16_t)__HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_2);
      m = &s_ic2;
      break;

    case HAL_TIM_ACTIVE_CHANNEL_3:
      cap = (uint16_t)__HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_3);
      m = &s_ic3;
      break;

    case HAL_TIM_ACTIVE_CHANNEL_4:
      cap = (uint16_t)__HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_4);
      m = &s_ic4;
      break;

    default:
      return;
  }

  m->captures++;

  if (!m->have_last)
  {
    m->last_cap = cap;
    m->have_last = 1;
    return;
  }

  uint32_t dt = delta16(cap, m->last_cap);
  m->last_cap = cap;

  if (dt < IC_MIN_DT_TICKS) return;

  m->period_ticks = dt;

  float tick = (float)IC_TIM1_GetTickHz();
  m->period_s = (float)dt / tick;
  m->freq_hz  = tick / (float)dt;
}

/* ================= Getters ================= */

float IC_TIM1_GetFreqHz(uint32_t ch)
{
  ic_meas_t *m = get_meas(ch);
  if (!m || m->period_ticks == 0) return 0.0f;
  return m->freq_hz;
}

float IC_TIM1_GetPeriodS(uint32_t ch)
{
  ic_meas_t *m = get_meas(ch);
  if (!m || m->period_ticks == 0) return 0.0f;
  return m->period_s;
}

uint32_t IC_TIM1_GetPeriodTicks(uint32_t ch)
{
  ic_meas_t *m = get_meas(ch);
  if (!m) return 0;
  return m->period_ticks;
}
