/** @file ksz8081.c @brief Read-only KSZ8081 Clause-22 diagnostic driver. */
#include "ksz8081.h"

#include <stddef.h>

#define KSZ8081_BMCR_SPEED_100_MASK 0x2000U
#define KSZ8081_BMCR_DUPLEX_MASK    0x0100U
#define KSZ8081_BMSR_LINK_MASK      0x0004U
#define KSZ8081_BMSR_AN_COMPLETE    0x0020U

static bool KSZ8081_HasValidBus(const ksz8081_t *device)
{
  return (device != NULL) && (device->bus.read_register != NULL);
}

static bool KSZ8081_ReadRegister(ksz8081_t *device,
                                 uint8_t phy_address,
                                 uint8_t register_address,
                                 uint16_t *value)
{
  if (!device->bus.read_register(device->bus.context, phy_address,
                                 register_address, value))
  {
    device->bus_error_count++;
    return false;
  }

  device->read_count++;
  return true;
}

static bool KSZ8081_IdMatches(uint16_t phyid1, uint16_t phyid2)
{
  return (phyid1 == KSZ8081_PHYID1_EXPECTED) &&
         ((phyid2 & KSZ8081_PHYID2_MASK) == KSZ8081_PHYID2_EXPECTED);
}

static ksz8081_result_t KSZ8081_ReadSnapshot(ksz8081_t *device,
                                              uint8_t phy_address,
                                              ksz8081_status_t *status)
{
  uint16_t bmsr_first;

  if (!KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_PHYID1,
                            &status->phyid1) ||
      !KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_PHYID2,
                            &status->phyid2))
  {
    return KSZ8081_RESULT_BUS_ERROR;
  }

  if (!KSZ8081_IdMatches(status->phyid1, status->phyid2))
  {
    return KSZ8081_RESULT_ID_MISMATCH;
  }

  if (!KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_BMCR,
                            &status->bmcr) ||
      !KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_BMSR,
                            &bmsr_first) ||
      !KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_BMSR,
                            &status->bmsr) ||
      !KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_ANAR,
                            &status->anar) ||
      !KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_ANLPAR,
                            &status->anlpar) ||
      !KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_PHY_CONTROL_2,
                            &status->phy_control_2))
  {
    return KSZ8081_RESULT_BUS_ERROR;
  }

  (void)bmsr_first; /* BMSR link status is latched low; the second read is live. */
  status->phy_address = phy_address;
  status->link_up = (status->bmsr & KSZ8081_BMSR_LINK_MASK) != 0U;
  status->auto_negotiation_complete =
      (status->bmsr & KSZ8081_BMSR_AN_COMPLETE) != 0U;
  status->speed_100_mbit = (status->bmcr & KSZ8081_BMCR_SPEED_100_MASK) != 0U;
  status->full_duplex = (status->bmcr & KSZ8081_BMCR_DUPLEX_MASK) != 0U;
  status->read_count = device->read_count;
  status->bus_error_count = device->bus_error_count;
  return KSZ8081_RESULT_OK;
}

ksz8081_result_t KSZ8081_Init(ksz8081_t *device, const ksz8081_bus_t *bus)
{
  if ((device == NULL) || (bus == NULL) || (bus->read_register == NULL))
  {
    return KSZ8081_RESULT_ARGUMENT;
  }

  device->bus = *bus;
  device->read_count = 0U;
  device->bus_error_count = 0U;
  device->scan_count = 0U;
  device->not_found_count = 0U;
  return KSZ8081_RESULT_OK;
}

ksz8081_result_t KSZ8081_Scan(ksz8081_t *device, ksz8081_status_t *status)
{
  uint16_t phyid1;
  uint16_t phyid2;
  bool any_response = false;

  if (!KSZ8081_HasValidBus(device) || (status == NULL))
  {
    return KSZ8081_RESULT_ARGUMENT;
  }

  device->scan_count++;
  for (uint8_t phy_address = 0U; phy_address < 32U; phy_address++)
  {
    if (!KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_PHYID1, &phyid1) ||
        !KSZ8081_ReadRegister(device, phy_address, KSZ8081_REG_PHYID2, &phyid2))
    {
      continue;
    }
    any_response = true;

    if (KSZ8081_IdMatches(phyid1, phyid2))
    {
      return KSZ8081_ReadSnapshot(device, phy_address, status);
    }
  }

  device->not_found_count++;
  return any_response ? KSZ8081_RESULT_ID_MISMATCH : KSZ8081_RESULT_NOT_FOUND;
}

ksz8081_result_t KSZ8081_ReadStatus(ksz8081_t *device,
                                     uint8_t phy_address,
                                     ksz8081_status_t *status)
{
  if (!KSZ8081_HasValidBus(device) || (status == NULL) || (phy_address >= 32U))
  {
    return KSZ8081_RESULT_ARGUMENT;
  }

  return KSZ8081_ReadSnapshot(device, phy_address, status);
}
