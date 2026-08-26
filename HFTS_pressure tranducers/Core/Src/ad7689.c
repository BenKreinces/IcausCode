// ad7689.c
#include "ad7689.h"

// ---------- DWT delay (works on Cortex-M3/M4/M7; enable in CubeMX: Debug -> Serial Wire) ----------
void AD7689_DWT_Init(void)
{
    // Enable TRC
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Reset cycle counter
    DWT->CYCCNT = 0;
    // Enable cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void AD7689_DelayNs(uint32_t ns)
{
    // Best-effort busy wait using CPU cycles.
    // ns resolution is limited; good for sub-us edge spacing.
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

// ---------- low-level transaction ----------
// This performs one "frame":
// 1) CNV rising edge starts conversion (DIN ignored, SDO hi-Z)
// 2) CNV low allows SPI burst to (a) shift out previous conversion result and (b) load next CFG (first 14 rising edges)
// Datasheet notes: CFG is latched on first 14 SCK rising edges; conversion result clocks out on first 16 falling edges:contentReference[oaicite:13]{index=13}
HAL_StatusTypeDef AD7689_ReadRaw(AD7689_Handle *dev, uint16_t cfg14, uint16_t *out_code)
{
    // Left-align 14-bit CFG into a 16-bit SPI word so last 2 DIN bits are 0
    uint16_t tx = (uint16_t)((cfg14 & 0x3FFFu) << 2);

    uint16_t rx = 0;


    // Start conversion: CNV rising edge
    HAL_GPIO_WritePin(dev->CNV_GPIO, dev->CNV_Pin, GPIO_PIN_SET);
    // Keep digital quiet ~20ns before and ~10ns after CNV edge per datasheet guidance:contentReference[oaicite:14]{index=14}
    AD7689_DelayNs(50);
    HAL_GPIO_WritePin(dev->CNV_GPIO, dev->CNV_Pin, GPIO_PIN_RESET);

    // If you are NOT polling SDO for end-of-conv, wait a conservative conversion time.
    // (Set dev->tconv_ns based on your throughput choice / datasheet timing table.)
    if (dev->tconv_ns) {
        AD7689_DelayNs(dev->tconv_ns);
    }

    // Burst 16 clocks while CNV is low:
    // - RX gives you the previous conversion’s 16-bit result
    // - TX updates the CFG for the next conversion (first 14 rising edges)
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(dev->hspi,
                                                  (uint8_t*)&tx,
                                                  (uint8_t*)&rx,
                                                  1,   // one 16-bit word
                                                  10); // timeout ms
    if (st != HAL_OK) return st;

    *out_code = rx;
    return HAL_OK;
}

HAL_StatusTypeDef AD7689_ReadIN0_Raw(AD7689_Handle *dev, uint16_t *out_code)
{
    // CFG[13]=1 overwrite, INCC=111 unipolar to GND, INx=000 IN0,
    // BW=1 full, REF=001 internal 4.096V, SEQ=00 disable, RB=1 no readback
    const uint16_t CFG_IN0_INT4096 = 0x3C49;
    return AD7689_ReadRaw(dev, CFG_IN0_INT4096, out_code);
}

HAL_StatusTypeDef AD7689_ReadIN1_Raw(AD7689_Handle *dev, uint16_t *out_code)
{
    // Same as IN0 but INx=001 for IN1
    const uint16_t CFG_IN1_INT4096 = 0x3C4A;
    return AD7689_ReadRaw(dev, CFG_IN1_INT4096, out_code);
}

HAL_StatusTypeDef AD7689_ReadIN2_Raw(AD7689_Handle *dev, uint16_t *out_code)
{
    // Same as IN0 but INx=010 for IN2
    const uint16_t CFG_IN2_INT4096 = 0x3C4B;
    return AD7689_ReadRaw(dev, CFG_IN2_INT4096, out_code);
}

HAL_StatusTypeDef AD7689_Init_IN0_IntRef4096(AD7689_Handle *dev)
{
    // Internal ref = 4.096V requires VDD >= 4.5V (datasheet pin description):contentReference[oaicite:17]{index=17}
    dev->vref_volts = 4.096f;

    // Datasheet: CFG undefined at power-up; two dummy conversions required:contentReference[oaicite:18]{index=18}
    uint16_t dummy = 0;
    HAL_StatusTypeDef st;

    // Run a few frames so the one-deep CFG delay and dummy requirement are satisfied
    for (int i = 0; i < 3; i++) {
        st = AD7689_ReadIN0_Raw(dev, &dummy);
        if (st != HAL_OK) return st;
    }
    return HAL_OK;
}
