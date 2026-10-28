/**
 * @file ad7689.h
 * @brief AD7689 8-channel ADC (IN0-IN7), internal ref 4.096V, raw codes.
 * Rotating frame: one conversion per channel per call; use next_channel to cycle 0..7.
 */
#ifndef AD7689_H
#define AD7689_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define AD7689_NUM_CHANNELS 8

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *CNV_GPIO;
    uint16_t CNV_Pin;
    float vref_volts;
    uint32_t tconv_ns;
    uint8_t next_channel;  /* 0..7 rotating */
    uint16_t raw[AD7689_NUM_CHANNELS]; /* last raw code per channel */
} AD7689_Handle_t;

void AD7689_DWT_Init(void);
void AD7689_DelayNs(uint32_t ns);

HAL_StatusTypeDef AD7689_Init(AD7689_Handle_t *dev);
/** Run one conversion for the current rotating channel; advance next_channel. */
HAL_StatusTypeDef AD7689_ReadNextChannel(AD7689_Handle_t *dev, uint16_t *out_code);
/** Read one channel by index (0..7). Does not advance rotation. */
HAL_StatusTypeDef AD7689_ReadChannelRaw(AD7689_Handle_t *dev, uint8_t ch, uint16_t *out_code);

static inline float AD7689_CodeToVolts_Unipolar16(const AD7689_Handle_t *dev, uint16_t code)
{
    return ((float)code) * (dev->vref_volts / 65536.0f);
}

#endif /* AD7689_H */
