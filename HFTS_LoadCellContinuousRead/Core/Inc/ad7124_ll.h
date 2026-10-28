/**
  ******************************************************************************
  * @file    ad7124_ll.h
  * @brief   Low-level AD7124-8 SPI register access (STM32 HAL)
  ******************************************************************************
  * @details Hardware assumptions (verify in CubeMX):
  *   - SPI2: CPOL=1, CPHA=1 (SPI Mode 3), MSB-first, CRC disabled
  *   - SPI2 MISO on PB14 (wired to AD7124 DOUT/RDY)
  *   - Chip select: software-controlled via spi2_cs_lc (SPI2_CS_LC on PD4)
  ******************************************************************************
  */

#ifndef AD7124_LL_H
#define AD7124_LL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* AD7124 register addresses (AD7124-8 datasheet) */
#define AD7124_REG_STATUS       0x00U
#define AD7124_REG_ADC_CONTROL  0x01U
#define AD7124_REG_DATA         0x02U
#define AD7124_REG_ID           0x05U
#define AD7124_REG_ERR          0x06U
#define AD7124_REG_ERREN        0x07U
#define AD7124_REG_CHANNEL_0    0x09U
#define AD7124_REG_CHANNEL_15   0x18U
#define AD7124_REG_CONFIG_0     0x19U
#define AD7124_REG_FILTER_0     0x21U

/* AD7124 ID register reset value */
#define AD7124_ID_EXPECTED     0x12U

/* Communications register: WEN=0 (bit7), R/W (bit6), Addr[5:0] */
#define AD7124_CMD_WEN_MASK    0x80U
#define AD7124_CMD_RW_MASK     0x40U
#define AD7124_CMD_ADDR_MASK   0x3FU

/**
 * @brief AD7124 low-level context
 */
typedef struct {
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef      *cs_port;
  uint16_t          cs_pin;
} ad7124_ll_ctx_t;

/**
 * @brief Form AD7124 communications register command byte.
 * @param rw 0=write, 1=read
 * @param addr Register address (0x00..0x3F)
 * @return Command byte: (0<<7) | (rw<<6) | (addr & 0x3F)
 */
static inline uint8_t ad7124_ll_cmd_byte(uint8_t rw, uint8_t addr)
{
  return (0U << 7) | ((rw & 1U) << 6) | (addr & AD7124_CMD_ADDR_MASK);
}

/**
 * @brief Interface reset: clock >= 64 SCLKs with DIN high.
 *        CS low, transmit 8 bytes 0xFF, CS high.
 * @param ctx Low-level context
 * @return HAL status
 */
HAL_StatusTypeDef ad7124_reset(const ad7124_ll_ctx_t *ctx);

/**
 * @brief Write a register (MSB-first, multi-byte).
 * @param ctx Low-level context
 * @param addr Register address (0x00..0x3F)
 * @param value Pointer to register value (MSB first)
 * @param nbytes Number of bytes (1, 2, or 3)
 * @return HAL status
 */
HAL_StatusTypeDef ad7124_write_reg(const ad7124_ll_ctx_t *ctx, uint8_t addr,
                                  const uint8_t *value, uint8_t nbytes);

/**
 * @brief Read a register (MSB-first, multi-byte).
 * @param ctx Low-level context
 * @param addr Register address (0x00..0x3F)
 * @param buf  Output buffer (MSB first)
 * @param nbytes Number of bytes to read
 * @return HAL status
 */
HAL_StatusTypeDef ad7124_read_reg(const ad7124_ll_ctx_t *ctx, uint8_t addr,
                                 uint8_t *buf, uint8_t nbytes);

/**
 * @brief Assert CS (low). For use by ISR when reading DATA in continuous mode.
 */
void ad7124_ll_cs_assert(const ad7124_ll_ctx_t *ctx);

/**
 * @brief Deassert CS (high).
 */
void ad7124_ll_cs_deassert(const ad7124_ll_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* AD7124_LL_H */
