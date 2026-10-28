#include "ksz8081.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
  bool present;
  uint8_t address;
  uint16_t registers[32U];
  uint32_t read_count;
} fake_mdio_t;

static bool FakeRead(void *context,
                     uint8_t phy_address,
                     uint8_t register_address,
                     uint16_t *value)
{
  fake_mdio_t *fake = (fake_mdio_t *)context;
  fake->read_count++;
  if (!fake->present || (phy_address != fake->address) ||
      (register_address >= 32U))
  {
    return false;
  }
  *value = fake->registers[register_address];
  return true;
}

static ksz8081_bus_t FakeBus(fake_mdio_t *fake)
{
  ksz8081_bus_t bus = {.context = fake, .read_register = FakeRead};
  return bus;
}

static void InitializeKsz8081(fake_mdio_t *fake, uint8_t address)
{
  memset(fake, 0, sizeof(*fake));
  fake->present = true;
  fake->address = address;
  fake->registers[KSZ8081_REG_PHYID1] = KSZ8081_PHYID1_EXPECTED;
  fake->registers[KSZ8081_REG_PHYID2] = KSZ8081_PHYID2_EXPECTED | 0x0007U;
  fake->registers[KSZ8081_REG_BMCR] = 0x2100U;
  fake->registers[KSZ8081_REG_BMSR] = 0x0024U;
  fake->registers[KSZ8081_REG_ANAR] = 0x01E1U;
  fake->registers[KSZ8081_REG_ANLPAR] = 0x01E1U;
  fake->registers[KSZ8081_REG_PHY_CONTROL_2] = 0x0080U;
}

static void TestScanAndStatus(void)
{
  fake_mdio_t fake;
  ksz8081_t phy;
  ksz8081_status_t status;
  ksz8081_bus_t bus;

  InitializeKsz8081(&fake, 5U);
  bus = FakeBus(&fake);
  assert(KSZ8081_Init(&phy, &bus) == KSZ8081_RESULT_OK);
  assert(KSZ8081_Scan(&phy, &status) == KSZ8081_RESULT_OK);
  assert(status.phy_address == 5U);
  assert(status.phyid1 == 0x0022U);
  assert(status.phyid2 == 0x1567U);
  assert(status.link_up);
  assert(status.auto_negotiation_complete);
  assert(status.speed_100_mbit);
  assert(status.full_duplex);
  assert(status.phy_control_2 == 0x0080U);

  fake.registers[KSZ8081_REG_BMSR] = 0x0000U;
  assert(KSZ8081_ReadStatus(&phy, status.phy_address, &status) ==
         KSZ8081_RESULT_OK);
  assert(!status.link_up);
  assert(!status.auto_negotiation_complete);
}

static void TestMissingAndWrongPhyAreReported(void)
{
  fake_mdio_t fake;
  ksz8081_t phy;
  ksz8081_status_t status;
  ksz8081_bus_t bus;

  memset(&fake, 0, sizeof(fake));
  bus = FakeBus(&fake);
  assert(KSZ8081_Init(&phy, &bus) == KSZ8081_RESULT_OK);
  assert(KSZ8081_Scan(&phy, &status) == KSZ8081_RESULT_NOT_FOUND);
  assert(phy.not_found_count == 1U);

  InitializeKsz8081(&fake, 0U);
  fake.registers[KSZ8081_REG_PHYID2] = 0x1234U;
  assert(KSZ8081_Scan(&phy, &status) == KSZ8081_RESULT_ID_MISMATCH);
}

int main(void)
{
  TestScanAndStatus();
  TestMissingAndWrongPhyAreReported();
  puts("KSZ8081 protocol tests passed");
  return 0;
}
