/*
 * ad7124-8.c
 *
 *  Created on: Feb 17, 2026
 *      Author: alexi
 */


#include "ad7124-8.h"

// ---- AD7124 register addresses (RS[5:0]) ----
#define AD7124_REG_STATUS   0x00
#define AD7124_REG_DATA     0x02
#define AD7124_REG_ID       0x05

// ---- Command byte builder: [7]=WEN(0), [6]=R/W, [5:0]=addr ----
static inline uint8_t ad7124_cmd(bool read, uint8_t reg)
{
    // WEN must be 0 for the write to the communications register to occur.:contentReference[oaicite:7]{index=7}
    return (uint8_t)((read ? 0x40 : 0x00) | (reg & 0x3F));
}

static inline void cs_low(ad7124_t *dev)  { HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET); }
static inline void cs_high(ad7124_t *dev) { HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET); }

void ad7124_reset(ad7124_t *dev)
{
    // 64 SCLK cycles with DIN high resets interface/device back to default state.:contentReference[oaicite:8]{index=8}
    uint8_t ff[8];
    for (int i = 0; i < 8; i++) ff[i] = 0xFF;

    cs_low(dev);
    (void)HAL_SPI_Transmit(dev->hspi, ff, sizeof(ff), 100);
    cs_high(dev);
}


HAL_StatusTypeDef ad7124_read_reg(ad7124_t *dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t cmd = ad7124_cmd(true, reg);

    cs_low(dev);

    // Write communications register command first.:contentReference[oaicite:9]{index=9}
    HAL_StatusTypeDef st = HAL_SPI_Transmit(dev->hspi, &cmd, 1, 100);
    if (st != HAL_OK) { cs_high(dev); return st; }

    // Then read register bytes
    st = HAL_SPI_Receive(dev->hspi, buf, len, 100);

    cs_high(dev);
    return st;
}


HAL_StatusTypeDef ad7124_write_reg(ad7124_t *dev, uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t cmd = ad7124_cmd(false, reg);

    cs_low(dev);

    HAL_StatusTypeDef st = HAL_SPI_Transmit(dev->hspi, &cmd, 1, 100);
    if (st != HAL_OK) { cs_high(dev); return st; }

    st = HAL_SPI_Transmit(dev->hspi, (uint8_t*)buf, len, 100);

    cs_high(dev);
    return st;
}

bool ad7124_data_ready(ad7124_t *dev)
{
    // If you wired DOUT/RDY to a GPIO, that’s fastest.
    if (dev->drdy_port != NULL) {
        return (HAL_GPIO_ReadPin(dev->drdy_port, dev->drdy_pin) == GPIO_PIN_RESET);
    }

    // Otherwise read STATUS and check RDY bit (bit7).:contentReference[oaicite:10]{index=10}
    uint8_t status = 0xFF;
    if (ad7124_read_reg(dev, AD7124_REG_STATUS, &status, 1) != HAL_OK) return false;
    return ((status & 0x80) == 0); // RDY=0 => new data
}

HAL_StatusTypeDef ad7124_read_data(ad7124_t *dev, ad7124_sample_t *out)
{
    // DATA register is 24-bit by default; can append STATUS if DATA_STATUS enabled (later).:contentReference[oaicite:11]{index=11}:contentReference[oaicite:12]{index=12}
    uint8_t rx[4] = {0};
    uint8_t n = dev->data_status_append ? 4 : 3;

    HAL_StatusTypeDef st = ad7124_read_reg(dev, AD7124_REG_DATA, rx, n);
    if (st != HAL_OK) return st;

    int32_t code = ((int32_t)rx[0] << 16) | ((int32_t)rx[1] << 8) | (int32_t)rx[2];

    // Sign-extend if you’re using bipolar mode; otherwise keep as unsigned.
    // (You’ll know once you set CONFIG_x bipolar/unipolar.)
    if (code & 0x800000) code |= 0xFF000000;

    out->code24 = code;
    out->status = dev->data_status_append ? rx[3] : 0;
    return HAL_OK;
}

HAL_StatusTypeDef ad7124_init(ad7124_t *dev)
{
    ad7124_reset(dev);

    // Verify ID register == 0x17 for AD7124-8.:contentReference[oaicite:13]{index=13}
    uint8_t id = 0;
    HAL_StatusTypeDef st = ad7124_read_reg(dev, AD7124_REG_ID, &id, 1);
    if (st != HAL_OK) return st;

    if (id != 0x17) {
        return HAL_ERROR; // or define your own error codes
    }

    dev->data_status_append = false;
    return HAL_OK;
}
