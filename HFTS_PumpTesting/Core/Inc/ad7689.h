// ad7689.h
#pragma once
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *CNV_GPIO;
    uint16_t CNV_Pin;
    float vref_volts;     // 4.096f for your case
    uint32_t tconv_ns;    // conservative conversion time (ns) if you don't poll SDO
} AD7689_Handle;

void     AD7689_DWT_Init(void);
void     AD7689_DelayNs(uint32_t ns);
void     AD7689_DelayUs(uint32_t us);

HAL_StatusTypeDef AD7689_Init_IN0_IntRef4096(AD7689_Handle *dev);
HAL_StatusTypeDef AD7689_ReadRaw(AD7689_Handle *dev, uint16_t cfg14, uint16_t *out_code);
HAL_StatusTypeDef AD7689_ReadIN0_Raw(AD7689_Handle *dev, uint16_t *out_code);
HAL_StatusTypeDef AD7689_ReadIN1_Raw(AD7689_Handle *dev, uint16_t *out_code);
HAL_StatusTypeDef AD7689_ReadIN2_Raw(AD7689_Handle *dev, uint16_t *out_code);
static inline float AD7689_CodeToVolts_Unipolar16(const AD7689_Handle *dev, uint16_t code)
{
    return ((float)code) * (dev->vref_volts / 65536.0f);
}
