/**
 * @file ad7124.c
 * @brief AD7124-8 register protocol implementation.
 */
#include "ad7124.h"

#define AD7124_COMMAND_READ_MASK 0x40U
#define AD7124_COMMAND_ADDR_MASK 0x3FU
#define AD7124_RESET_BYTE        0xFFU
#define AD7124_RESET_BYTE_COUNT  8U
#define AD7124_POST_RESET_DELAY_MS 1U

static bool AD7124_HasValidBus(const ad7124_t *device)
{
  return (device != NULL) &&
         (device->bus.set_chip_select != NULL) &&
         (device->bus.transfer != NULL);
}

static ad7124_result_t AD7124_Transfer(ad7124_t *device,
                                        const uint8_t *tx,
                                        uint8_t *rx,
                                        size_t length)
{
  if (!device->bus.transfer(device->bus.context,
                            tx,
                            rx,
                            length,
                            device->transfer_timeout_ms))
  {
    device->transport_error_count++;
    return AD7124_RESULT_BUS_ERROR;
  }

  return AD7124_RESULT_OK;
}

static uint16_t AD7124_ReadBe16(const uint8_t value[2])
{
  return ((uint16_t)value[0] << 8) | value[1];
}

static uint32_t AD7124_ReadBe24(const uint8_t value[3])
{
  return ((uint32_t)value[0] << 16) |
         ((uint32_t)value[1] << 8) |
         value[2];
}

static void AD7124_WriteBe16(uint16_t value, uint8_t bytes[2])
{
  bytes[0] = (uint8_t)(value >> 8);
  bytes[1] = (uint8_t)value;
}

static void AD7124_WriteBe24(uint32_t value, uint8_t bytes[3])
{
  bytes[0] = (uint8_t)(value >> 16);
  bytes[1] = (uint8_t)(value >> 8);
  bytes[2] = (uint8_t)value;
}

static uint16_t AD7124_MakeChannelRegister(
    const ad7124_single_channel_config_t *configuration)
{
  return (uint16_t)(0x8000U |
                    ((uint16_t)configuration->setup << 12) |
                    ((uint16_t)configuration->input_positive << 5) |
                    configuration->input_negative);
}

static ad7124_result_t AD7124_ReadRegisterBe16(ad7124_t *device,
                                                uint8_t address,
                                                uint16_t *value)
{
  uint8_t bytes[2];
  ad7124_result_t result;

  result = AD7124_ReadRegister(device, address, bytes, sizeof(bytes));
  if (result == AD7124_RESULT_OK)
  {
    *value = AD7124_ReadBe16(bytes);
  }
  return result;
}

static ad7124_result_t AD7124_ReadRegisterBe24(ad7124_t *device,
                                                uint8_t address,
                                                uint32_t *value)
{
  uint8_t bytes[3];
  ad7124_result_t result;

  result = AD7124_ReadRegister(device, address, bytes, sizeof(bytes));
  if (result == AD7124_RESULT_OK)
  {
    *value = AD7124_ReadBe24(bytes);
  }
  return result;
}

ad7124_result_t AD7124_Init(ad7124_t *device,
                             const ad7124_bus_t *bus,
                             uint32_t transfer_timeout_ms)
{
  if ((device == NULL) || (bus == NULL) ||
      (bus->set_chip_select == NULL) || (bus->transfer == NULL) ||
      (transfer_timeout_ms == 0U))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  device->bus = *bus;
  device->transfer_timeout_ms = transfer_timeout_ms;
  device->reset_count = 0U;
  device->register_read_count = 0U;
  device->register_write_count = 0U;
  device->sample_read_count = 0U;
  device->not_ready_count = 0U;
  device->transport_error_count = 0U;
  device->id_mismatch_count = 0U;
  device->configuration_mismatch_count = 0U;
  device->bus.set_chip_select(device->bus.context, false);

  return AD7124_RESULT_OK;
}

ad7124_result_t AD7124_Reset(ad7124_t *device)
{
  uint8_t reset_bytes[AD7124_RESET_BYTE_COUNT] = {
      AD7124_RESET_BYTE, AD7124_RESET_BYTE, AD7124_RESET_BYTE,
      AD7124_RESET_BYTE, AD7124_RESET_BYTE, AD7124_RESET_BYTE,
      AD7124_RESET_BYTE, AD7124_RESET_BYTE};
  ad7124_result_t result;

  if (!AD7124_HasValidBus(device))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  device->bus.set_chip_select(device->bus.context, true);
  result = AD7124_Transfer(device, reset_bytes, NULL, sizeof(reset_bytes));
  device->bus.set_chip_select(device->bus.context, false);

  if (result == AD7124_RESULT_OK)
  {
    device->reset_count++;
    if (device->bus.delay_ms != NULL)
    {
      device->bus.delay_ms(device->bus.context, AD7124_POST_RESET_DELAY_MS);
    }
  }

  return result;
}

ad7124_result_t AD7124_ReadRegister(ad7124_t *device,
                                     uint8_t address,
                                     uint8_t *value,
                                     size_t value_length)
{
  uint8_t command;
  uint8_t read_clocks[3] = {AD7124_RESET_BYTE, AD7124_RESET_BYTE, AD7124_RESET_BYTE};
  ad7124_result_t result;

  if (!AD7124_HasValidBus(device) || (value == NULL) ||
      (address > AD7124_COMMAND_ADDR_MASK) ||
      (value_length == 0U) || (value_length > sizeof(read_clocks)))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  command = AD7124_COMMAND_READ_MASK | (address & AD7124_COMMAND_ADDR_MASK);
  device->bus.set_chip_select(device->bus.context, true);
  result = AD7124_Transfer(device, &command, NULL, sizeof(command));
  if (result == AD7124_RESULT_OK)
  {
    result = AD7124_Transfer(device, read_clocks, value, value_length);
  }
  device->bus.set_chip_select(device->bus.context, false);

  if (result == AD7124_RESULT_OK)
  {
    device->register_read_count++;
  }

  return result;
}

ad7124_result_t AD7124_WriteRegister(ad7124_t *device,
                                      uint8_t address,
                                      const uint8_t *value,
                                      size_t value_length)
{
  uint8_t transaction[4];
  ad7124_result_t result;

  if (!AD7124_HasValidBus(device) || (value == NULL) ||
      (address > AD7124_COMMAND_ADDR_MASK) ||
      (value_length == 0U) || (value_length > (sizeof(transaction) - 1U)))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  transaction[0] = address & AD7124_COMMAND_ADDR_MASK;
  for (size_t index = 0U; index < value_length; index++)
  {
    transaction[index + 1U] = value[index];
  }

  device->bus.set_chip_select(device->bus.context, true);
  result = AD7124_Transfer(device, transaction, NULL, value_length + 1U);
  device->bus.set_chip_select(device->bus.context, false);

  if (result == AD7124_RESULT_OK)
  {
    device->register_write_count++;
  }

  return result;
}

ad7124_result_t AD7124_Probe(ad7124_t *device, uint8_t *observed_id)
{
  ad7124_result_t result;
  uint8_t id = 0U;

  if ((device == NULL) || (observed_id == NULL))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  result = AD7124_Reset(device);
  if (result != AD7124_RESULT_OK)
  {
    return result;
  }

  result = AD7124_ReadRegister(device, AD7124_REG_ID, &id, sizeof(id));
  *observed_id = id;
  if (result != AD7124_RESULT_OK)
  {
    return result;
  }

  if (id != AD7124_ID_EXPECTED)
  {
    device->id_mismatch_count++;
    return AD7124_RESULT_ID_MISMATCH;
  }

  return AD7124_RESULT_OK;
}

ad7124_result_t AD7124_ConfigureSingleChannel(
    ad7124_t *device,
    const ad7124_single_channel_config_t *configuration)
{
  uint8_t value_16[2];
  uint8_t value_24[3];
  uint8_t observed_id;
  uint16_t observed_16;
  uint32_t observed_24;
  const uint16_t disabled_channel = 0x0001U;
  uint16_t enabled_channel;
  ad7124_result_t result;

  if ((device == NULL) || (configuration == NULL) ||
      (configuration->setup > 7U) ||
      (configuration->input_positive > 15U) ||
      (configuration->input_negative > 15U) ||
      (configuration->filter_0 > 0xFFFFFFU))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  enabled_channel = AD7124_MakeChannelRegister(configuration);

  result = AD7124_Probe(device, &observed_id);
  if (result != AD7124_RESULT_OK)
  {
    return result;
  }

  AD7124_WriteBe16(disabled_channel, value_16);
  for (uint8_t channel = AD7124_REG_CHANNEL_0;
       channel <= AD7124_REG_CHANNEL_15;
       channel++)
  {
    result = AD7124_WriteRegister(device, channel, value_16, sizeof(value_16));
    if (result != AD7124_RESULT_OK)
    {
      return result;
    }
  }

  for (uint8_t channel = AD7124_REG_CHANNEL_0;
       channel <= AD7124_REG_CHANNEL_15;
       channel++)
  {
    result = AD7124_ReadRegisterBe16(device, channel, &observed_16);
    if ((result != AD7124_RESULT_OK) || (observed_16 != disabled_channel)) goto mismatch;
  }

  AD7124_WriteBe16(configuration->config_0, value_16);
  result = AD7124_WriteRegister(device, AD7124_REG_CONFIG_0,
                                value_16, sizeof(value_16));
  if (result != AD7124_RESULT_OK) return result;

  AD7124_WriteBe24(configuration->filter_0, value_24);
  result = AD7124_WriteRegister(device, AD7124_REG_FILTER_0,
                                value_24, sizeof(value_24));
  if (result != AD7124_RESULT_OK) return result;

  AD7124_WriteBe16(configuration->adc_control, value_16);
  result = AD7124_WriteRegister(device, AD7124_REG_ADC_CONTROL,
                                value_16, sizeof(value_16));
  if (result != AD7124_RESULT_OK) return result;

  AD7124_WriteBe16(enabled_channel, value_16);
  result = AD7124_WriteRegister(device, AD7124_REG_CHANNEL_0,
                                value_16, sizeof(value_16));
  if (result != AD7124_RESULT_OK) return result;

  result = AD7124_ReadRegisterBe16(device, AD7124_REG_ADC_CONTROL, &observed_16);
  if ((result != AD7124_RESULT_OK) || (observed_16 != configuration->adc_control)) goto mismatch;

  result = AD7124_ReadRegisterBe16(device, AD7124_REG_CONFIG_0, &observed_16);
  if ((result != AD7124_RESULT_OK) || (observed_16 != configuration->config_0)) goto mismatch;

  result = AD7124_ReadRegisterBe24(device, AD7124_REG_FILTER_0, &observed_24);
  if ((result != AD7124_RESULT_OK) || (observed_24 != configuration->filter_0)) goto mismatch;

  result = AD7124_ReadRegisterBe16(device, AD7124_REG_CHANNEL_0, &observed_16);
  if ((result != AD7124_RESULT_OK) || (observed_16 != enabled_channel)) goto mismatch;

  return AD7124_RESULT_OK;

mismatch:
  if (result == AD7124_RESULT_OK)
  {
    device->configuration_mismatch_count++;
    return AD7124_RESULT_CONFIGURATION_MISMATCH;
  }
  return result;
}

ad7124_result_t AD7124_ReadSample(ad7124_t *device, ad7124_sample_t *sample)
{
  uint8_t data[3];
  ad7124_result_t result;

  if ((device == NULL) || (sample == NULL))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  result = AD7124_ReadRegister(device, AD7124_REG_STATUS, &sample->status, 1U);
  if (result != AD7124_RESULT_OK)
  {
    return result;
  }

  if ((sample->status & 0x80U) != 0U)
  {
    device->not_ready_count++;
    return AD7124_RESULT_NOT_READY;
  }

  result = AD7124_ReadRegister(device, AD7124_REG_DATA, data, sizeof(data));
  if (result != AD7124_RESULT_OK)
  {
    return result;
  }

  sample->raw_code = AD7124_ReadBe24(data);
  sample->signed_code = (int32_t)sample->raw_code - 0x800000L;
  device->sample_read_count++;
  return AD7124_RESULT_OK;
}

ad7124_result_t AD7124_ReadDiagnostics(ad7124_t *device,
                                        ad7124_diagnostics_t *diagnostics)
{
  ad7124_result_t result;
  uint8_t value_16[2];
  uint8_t value_24[3];

  if ((device == NULL) || (diagnostics == NULL))
  {
    return AD7124_RESULT_ARGUMENT;
  }

  result = AD7124_ReadRegister(device, AD7124_REG_STATUS,
                                &diagnostics->status, 1U);
  if (result != AD7124_RESULT_OK) return result;

  result = AD7124_ReadRegister(device, AD7124_REG_ID, &diagnostics->id, 1U);
  if (result != AD7124_RESULT_OK) return result;
  if (diagnostics->id != AD7124_ID_EXPECTED)
  {
    device->id_mismatch_count++;
    return AD7124_RESULT_ID_MISMATCH;
  }

  result = AD7124_ReadRegister(device, AD7124_REG_ERROR, value_24, sizeof(value_24));
  if (result != AD7124_RESULT_OK) return result;
  diagnostics->error = AD7124_ReadBe24(value_24);

  result = AD7124_ReadRegister(device, AD7124_REG_ADC_CONTROL,
                                value_16, sizeof(value_16));
  if (result != AD7124_RESULT_OK) return result;
  diagnostics->adc_control = AD7124_ReadBe16(value_16);

  result = AD7124_ReadRegister(device, AD7124_REG_CHANNEL_0,
                                value_16, sizeof(value_16));
  if (result != AD7124_RESULT_OK) return result;
  diagnostics->channel_0 = AD7124_ReadBe16(value_16);

  result = AD7124_ReadRegister(device, AD7124_REG_CONFIG_0,
                                value_16, sizeof(value_16));
  if (result != AD7124_RESULT_OK) return result;
  diagnostics->config_0 = AD7124_ReadBe16(value_16);

  result = AD7124_ReadRegister(device, AD7124_REG_FILTER_0,
                                value_24, sizeof(value_24));
  if (result != AD7124_RESULT_OK) return result;
  diagnostics->filter_0 = AD7124_ReadBe24(value_24);

  return AD7124_RESULT_OK;
}
