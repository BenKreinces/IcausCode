/**
 * @file ksz8081.h
 * @brief Bus-independent, read-only KSZ8081 PHY diagnostics.
 *
 * The driver performs Clause-22 MDIO reads only. It does not reset, configure,
 * or otherwise alter the PHY. A single task must own a bus instance.
 */
#ifndef KSZ8081_H
#define KSZ8081_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  KSZ8081_REG_BMCR = 0x00U,
  KSZ8081_REG_BMSR = 0x01U,
  KSZ8081_REG_PHYID1 = 0x02U,
  KSZ8081_REG_PHYID2 = 0x03U,
  KSZ8081_REG_ANAR = 0x04U,
  KSZ8081_REG_ANLPAR = 0x05U,
  KSZ8081_REG_PHY_CONTROL_2 = 0x1FU,
  KSZ8081_PHYID1_EXPECTED = 0x0022U,
  KSZ8081_PHYID2_MASK = 0xFFF0U,
  KSZ8081_PHYID2_EXPECTED = 0x1560U
};

typedef enum
{
  KSZ8081_RESULT_OK = 0,
  KSZ8081_RESULT_ARGUMENT,
  KSZ8081_RESULT_NOT_FOUND,
  KSZ8081_RESULT_BUS_ERROR,
  KSZ8081_RESULT_ID_MISMATCH
} ksz8081_result_t;

typedef struct
{
  void *context;
  bool (*read_register)(void *context,
                        uint8_t phy_address,
                        uint8_t register_address,
                        uint16_t *value);
} ksz8081_bus_t;

typedef struct
{
  uint8_t phy_address;
  uint16_t phyid1;
  uint16_t phyid2;
  uint16_t bmcr;
  uint16_t bmsr;
  uint16_t anar;
  uint16_t anlpar;
  uint16_t phy_control_2;
  bool link_up;
  bool auto_negotiation_complete;
  bool speed_100_mbit;
  bool full_duplex;
  uint32_t read_count;
  uint32_t bus_error_count;
} ksz8081_status_t;

typedef struct
{
  ksz8081_bus_t bus;
  uint32_t read_count;
  uint32_t bus_error_count;
  uint32_t scan_count;
  uint32_t not_found_count;
} ksz8081_t;

/** Store validated MDIO callbacks. No MDIO transaction occurs. */
ksz8081_result_t KSZ8081_Init(ksz8081_t *device, const ksz8081_bus_t *bus);

/** Scan Clause-22 addresses 0 through 31 and return a KSZ8081 status snapshot. */
ksz8081_result_t KSZ8081_Scan(ksz8081_t *device, ksz8081_status_t *status);

/** Refresh link/negotiation status for the previously discovered PHY address. */
ksz8081_result_t KSZ8081_ReadStatus(ksz8081_t *device,
                                     uint8_t phy_address,
                                     ksz8081_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* KSZ8081_H */
