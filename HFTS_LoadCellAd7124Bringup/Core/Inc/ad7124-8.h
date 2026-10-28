/*
 * ad7124-8.h
 *
 *  Created on: Feb 17, 2026
 *      Author: alexi
 */
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;

    // Optional but useful:
    GPIO_TypeDef *drdy_port;   // can be NULL if not used
    uint16_t drdy_pin;

    // Cached settings (optional)
    bool data_status_append;   // if you enable DATA_STATUS later
} ad7124_t;

typedef struct {
    int32_t code24;     // signed/unsigned handling depends on bipolar/unipolar config
    uint8_t status;     // valid if data_status_append enabled, else you can read STATUS separately
} ad7124_sample_t;

HAL_StatusTypeDef ad7124_init(ad7124_t *dev);
void ad7124_reset(ad7124_t *dev);

HAL_StatusTypeDef ad7124_read_reg(ad7124_t *dev, uint8_t reg, uint8_t *buf, uint8_t len);
HAL_StatusTypeDef ad7124_write_reg(ad7124_t *dev, uint8_t reg, const uint8_t *buf, uint8_t len);

bool ad7124_data_ready(ad7124_t *dev);
HAL_StatusTypeDef ad7124_read_data(ad7124_t *dev, ad7124_sample_t *out);
