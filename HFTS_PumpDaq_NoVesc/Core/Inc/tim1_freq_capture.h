/**
 * @file tim1_freq_capture.h
 * @brief TIM1 CH1 input-capture frequency measurement (PE9). DMA-based.
 * Requires: TIM1 input capture CH1, DMA for TIM1_CH1 (e.g. DMA1 Stream0),
 * and in DMA IRQ call tim1_freq_capture_set_dma_channel(0) before HAL_TIM_IRQHandler.
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

/** @param ch 1 for CH1. Returns frequency in Hz, or 0.0f if no valid measurement. */
float get_freq_hz(uint8_t ch);

/** Call at start of TIM1 CH1 DMA IRQ handler (ch_index 0 for CH1). */
void tim1_freq_capture_set_dma_channel(uint8_t ch_index);

#ifdef __cplusplus
}
#endif

#endif /* TIM1_FREQ_CAPTURE_H */
