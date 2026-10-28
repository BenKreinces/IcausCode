#ifndef IC_TIM1_H
#define IC_TIM1_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define IC_CH1   (1U << 0)
#define IC_CH2   (1U << 1)
#define IC_CH3   (1U << 2)
#define IC_CH4   (1U << 3)

void     IC_TIM1_Start(uint32_t ch_mask);
void     IC_TIM1_Stop(uint32_t ch_mask);

float    IC_TIM1_GetFreqHz(uint32_t ch_mask);
float    IC_TIM1_GetPeriodS(uint32_t ch_mask);
uint32_t IC_TIM1_GetPeriodTicks(uint32_t ch_mask);
uint32_t IC_TIM1_GetTickHz(void);

#endif
