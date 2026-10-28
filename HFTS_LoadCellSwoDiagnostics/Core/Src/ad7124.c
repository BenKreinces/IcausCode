/**
 * @file ad7124.c
 * @brief AD7124-8 SPI driver: differential AIN2/AIN3, ~1 kHz, gain 1.
 */

#include "ad7124.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/* AD7124 register addresses (6-bit; first byte = [0,0,R/W,A5..A0]) */
#define AD7124_REG_STATUS        0x00U
#define AD7124_REG_ADC_CONTROL   0x01U
#define AD7124_REG_DATA          0x02U
#define AD7124_REG_IO_CTRL1      0x03U
#define AD7124_REG_CHANNEL_0     0x09U
#define AD7124_REG_CONFIG_0      0x19U
#define AD7124_REG_FILTER_0      0x21U

#define AD7124_COM_READ(addr)   (0x40U | ((addr) & 0x3FU))
#define AD7124_COM_WRITE(addr) ((addr) & 0x3FU)

/* ADC_Control: continuous, full power, internal ref enable */
#define AD7124_ADC_CTRL_CONT_FULL_REF  0x20U  /* ref_en=1, mode=continuous, power=full */

/* Channel 0: enable, setup 0, AIN2+ (positive), AIN3- (negative). Result 0x8023. */
#define AD7124_CH0_EN    (1u << 15)
#define AD7124_CH0_AINP  2u  /* AIN2 */
#define AD7124_CH0_AINM  3u  /* AIN3 */
#define AD7124_CH0_VAL   (AD7124_CH0_EN | (0u << 10) | (AD7124_CH0_AINP << 6) | (AD7124_CH0_AINM << 2))

/* Config_0: internal ref, gain 1, bipolar */
#define AD7124_CFG_REF_INTERNAL  (1u << 5)
#define AD7124_CFG_GAIN_1       0u
#define AD7124_CFG_0_VAL        (AD7124_CFG_REF_INTERNAL | AD7124_CFG_GAIN_1)

/* Filter_0: Sinc4, FS for ~1 kHz. ODR = f_CLK/(4*FS), f_CLK = 614.4 kHz => FS = 154 */
#define AD7124_FILTER_SINC4  0u
#define AD7124_FILTER_FS_1K  154u
#define AD7124_FILTER_0_VAL  ((AD7124_FILTER_SINC4 << 12) | (AD7124_FILTER_FS_1K & 0x7FFu))

/* Internal reference voltage (V) for voltage conversion */
#define AD7124_VREF 2.5f

static bool ad7124_write_reg(ad7124_dev_t *dev, uint8_t addr, const uint8_t *data, uint8_t len)
{
    if (!dev || !dev->spi_xfer || len > 4) return false;
    uint8_t tx[5], rx[5];
    tx[0] = AD7124_COM_WRITE(addr);
    for (uint8_t i = 0; i < len; i++) tx[i + 1] = data[i];
    memset(rx, 0, sizeof(rx));
    return dev->spi_xfer(dev->user, tx, rx, (uint16_t)(len + 1));
}

static bool ad7124_read_reg(ad7124_dev_t *dev, uint8_t addr, uint8_t *data, uint8_t len)
{
    if (!dev || !dev->spi_xfer || !data || len > 4) return false;
    uint8_t tx[5], rx[5];
    tx[0] = AD7124_COM_READ(addr);
    for (uint8_t i = 1; i < 5; i++) tx[i] = 0u;
    memset(rx, 0, sizeof(rx));
    if (!dev->spi_xfer(dev->user, tx, rx, (uint16_t)(len + 1))) return false;
    for (uint8_t i = 0; i < len; i++) data[i] = rx[i + 1];
    return true;
}

bool ad7124_init(ad7124_dev_t *dev)
{
    if (!dev || !dev->spi_xfer) return false;

    uint8_t buf[3];

    /* ADC_Control: continuous conversion, full power, internal ref on */
    buf[0] = AD7124_ADC_CTRL_CONT_FULL_REF;
    if (!ad7124_write_reg(dev, AD7124_REG_ADC_CONTROL, buf, 1)) return false;

    /* Channel 0: AIN2+ / AIN3- differential, setup 0 */
    buf[0] = (uint8_t)(AD7124_CH0_VAL >> 8);
    buf[1] = (uint8_t)(AD7124_CH0_VAL & 0xFFu);
    if (!ad7124_write_reg(dev, AD7124_REG_CHANNEL_0, buf, 2)) return false;

    /* Config_0: internal ref, gain 1 */
    buf[0] = (uint8_t)(AD7124_CFG_0_VAL >> 8);
    buf[1] = (uint8_t)(AD7124_CFG_0_VAL & 0xFFu);
    if (!ad7124_write_reg(dev, AD7124_REG_CONFIG_0, buf, 2)) return false;

    /* Filter_0: Sinc4, FS = 154 (~1 kHz) */
    buf[0] = (uint8_t)(AD7124_FILTER_0_VAL >> 8);
    buf[1] = (uint8_t)(AD7124_FILTER_0_VAL & 0xFFu);
    if (!ad7124_write_reg(dev, AD7124_REG_FILTER_0, buf, 2)) return false;

    return true;
}

bool ad7124_read_data(ad7124_dev_t *dev, uint32_t *code)
{
    if (!dev || !code) return false;
    uint8_t buf[3];
    if (!ad7124_read_reg(dev, AD7124_REG_DATA, buf, 3)) return false;
    *code = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];
    return true;
}

float ad7124_code_to_voltage(uint32_t code)
{
    /* 24-bit offset binary: 0 = -VREF, 0x800000 = 0, 0xFFFFFF ≈ +VREF */
    int32_t s = (int32_t)code - 0x800000;
    return (float)s * (AD7124_VREF / (float)0x800000);
}

void ad7124_swo_init(void)
{
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7M__) || defined(__CORE_CM7_H_GENERIC)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* enable trace */
    ITM->TCR  = 1u;                                   /* enable ITM */
    ITM->TER  = 1u;                                   /* enable stimulus port 0 */
    ITM->TPR  = 0x0000000Fu;                         /* allow all privilege levels to write */
#endif
}

void ad7124_swo_send_u32(uint32_t value)
{
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7M__) || defined(__CORE_CM7_H_GENERIC)
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0) return;
    if ((ITM->PORT[0].u32 & 1u) == 0) return;
    ITM->PORT[0].u8 = (uint8_t)(value >> 0);
    ITM->PORT[0].u8 = (uint8_t)(value >> 8);
    ITM->PORT[0].u8 = (uint8_t)(value >> 16);
    ITM->PORT[0].u8 = (uint8_t)(value >> 24);
#else
    (void)value;
#endif
}

/* Print string to SWO (ITM port 0). Use for debug on PB3 / TRACESWO. */
void ad7124_swo_print(const char *s)
{
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7M__) || defined(__CORE_CM7_H_GENERIC)
    if (!s || (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0) return;
    if ((ITM->PORT[0].u32 & 1u) == 0) return;
    while (*s) {
        ITM->PORT[0].u8 = (uint8_t)*s++;
    }
#else
    (void)s;
#endif
}
