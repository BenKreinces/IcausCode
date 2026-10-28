#include "ad7124.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
  bool chip_select_asserted;
  bool response_pending;
  bool fail_next_transfer;
  bool force_config_mismatch;
  uint8_t command;
  uint8_t registers[0x40U][3U];
  uint32_t transfer_count;
} fake_bus_t;

static void FakeSetBe16(uint8_t bytes[3], uint16_t value)
{
  bytes[0] = (uint8_t)(value >> 8);
  bytes[1] = (uint8_t)value;
}

static void FakeSetBe24(uint8_t bytes[3], uint32_t value)
{
  bytes[0] = (uint8_t)(value >> 16);
  bytes[1] = (uint8_t)(value >> 8);
  bytes[2] = (uint8_t)value;
}

static void FakeResetRegisters(fake_bus_t *fake)
{
  memset(fake->registers, 0, sizeof(fake->registers));
  fake->registers[AD7124_REG_STATUS][0] = 0x10U;
  fake->registers[AD7124_REG_ID][0] = AD7124_ID_EXPECTED;
  FakeSetBe16(fake->registers[AD7124_REG_ADC_CONTROL], 0x0000U);
  FakeSetBe16(fake->registers[AD7124_REG_CHANNEL_0], 0x8001U);
  FakeSetBe16(fake->registers[AD7124_REG_CONFIG_0], 0x0860U);
  FakeSetBe24(fake->registers[AD7124_REG_FILTER_0], 0x060180U);
  FakeSetBe24(fake->registers[AD7124_REG_DATA], 0xA00001U);
}

static void FakeSetChipSelect(void *context, bool asserted)
{
  ((fake_bus_t *)context)->chip_select_asserted = asserted;
}

static bool FakeTransfer(void *context,
                         const uint8_t *tx,
                         uint8_t *rx,
                         size_t length,
                         uint32_t timeout_ms)
{
  fake_bus_t *fake = (fake_bus_t *)context;

  assert(timeout_ms == 20U);
  assert(fake->chip_select_asserted);
  assert(tx != NULL);
  fake->transfer_count++;

  if (fake->fail_next_transfer)
  {
    fake->fail_next_transfer = false;
    return false;
  }

  if (length == 8U)
  {
    for (size_t index = 0U; index < length; index++) assert(tx[index] == 0xFFU);
    FakeResetRegisters(fake);
    return true;
  }

  if (fake->response_pending)
  {
    assert(rx != NULL);
    assert(length <= 3U);
    for (size_t index = 0U; index < length; index++)
    {
      assert(tx[index] == 0xFFU);
      rx[index] = fake->registers[fake->command][index];
    }
    if (fake->force_config_mismatch && (fake->command == AD7124_REG_CONFIG_0))
    {
      rx[1] ^= 0x01U;
    }
    fake->response_pending = false;
    return true;
  }

  if (length == 1U)
  {
    assert((tx[0] & 0x40U) != 0U);
    fake->command = tx[0] & 0x3FU;
    fake->response_pending = true;
    return true;
  }

  assert((tx[0] & 0x40U) == 0U);
  assert((length - 1U) <= 3U);
  for (size_t index = 1U; index < length; index++)
  {
    fake->registers[tx[0] & 0x3FU][index - 1U] = tx[index];
  }
  return true;
}

static ad7124_bus_t FakeBus(fake_bus_t *fake)
{
  ad7124_bus_t bus = {
      .context = fake,
      .set_chip_select = FakeSetChipSelect,
      .transfer = FakeTransfer,
      .delay_ms = NULL};
  return bus;
}

static void TestProbeAndDiagnostics(void)
{
  fake_bus_t fake;
  ad7124_t adc;
  ad7124_diagnostics_t diagnostics;
  ad7124_bus_t bus;
  uint8_t observed_id = 0U;

  memset(&fake, 0, sizeof(fake));
  FakeResetRegisters(&fake);
  bus = FakeBus(&fake);
  assert(AD7124_Init(&adc, &bus, 20U) == AD7124_RESULT_OK);
  assert(!fake.chip_select_asserted);
  assert(AD7124_Probe(&adc, &observed_id) == AD7124_RESULT_OK);
  assert(observed_id == AD7124_ID_EXPECTED);
  assert(adc.reset_count == 1U);
  assert(adc.register_read_count == 1U);

  assert(AD7124_ReadDiagnostics(&adc, &diagnostics) == AD7124_RESULT_OK);
  assert(diagnostics.status == 0x10U);
  assert(diagnostics.id == AD7124_ID_EXPECTED);
  assert(diagnostics.error == 0x000000U);
  assert(diagnostics.adc_control == 0x0000U);
  assert(diagnostics.channel_0 == 0x8001U);
  assert(diagnostics.config_0 == 0x0860U);
  assert(diagnostics.filter_0 == 0x060180U);
  assert(!fake.chip_select_asserted);
}

static ad7124_single_channel_config_t TestConfiguration(void)
{
  ad7124_single_channel_config_t configuration = {
      .adc_control = 0x0000U,
      .config_0 = 0x0860U,
      .filter_0 = 0x060180U,
      .setup = 0U,
      .input_positive = 2U,
      .input_negative = 3U};
  return configuration;
}

static void TestConfigurationAndSampling(void)
{
  fake_bus_t fake;
  ad7124_t adc;
  ad7124_bus_t bus;
  ad7124_sample_t sample;
  ad7124_single_channel_config_t configuration = TestConfiguration();

  memset(&fake, 0, sizeof(fake));
  FakeResetRegisters(&fake);
  bus = FakeBus(&fake);
  assert(AD7124_Init(&adc, &bus, 20U) == AD7124_RESULT_OK);
  assert(AD7124_ConfigureSingleChannel(&adc, &configuration) == AD7124_RESULT_OK);
  assert(adc.register_write_count == 20U);
  assert(fake.registers[AD7124_REG_CHANNEL_0][0] == 0x80U);
  assert(fake.registers[AD7124_REG_CHANNEL_0][1] == 0x43U);
  assert(fake.registers[AD7124_REG_CHANNEL_15][0] == 0x00U);
  assert(fake.registers[AD7124_REG_CHANNEL_15][1] == 0x01U);
  assert(AD7124_ReadSample(&adc, &sample) == AD7124_RESULT_OK);
  assert(sample.status == 0x10U);
  assert(sample.raw_code == 0xA00001U);
  assert(sample.signed_code == 0x200001L);
  assert(adc.sample_read_count == 1U);

  fake.registers[AD7124_REG_STATUS][0] = 0x90U;
  assert(AD7124_ReadSample(&adc, &sample) == AD7124_RESULT_NOT_READY);
  assert(adc.not_ready_count == 1U);
}

static void TestFailuresAreReported(void)
{
  fake_bus_t fake;
  ad7124_t adc;
  ad7124_bus_t bus;
  ad7124_single_channel_config_t configuration = TestConfiguration();
  uint8_t id = 0U;

  memset(&fake, 0, sizeof(fake));
  FakeResetRegisters(&fake);
  bus = FakeBus(&fake);
  assert(AD7124_Init(&adc, &bus, 20U) == AD7124_RESULT_OK);
  fake.fail_next_transfer = true;
  assert(AD7124_ReadRegister(&adc, AD7124_REG_ID, &id, sizeof(id)) ==
         AD7124_RESULT_BUS_ERROR);
  assert(adc.transport_error_count == 1U);

  fake.force_config_mismatch = true;
  assert(AD7124_ConfigureSingleChannel(&adc, &configuration) ==
         AD7124_RESULT_CONFIGURATION_MISMATCH);
  assert(adc.configuration_mismatch_count == 1U);
  assert(!fake.chip_select_asserted);
}

int main(void)
{
  TestProbeAndDiagnostics();
  TestConfigurationAndSampling();
  TestFailuresAreReported();
  puts("AD7124 protocol tests passed");
  return 0;
}
