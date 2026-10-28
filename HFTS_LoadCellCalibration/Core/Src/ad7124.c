/**
 ******************************************************************************
 * @file    ad7124.c
 * @brief   AD7124-8 SPI driver implementation. HAL-based, no dynamic allocation.
 ******************************************************************************
 */
#include "ad7124.h"
#include "board_pins.h"
#include "main.h"
#include <string.h>

/* HAL SPI handle, set by AD7124_Init */
static SPI_HandleTypeDef *s_hspi = NULL;

/* CS assert/deassert */
#define CS_LOW()   HAL_GPIO_WritePin(LC_CS_GPIO_Port, LC_CS_Pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(LC_CS_GPIO_Port, LC_CS_Pin, GPIO_PIN_SET)

static void cs_delay(void)
{
  for (volatile int i = 0; i < 20; i++) { (void)i; }  /* ~1us setup/hold for AD7124 */
}

/**
 * @brief Write then read in one CS session (full-duplex).
 */
static int ad7124_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
  if (!s_hspi) return -1;
  CS_LOW();
  cs_delay();
  int ret = HAL_SPI_TransmitReceive(s_hspi, (uint8_t *)tx, rx, len, 100);
  cs_delay();
  CS_HIGH();
  return ret;
}

/**
 * @brief Write only.
 */
static int ad7124_write(const uint8_t *tx, uint16_t len)
{
  if (!s_hspi) return -1;
  CS_LOW();
  cs_delay();
  int ret = HAL_SPI_Transmit(s_hspi, (uint8_t *)tx, len, 100);
  cs_delay();
  CS_HIGH();
  return ret;
}

bool AD7124_Init(void *hspi)
{
  if (!hspi) return false;
  s_hspi = (SPI_HandleTypeDef *)hspi;
  return true;
}

void AD7124_Reset(void)
{
  /* AD7124 reset: 64 consecutive 1s on DIN. We send 8 bytes of 0xFF. */
  uint8_t buf[8];
  memset(buf, 0xFF, sizeof(buf));
  CS_LOW();
  cs_delay();
  HAL_SPI_Transmit(s_hspi, buf, 8, 100);
  cs_delay();
  CS_HIGH();
  /* Wait for reset to complete */
  for (volatile uint32_t i = 0; i < 10000; i++) { (void)i; }
}

uint8_t AD7124_ReadID(void)
{
  uint8_t tx[2] = { AD7124_COMM_RD | (AD7124_ID_REG & AD7124_COMM_RA_MASK), 0xFF };
  uint8_t rx[2];
  extern volatile uint8_t g_id_rx0, g_id_rx1;
  if (ad7124_xfer(tx, rx, 2) != HAL_OK) return 0;
  g_id_rx0 = rx[0];
  g_id_rx1 = rx[1];
  return rx[1];
}

uint8_t AD7124_ReadStatus(void)
{
  uint8_t tx[2] = { AD7124_COMM_RD | (AD7124_STATUS_REG & AD7124_COMM_RA_MASK), 0xFF };
  uint8_t rx[2];
  if (ad7124_xfer(tx, rx, 2) != HAL_OK) return 0;
  return rx[1];
}

bool AD7124_DataReady(void)
{
#if defined(AD7124_USE_DRDY)
  /* Poll DRDY GPIO - active low when conversion complete */
  extern GPIO_TypeDef *LC_DRDY_GPIO_Port;
  extern uint16_t LC_DRDY_Pin;
  return (HAL_GPIO_ReadPin(LC_DRDY_GPIO_Port, LC_DRDY_Pin) == GPIO_PIN_RESET);
#else
  /* RDY=1 means data ready (revert: conflicting datasheet interpretation) */
  uint8_t status = AD7124_ReadStatus();
  return ((status & AD7124_STATUS_RDY) == 0);
#endif
}

bool AD7124_WaitDataReady(uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  while (1) {
    if (AD7124_DataReady()) return true;
    if (timeout_ms > 0 && (HAL_GetTick() - start) >= timeout_ms) return false;
  }
}

/* Debug: raw SPI bytes from last DATA read (watch in debugger) */
volatile uint8_t g_ad7124_rx[4] = {0};

/*
 * Read DATA register (3 bytes, MSB first).
 * rx[0] = first byte (status or during COMM) - often 0x00, gives only 16 bits if used
 * rx[1],rx[2],rx[3] = 24-bit DATA (MSB, mid, LSB)
 */
#define AD7124_DATA_MSB 1
#define AD7124_DATA_MID 2
#define AD7124_DATA_LSB 3

int32_t AD7124_ReadData(void)
{
  uint8_t cmd = AD7124_COMM_RD | (AD7124_DATA_REG & AD7124_COMM_RA_MASK);
  uint8_t d[3] = {0};

  if (!s_hspi) return 0;

  CS_LOW(); cs_delay();
  if (HAL_SPI_Transmit(s_hspi, &cmd, 1, 100) != HAL_OK) { CS_HIGH(); return 0; }
  if (HAL_SPI_Receive(s_hspi, d, 3, 100) != HAL_OK)      { CS_HIGH(); return 0; }
  cs_delay(); CS_HIGH();

  uint32_t raw = ((uint32_t)d[0] << 16) | ((uint32_t)d[1] << 8) | d[2];
  if (raw & 0x800000u) raw |= 0xFF000000u;
  return (int32_t)raw;
}
/*
int32_t AD7124_ReadData(void)
{
  uint8_t tx[4] = {
    AD7124_COMM_RD | (AD7124_DATA_REG & AD7124_COMM_RA_MASK),
    0xFF, 0xFF, 0xFF
  };
  uint8_t rx[4];
  if (ad7124_xfer(tx, rx, 4) != HAL_OK) return 0;
  for (int i = 0; i < 4; i++) g_ad7124_rx[i] = rx[i];
  uint32_t raw = ((uint32_t)rx[AD7124_DATA_MSB] << 16) |
                 ((uint32_t)rx[AD7124_DATA_MID] << 8) |
                 rx[AD7124_DATA_LSB];
  if (raw & 0x800000u) raw |= 0xFF000000u;
  return (int32_t)raw;
}
*/
static void ad7124_write_reg(uint8_t reg, uint8_t num_bytes, const uint8_t *data)
{
  uint8_t buf[5];
  buf[0] = AD7124_COMM_WR | (reg & AD7124_COMM_RA_MASK);
  for (uint8_t i = 0; i < num_bytes && i < 4; i++)
    buf[1 + i] = data[i];
  ad7124_write(buf, 1 + num_bytes);
}
/*
void AD7124_ConfigChannel(uint8_t bipolar, uint8_t pga, uint8_t ref_sel)
{
  /* Channel 0: AIN2 positive, AIN3 negative (diff = AIN2 - AIN3). Setup 0. */
  /* AD7124 expects MSB first on SPI
  uint16_t ch = AD7124_CH_ENABLE | AD7124_CH_SETUP(0) | AD7124_CH_AINP(2) | AD7124_CH_AINM(3);
  uint8_t ch_map[2] = { (uint8_t)(ch >> 8), (uint8_t)(ch >> 0) };
  ad7124_write_reg(AD7124_CH0_MAP_REG, 2, ch_map);

  /* Config setup 0: bipolar, ref/ain buffers, ref select, PGA. MSB first.
  uint32_t cfg = (bipolar ? AD7124_CFG_BIPOLAR : 0) |
                 //AD7124_CFG_REF_BUFP | AD7124_CFG_REF_BUFM |
                 //AD7124_CFG_AIN_BUFP | AD7124_CFG_AINN_BUFM |
                 AD7124_CFG_REF_SEL(ref_sel) |
                 AD7124_CFG_PGA(pga);
  uint8_t cfg_buf[3] = {
    (uint8_t)(cfg >> 16),
    (uint8_t)(cfg >> 8),
    (uint8_t)(cfg >> 0)
  };
  ad7124_write_reg(AD7124_CFG0_REG, 3, cfg_buf);
}
*/
void AD7124_ConfigFilter(uint8_t filter_type, uint16_t fs)
{
  uint32_t filt = AD7124_FILT_TYPE(filter_type) | AD7124_FILT_FS(fs & 0x7FFu);
  uint8_t buf[3] = {
    (uint8_t)(filt >> 16),
    (uint8_t)(filt >> 8),
    (uint8_t)(filt >> 0)
  };
  ad7124_write_reg(AD7124_FILT0_REG, 3, buf);
}
void AD7124_StartContinuous(bool use_internal_ref)
{
  uint16_t ctrl = 0;
  ctrl |= AD7124_ADC_CTRL_MODE(AD7124_MODE_CONT_CONV);
  ctrl |= AD7124_ADC_CTRL_PWR(0);

  if (use_internal_ref) ctrl |= AD7124_ADC_CTRL_REF_EN;

  uint8_t buf[2] = { (uint8_t)(ctrl >> 8), (uint8_t)(ctrl >> 0) };
  ad7124_write_reg(AD7124_ADC_CTRL_REG, 2, buf);
}

void AD7124_Stop(void)
{
  uint16_t ctrl = AD7124_ADC_CTRL_MODE(0) | AD7124_ADC_CTRL_PWR(0);
  uint8_t buf[2] = { (uint8_t)(ctrl >> 8), (uint8_t)(ctrl >> 0) };
  ad7124_write_reg(AD7124_ADC_CTRL_REG, 2, buf);
}

void AD7124_ConfigChannel(uint8_t bipolar, uint8_t pga, uint8_t ref_sel)
{
  // CH0 map is 2 bytes (you already do this right)
  uint16_t ch = AD7124_CH_ENABLE | AD7124_CH_SETUP(0) | AD7124_CH_AINP(2) | AD7124_CH_AINM(3);
  uint8_t ch_map[2] = { (uint8_t)(ch >> 8), (uint8_t)(ch >> 0) };
  ad7124_write_reg(AD7124_CH0_MAP_REG, 2, ch_map);

  // CONFIG_0 is 2 bytes (this is currently wrong in your code)
  uint16_t cfg = (bipolar ? AD7124_CFG_BIPOLAR : 0) |
                 AD7124_CFG_REF_SEL(ref_sel) |
                 AD7124_CFG_PGA(pga);
  uint8_t cfg_buf[2] = { (uint8_t)(cfg >> 8), (uint8_t)(cfg >> 0) };
  ad7124_write_reg(AD7124_CFG0_REG, 2, cfg_buf);
}

uint8_t AD7124_ReadReg8(uint8_t addr)
{
  uint8_t tx[2] = { AD7124_COMM_RD | (addr & AD7124_COMM_RA_MASK), 0xFF };
  uint8_t rx[2];
  if (ad7124_xfer(tx, rx, 2) != HAL_OK) return 0;
  return rx[1];
}

uint16_t AD7124_ReadReg16(uint8_t addr)
{
  uint8_t tx[3] = { AD7124_COMM_RD | (addr & AD7124_COMM_RA_MASK), 0xFF, 0xFF };
  uint8_t rx[3];
  if (ad7124_xfer(tx, rx, 3) != HAL_OK) return 0;
  return ((uint16_t)rx[1] << 8) | (uint16_t)rx[2];  /* rx[1]=MSB, rx[2]=LSB */
}

uint32_t AD7124_ReadReg24(uint8_t addr)
{
  uint8_t tx[4] = { AD7124_COMM_RD | (addr & AD7124_COMM_RA_MASK), 0xFF, 0xFF, 0xFF };
  uint8_t rx[4];
  if (ad7124_xfer(tx, rx, 4) != HAL_OK) return 0;
  return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | (uint32_t)rx[3];  /* rx[1]=MSB */
}
