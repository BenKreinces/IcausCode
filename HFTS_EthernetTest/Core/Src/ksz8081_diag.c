/*
 * KSZ8081 (Micrel/Microchip) strap/config sanity check over Clause-22 MDIO.
 *
 * See `Core/Inc/ksz8081_diag.h` for notes and vendor-doc references.
 */

#include "ksz8081_diag.h"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_eth.h"

#include <stdbool.h>

extern ETH_HandleTypeDef heth;

volatile ksz8081_diag_snapshot_t g_ksz8081_diag = {0};

/* --- Clause 22 registers --- */
#define C22_BMCR      0x00u
#define C22_BMSR      0x01u
#define C22_PHYID1    0x02u
#define C22_PHYID2    0x03u
#define C22_ANAR      0x04u
#define C22_ANLPAR    0x05u

/* --- IEEE standard bitfields (subset) --- */
#define BMCR_RESET          0x8000u
#define BMCR_LOOPBACK       0x4000u
#define BMCR_SPEED_100      0x2000u
#define BMCR_ANEG_EN        0x1000u
#define BMCR_POWER_DOWN     0x0800u
#define BMCR_ISOLATE        0x0400u
#define BMCR_RESTART_ANEG   0x0200u
#define BMCR_DUPLEX         0x0100u

#define BMSR_100T4          0x8000u
#define BMSR_100TX_FD       0x4000u
#define BMSR_100TX_HD       0x2000u
#define BMSR_10_FD          0x1000u
#define BMSR_10_HD          0x0800u
#define BMSR_ANEG_CPLT      0x0020u
#define BMSR_ANEG_ABLE      0x0008u
#define BMSR_LINK_STATUS    0x0004u

#define AN_100TX_FD         0x0100u
#define AN_100TX_HD         0x0080u
#define AN_10_FD            0x0040u
#define AN_10_HD            0x0020u
#define AN_PAUSE_MASK       0x0C00u

/* --- KSZ8081 identification (verified from Microchip/Micrel datasheets) --- */
#define KSZ8081_PHYID1_EXPECTED     0x0022u
/* KSZ8081 PHYID2: OUI[19:24]=0x05, model=0x16; revision varies in [3:0] */
#define KSZ8081_PHYID2_MASK         0xFFF0u
#define KSZ8081_PHYID2_EXPECTED     0x1560u

/* --- KSZ8081 vendor register(s) --- */
#define KSZ8081_REG_PHYCTRL2        0x1Fu

/* KSZ8081RNA/RND: Reg 0x1F bit7 RMII reference clock select (datasheet: "PHY Control 2"). */
#define KSZ8081_PHYCTRL2_RMII_REFCLK_SEL   (1u << 7)

/* KSZ8081MLX: Reg 0x1F[15:13] Auto-MDI/MDIX controls (datasheet: "PHY Control 2"). */
#define KSZ8081_PHYCTRL2_HP_MDIX          (1u << 15)
#define KSZ8081_PHYCTRL2_MDI_MDIX_SEL     (1u << 14) /* valid when Auto-MDIX disabled */
#define KSZ8081_PHYCTRL2_PAIR_SWAP_DIS    (1u << 13) /* 1=disable Auto-MDIX, 0=enable */

static bool mdio_read_c22_retry(uint8_t phy_addr, uint8_t reg, uint16_t *out)
{
  if (!out)
  {
    return false;
  }

  for (int attempt = 0; attempt < 3; attempt++)
  {
    uint32_t v = 0;
    if (HAL_ETH_ReadPHYRegister(&heth, phy_addr, reg, &v) == HAL_OK)
    {
      uint16_t vv = (uint16_t)(v & 0xFFFFu);
      if (vv != 0xFFFFu && vv != 0x0000u)
      {
        *out = vv;
        return true;
      }
    }
    HAL_Delay(1);
  }
  return false;
}

static bool mdio_read_c22_raw(uint8_t phy_addr, uint8_t reg, uint16_t *out)
{
  if (!out)
  {
    return false;
  }
  for (int attempt = 0; attempt < 3; attempt++)
  {
    uint32_t v = 0;
    if (HAL_ETH_ReadPHYRegister(&heth, phy_addr, reg, &v) == HAL_OK)
    {
      *out = (uint16_t)(v & 0xFFFFu);
      return true;
    }
    HAL_Delay(1);
  }
  return false;
}

static bool ksz8081_id_match(uint16_t id1, uint16_t id2)
{
  if (id1 != KSZ8081_PHYID1_EXPECTED)
  {
    return false;
  }
  if ((id2 & KSZ8081_PHYID2_MASK) != KSZ8081_PHYID2_EXPECTED)
  {
    return false;
  }
  return true;
}

static bool wait_for_phy_reset_clear(uint8_t phy_addr, uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  while ((HAL_GetTick() - start) < timeout_ms)
  {
    uint16_t bmcr = 0;
    if (mdio_read_c22_raw(phy_addr, C22_BMCR, &bmcr))
    {
      if ((bmcr & BMCR_RESET) == 0)
      {
        return true;
      }
    }
    HAL_Delay(2);
  }
  return false;
}

int ksz8081_strap_check(ksz8081_detect_result_t *out_result)
{
  /* Update a coherent snapshot for debugger live-view. */
  ksz8081_diag_snapshot_t snap;
  snap.stamp_ms = HAL_GetTick();
  snap.found = 0;
  snap.phy_addr = 0xFF;
  snap.phyid1 = 0;
  snap.phyid2 = 0;
  snap.bmcr = 0;
  snap.bmsr = 0;
  snap.anar = 0;
  snap.anlpar = 0;
  snap.vend_1f = 0;
  for (int i = 0; i < 16; i++)
  {
    snap.vend_10_1f[i] = 0;
  }

  /* Ensure MDIO clock range is configured (safe to call any time). */
  HAL_ETH_SetMDIOClockRange(&heth);

  /* Scan 0..31 for a valid KSZ8081 PHY ID. */
  int found = 0;
  ksz8081_detect_result_t det = {0};
  for (uint8_t addr = 0; addr < 32; addr++)
  {
    uint16_t id1 = 0, id2 = 0;
    if (!mdio_read_c22_retry(addr, C22_PHYID1, &id1))
    {
      continue;
    }
    if (!mdio_read_c22_retry(addr, C22_PHYID2, &id2))
    {
      continue;
    }
    if (ksz8081_id_match(id1, id2))
    {
      det.phy_addr = addr;
      det.phyid1 = id1;
      det.phyid2 = id2;
      found = 1;
      break;
    }
  }

  if (!found)
  {
    g_ksz8081_diag = snap;
    return -1;
  }

  snap.found = 1;
  snap.phy_addr = det.phy_addr;
  snap.phyid1 = det.phyid1;
  snap.phyid2 = det.phyid2;

  /* Reset/timing robustness: poll BMCR.RESET clearing (bounded). */
  (void)wait_for_phy_reset_clear(det.phy_addr, 100);

  /* Read standard IEEE 802.3 clause 22 registers. */
  uint16_t bmcr = 0, bmsr = 0, anar = 0, anlpar = 0;
  (void)mdio_read_c22_raw(det.phy_addr, C22_BMCR, &bmcr);
  (void)mdio_read_c22_raw(det.phy_addr, C22_BMSR, &bmsr);
  /* BMSR link is latched-low; read twice per IEEE common practice. */
  (void)mdio_read_c22_raw(det.phy_addr, C22_BMSR, &bmsr);
  (void)mdio_read_c22_raw(det.phy_addr, C22_ANAR, &anar);
  (void)mdio_read_c22_raw(det.phy_addr, C22_ANLPAR, &anlpar);

  snap.bmcr = bmcr;
  snap.bmsr = bmsr;
  snap.anar = anar;
  snap.anlpar = anlpar;

  /* Vendor range dump (0x10..0x1F): raw hex only (bounded). */
  for (uint8_t r = 0x10; r <= 0x1F; r++)
  {
    uint16_t v = 0;
    if (mdio_read_c22_raw(det.phy_addr, r, &v))
    {
      snap.vend_10_1f[r - 0x10] = v;
    }
    else
    {
      snap.vend_10_1f[r - 0x10] = 0;
    }
  }

  /* KSZ8081-specific decode (verified bitfields only). */
  uint16_t phyctrl2 = 0;
  if (mdio_read_c22_raw(det.phy_addr, KSZ8081_REG_PHYCTRL2, &phyctrl2))
  {
    snap.vend_1f = phyctrl2;
  }

  g_ksz8081_diag = snap;

  if (out_result)
  {
    *out_result = det;
  }
  return 0;
}

