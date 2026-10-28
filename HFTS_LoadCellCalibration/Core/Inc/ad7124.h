/**
 ******************************************************************************
 * @file    ad7124.h
 * @brief   AD7124-8 SPI driver for load-cell ADC path.
 ******************************************************************************
 * Uses HAL SPI. CS controlled by board_pins.h (LC_CS).
 * No UART, no dynamic allocation.
 ******************************************************************************
 */
#ifndef AD7124_H
#define AD7124_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------- */
/* AD7124 register map                                                        */
/* -------------------------------------------------------------------------- */
#define AD7124_COMM_REG      0x00
#define AD7124_STATUS_REG    0x00
#define AD7124_ADC_CTRL_REG  0x01
#define AD7124_DATA_REG      0x02
#define AD7124_IO_CTRL1_REG  0x03
#define AD7124_IO_CTRL2_REG  0x04
#define AD7124_ID_REG        0x05
#define AD7124_ERR_REG       0x06
#define AD7124_ERREN_REG     0x07
#define AD7124_CH0_MAP_REG   0x09
#define AD7124_CH1_MAP_REG   0x0A
#define AD7124_CH2_MAP_REG   0x0B
#define AD7124_CH3_MAP_REG   0x0C
#define AD7124_CH4_MAP_REG   0x0D
#define AD7124_CH5_MAP_REG   0x0E
#define AD7124_CH6_MAP_REG   0x0F
#define AD7124_CH7_MAP_REG   0x10
#define AD7124_CH8_MAP_REG   0x11
#define AD7124_CH9_MAP_REG   0x12
#define AD7124_CH10_MAP_REG  0x13
#define AD7124_CH11_MAP_REG  0x14
#define AD7124_CH12_MAP_REG  0x15
#define AD7124_CH13_MAP_REG  0x16
#define AD7124_CH14_MAP_REG  0x17
#define AD7124_CH15_MAP_REG  0x18
#define AD7124_CFG0_REG      0x19
#define AD7124_CFG1_REG      0x1A
#define AD7124_CFG2_REG      0x1B
#define AD7124_CFG3_REG      0x1C
#define AD7124_CFG4_REG      0x1D
#define AD7124_CFG5_REG      0x1E
#define AD7124_CFG6_REG      0x1F
#define AD7124_CFG7_REG      0x20
#define AD7124_FILT0_REG     0x21
#define AD7124_FILT1_REG     0x22
#define AD7124_FILT2_REG     0x23
#define AD7124_FILT3_REG     0x24
#define AD7124_FILT4_REG     0x25
#define AD7124_FILT5_REG     0x26
#define AD7124_FILT6_REG     0x27
#define AD7124_FILT7_REG     0x28
#define AD7124_OFFS0_REG     0x29
#define AD7124_GAIN0_REG     0x31

/* Comm register */
#define AD7124_COMM_RD       (1u << 6)
#define AD7124_COMM_WR       (0u << 6)
#define AD7124_COMM_RA_MASK  0x3Fu

/* Status register */
#define AD7124_STATUS_RDY    (1u << 7)
#define AD7124_STATUS_ERR    (1u << 6)

/* ADC control */
#define AD7124_ADC_CTRL_MODE(x)      (((x) & 0xFu) << 2)
#define AD7124_ADC_CTRL_PWR(x)       (((x) & 0x3u) << 6)
#define AD7124_ADC_CTRL_REF_EN       (1u << 8)
#define AD7124_ADC_CTRL_DATA_STATUS  (1u << 10)
#define AD7124_ADC_CTRL_CONT_READ    (1u << 11)
#define AD7124_MODE_CONT_CONV        0x01u  /* Continuous conversion mode */

/* Config register */
#define AD7124_CFG_BIPOLAR     (1u << 11)
#define AD7124_CFG_REF_BUFP    (1u << 8)
#define AD7124_CFG_REF_BUFM    (1u << 7)
#define AD7124_CFG_AIN_BUFP    (1u << 6)
#define AD7124_CFG_AINN_BUFM   (1u << 5)
#define AD7124_CFG_REF_SEL(x)  (((x) & 0x3u) << 3)
#define AD7124_CFG_PGA(x)      (((x) & 0x7u) << 0)
#define AD7124_REF_EXTERNAL  0
#define AD7124_REF_INTERNAL  1
#define AD7124_REF_AVDD      2

/* PGA: 0=1, 1=2, 2=4, 3=8, 4=16, 5=32, 6=64, 7=128 */
#define AD7124_PGA_1         0u
#define AD7124_PGA_2         1u
#define AD7124_PGA_4         2u
#define AD7124_PGA_8         3u
#define AD7124_PGA_16        4u
#define AD7124_PGA_32        5u
#define AD7124_PGA_64        6u
#define AD7124_PGA_128       7u

/* Channel map */
#define AD7124_CH_ENABLE     (1u << 15)
#define AD7124_CH_SETUP(x)   (((x) & 0x7u) << 12)
#define AD7124_CH_AINP(x)    (((x) & 0x1Fu) << 5)
#define AD7124_CH_AINM(x)    (((x) & 0x1Fu) << 0)

/* Filter register */
#define AD7124_FILT_TYPE(x)  (((x) & 0x7u) << 21)
#define AD7124_FILT_FS(x)    (((x) & 0x7FFu) << 0)
#define AD7124_FILTER_SINC4  0u

/* Expected device IDs */
#define AD7124_8_STD_ID      0x14u
#define AD7124_8_BW_ID       0x16u
#define AD7124_8_NEW_ID      0x17u

/* -------------------------------------------------------------------------- */
/* Driver API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize AD7124 driver with HAL SPI handle.
 * @param hspi HAL SPI handle (e.g. &hspi2)
 * @return true on success
 */
bool AD7124_Init(void *hspi);

/**
 * @brief Perform software reset (64 consecutive 1s on DIN).
 */
void AD7124_Reset(void);

/**
 * @brief Read ID register.
 * @return ID byte (expected 0x14, 0x16, or 0x17 for AD7124-8)
 */
uint8_t AD7124_ReadID(void);

/**
 * @brief Read status register.
 * @return Status byte (bit 7 = RDY)
 */
uint8_t AD7124_ReadStatus(void);

/**
 * @brief Check if conversion result is ready (RDY bit or DRDY pin).
 * AD7124: RDY=0 means data ready. @return true if new data available
 */
bool AD7124_DataReady(void);

/**
 * @brief Block until RDY or timeout. Per datasheet, must wait for RDY before reading DATA.
 * @param timeout_ms Max ms to wait (0 = return immediately)
 * @return true if RDY seen, false on timeout
 */
bool AD7124_WaitDataReady(uint32_t timeout_ms);

/**
 * @brief Read 24-bit conversion result (raw code).
 * @return Signed 24-bit value (-8388608..8388607)
 */
int32_t AD7124_ReadData(void);

/** Debug: raw SPI bytes from last DATA read [0..3]. Watch in debugger. */
extern volatile uint8_t g_ad7124_rx[4];

/**
 * @brief Configure for load-cell: channel 0 = AIN2/AIN3 differential, setup 0.
 * @param bipolar 1 for bipolar, 0 for unipolar
 * @param pga PGA setting (0..7)
 * @param ref_sel Reference select (0=external, 1=internal, 2=AVDD)
 */
void AD7124_ConfigChannel(uint8_t bipolar, uint8_t pga, uint8_t ref_sel);

/**
 * @brief Configure filter for setup 0.
 * @param filter_type 0=SINC4, etc.
 * @param fs Filter register FS value (0..1023, higher = lower ODR)
 */
void AD7124_ConfigFilter(uint8_t filter_type, uint16_t fs);

/**
 * @brief Start continuous conversion mode.
 * @param use_internal_ref true = enable internal 2.5V ref (REF_EN), false = use external REF pins
 */
void AD7124_StartContinuous(bool use_internal_ref);

/**
 * @brief Read 8-bit register (e.g. ID, Status).
 */
uint8_t AD7124_ReadReg8(uint8_t addr);

/**
 * @brief Read 16-bit register (e.g. CH_MAP).
 */
uint16_t AD7124_ReadReg16(uint8_t addr);

/**
 * @brief Read 24-bit register (e.g. CFG0, ADC_CTRL, ERR).
 */
uint32_t AD7124_ReadReg24(uint8_t addr);

/**
 * @brief Stop conversions.
 */
void AD7124_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* AD7124_H */
