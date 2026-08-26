// ad7689.c
#include "ad7689.h"

void AD7689_DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void AD7689_DelayNs(uint32_t ns)
{
    uint32_t cycles = (uint32_t)(((uint64_t)SystemCoreClock * (uint64_t)ns) / 1000000000ULL);
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles) { __NOP(); }
}

void AD7689_DelayUs(uint32_t us)
{
    uint32_t cycles = (uint32_t)(((uint64_t)SystemCoreClock * (uint64_t)us) / 1000000ULL);
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles) { __NOP(); }
}

HAL_StatusTypeDef AD7689_ReadRaw(AD7689_Handle *dev, uint16_t cfg14, uint16_t *out_code)
{
    uint16_t tx = (uint16_t)((cfg14 & 0x3FFFu) << 2);
    uint16_t rx = 0;

    HAL_GPIO_WritePin(dev->CNV_GPIO, dev->CNV_Pin, GPIO_PIN_SET);
    AD7689_DelayNs(50);
    HAL_GPIO_WritePin(dev->CNV_GPIO, dev->CNV_Pin, GPIO_PIN_RESET);

    if (dev->tconv_ns) {
        AD7689_DelayNs(dev->tconv_ns);
    }

    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(dev->hspi,
                                                  (uint8_t*)&tx,
                                                  (uint8_t*)&rx,
                                                  1,
                                                  10);
    if (st != HAL_OK) return st;

    *out_code = rx;
    return HAL_OK;
}

HAL_StatusTypeDef AD7689_ReadIN0_Raw(AD7689_Handle *dev, uint16_t *out_code)
{
    const uint16_t CFG_IN0_INT4096 = 0x3C49;
    return AD7689_ReadRaw(dev, CFG_IN0_INT4096, out_code);
}

HAL_StatusTypeDef AD7689_ReadIN1_Raw(AD7689_Handle *dev, uint16_t *out_code)
{
    const uint16_t CFG_IN1_INT4096 = 0x3C4A;
    return AD7689_ReadRaw(dev, CFG_IN1_INT4096, out_code);
}

HAL_StatusTypeDef AD7689_ReadIN2_Raw(AD7689_Handle *dev, uint16_t *out_code)
{
    const uint16_t CFG_IN2_INT4096 = 0x3C4B;
    return AD7689_ReadRaw(dev, CFG_IN2_INT4096, out_code);
}

HAL_StatusTypeDef AD7689_Init_IN0_IntRef4096(AD7689_Handle *dev)
{
    dev->vref_volts = 4.096f;
    uint16_t dummy = 0;
    HAL_StatusTypeDef st;

    for (int i = 0; i < 3; i++) {
        st = AD7689_ReadIN0_Raw(dev, &dummy);
        if (st != HAL_OK) return st;
    }
    return HAL_OK;
}
