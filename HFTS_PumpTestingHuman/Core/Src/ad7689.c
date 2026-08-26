/**
 * @file ad7689.c
 * @brief AD7689 8-channel (IN0-IN7) with rotating conversions.
 */
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

/* CFG: 14-bit, left-aligned in 16-bit. INCC=111 unipolar, INx=channel (0..7), REF=001 internal 4.096V. */
#define CFG_BASE  0x3C40u  /* overwrite=1, unipolar, full BW, internal ref, no seq, no readback */
static uint16_t cfg_for_channel(uint8_t ch)
{
    if (ch > 7) ch = 0;
    return (uint16_t)(CFG_BASE | (ch & 0x07u));
}

HAL_StatusTypeDef AD7689_ReadRaw(AD7689_Handle_t *dev, uint16_t cfg14, uint16_t *out_code)
{
    uint16_t tx = (uint16_t)((cfg14 & 0x3FFFu) << 2);
    uint16_t rx = 0;

    HAL_GPIO_WritePin(dev->CNV_GPIO, dev->CNV_Pin, GPIO_PIN_SET);
    AD7689_DelayNs(50);
    HAL_GPIO_WritePin(dev->CNV_GPIO, dev->CNV_Pin, GPIO_PIN_RESET);

    if (dev->tconv_ns)
        AD7689_DelayNs(dev->tconv_ns);

    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(dev->hspi, (uint8_t *)&tx, (uint8_t *)&rx, 1, 10);
    if (st != HAL_OK) return st;
    *out_code = rx;
    return HAL_OK;
}

HAL_StatusTypeDef AD7689_ReadNextChannel(AD7689_Handle_t *dev, uint16_t *out_code)
{
    uint8_t ch = dev->next_channel % AD7689_NUM_CHANNELS;
    uint16_t cfg = cfg_for_channel(ch);
    HAL_StatusTypeDef st = AD7689_ReadRaw(dev, cfg, out_code);
    if (st == HAL_OK) {
        dev->raw[ch] = *out_code;
        dev->next_channel = (ch + 1) % AD7689_NUM_CHANNELS;
    }
    return st;
}

HAL_StatusTypeDef AD7689_ReadChannelRaw(AD7689_Handle_t *dev, uint8_t ch, uint16_t *out_code)
{
    if (ch >= AD7689_NUM_CHANNELS) return HAL_ERROR;
    return AD7689_ReadRaw(dev, cfg_for_channel(ch), out_code);
}

HAL_StatusTypeDef AD7689_Init(AD7689_Handle_t *dev)
{
    if (!dev || !dev->hspi) return HAL_ERROR;
    dev->vref_volts = 4.096f;
    if (dev->tconv_ns == 0)
        dev->tconv_ns = 5000;
    dev->next_channel = 0;

    uint16_t dummy;
    for (int i = 0; i < 4; i++) {
        if (AD7689_ReadRaw(dev, cfg_for_channel(0), &dummy) != HAL_OK)
            return HAL_ERROR;
    }
    return HAL_OK;
}
