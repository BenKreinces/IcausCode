/**
 * @file ad7124_ll.h
 * @brief Low-level AD7124-8 SPI register access. SPI Mode 3 (CPOL=1, CPHA=1), MSB-first.
 */
#ifndef AD7124_LL_H
#define AD7124_LL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define AD7124_REG_STATUS       0x00U
#define AD7124_REG_ADC_CONTROL  0x01U
#define AD7124_REG_DATA         0x02U
#define AD7124_REG_ID           0x05U
#define AD7124_REG_ERR          0x06U
#define AD7124_REG_ERREN        0x07U
#define AD7124_REG_CHANNEL_0    0x09U
#define AD7124_REG_CONFIG_0     0x19U
#define AD7124_REG_FILTER_0     0x21U

#define AD7124_CMD_ADDR_MASK    0x3FU

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *cs_port;
    uint16_t          cs_pin;
} ad7124_ll_ctx_t;

static inline uint8_t ad7124_ll_cmd_byte(uint8_t rw, uint8_t addr)
{
    return (0U << 7) | ((rw & 1U) << 6) | (addr & AD7124_CMD_ADDR_MASK);
}

HAL_StatusTypeDef ad7124_reset(const ad7124_ll_ctx_t *ctx);
HAL_StatusTypeDef ad7124_write_reg(const ad7124_ll_ctx_t *ctx, uint8_t addr,
                                   const uint8_t *value, uint8_t nbytes);
HAL_StatusTypeDef ad7124_read_reg(const ad7124_ll_ctx_t *ctx, uint8_t addr,
                                  uint8_t *buf, uint8_t nbytes);
void ad7124_ll_cs_assert(const ad7124_ll_ctx_t *ctx);
void ad7124_ll_cs_deassert(const ad7124_ll_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* AD7124_LL_H */
