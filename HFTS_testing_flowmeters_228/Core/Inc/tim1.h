/**
  ******************************************************************************
  * @file    tim1.h
  * @brief   TIM1 input-capture frequency helper API.
  ******************************************************************************
  */

#ifndef __TIM1_H
#define __TIM1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void TIM1_Freq_Init(void);
float TIM1_Freq_GetHz(void);
void TIM1_Freq_SetTimeoutMs(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __TIM1_H */

