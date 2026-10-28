/**
 ******************************************************************************
 * @file    tim1_freq_capture.h
 * @brief   TIM1 input-capture frequency measurement (PE9=CH1).
 ******************************************************************************
 */

#ifndef TIM1_FREQ_CAPTURE_H
#define TIM1_FREQ_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TIM1_FREQ_CAPTURE_CH_1  1

void start_tim1(void);
void start_ch1(void);
float get_freq_hz(uint8_t ch);
void tim1_freq_capture_set_dma_channel(uint8_t ch_index);

#ifdef __cplusplus
}
#endif

#endif /* TIM1_FREQ_CAPTURE_H */
