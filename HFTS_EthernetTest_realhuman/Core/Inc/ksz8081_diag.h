/*
 * KSZ8081 (Micrel/Microchip) strap/config sanity check over Clause-22 MDIO.
 *
 * This module is intended for *runtime bring-up diagnostics* to confirm that
 * the PHY came out of reset with the expected strap configuration.
 *
 * Verified references used for vendor-specific decoding:
 * - KSZ8081RNA/RND datasheet (Microchip): "Register 1Fh – PHY Control 2"
 *   bit[7] "RMII Reference Clock Select" (25 MHz vs 50 MHz mode).
 * - KSZ8081MLX datasheet (Micrel/Microchip): "Register 1Fh – PHY Control 2"
 *   bit[13] Auto MDI/MDI-X disable, bit[14] MDI/MDI-X select (when disabled),
 *   bit[15] HP_MDIX selector.
 *
 * Note: KSZ8081 variants differ. If you are *not* using an RNA/RND or MLX
 * family part, treat the vendor register decodes as best-effort and rely on
 * the raw hex dump.
 */

#ifndef KSZ8081_DIAG_H
#define KSZ8081_DIAG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  uint8_t phy_addr;     /* 0..31 */
  uint16_t phyid1;      /* reg 0x02 */
  uint16_t phyid2;      /* reg 0x03 */
} ksz8081_detect_result_t;

typedef struct
{
  uint32_t stamp_ms;
  uint8_t found;
  uint8_t phy_addr;

  uint16_t phyid1;
  uint16_t phyid2;

  uint16_t bmcr;     /* 0x00 */
  uint16_t bmsr;     /* 0x01 (read twice before storing) */
  uint16_t anar;     /* 0x04 */
  uint16_t anlpar;   /* 0x05 */

  uint16_t vend_1f;  /* 0x1F */
  uint16_t vend_10_1f[16]; /* regs 0x10..0x1F */
} ksz8081_diag_snapshot_t;

/* Live-view in debugger (filled by ksz8081_strap_check). */
extern volatile ksz8081_diag_snapshot_t g_ksz8081_diag;

/**
 * @brief Scan MDIO 0..31, detect KSZ8081, then print a concise report.
 *
 * @param[out] out_result Optional; filled on success.
 * @return 0 on success, <0 on failure (not found / MDIO error).
 */
int ksz8081_strap_check(ksz8081_detect_result_t *out_result);

#ifdef __cplusplus
}
#endif

#endif /* KSZ8081_DIAG_H */
