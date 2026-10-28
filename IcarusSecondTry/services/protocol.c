#include "protocol.h"

#define HEADER_MAGIC_OFFSET 0U
#define HEADER_VERSION_OFFSET 2U
#define HEADER_TYPE_OFFSET 3U
#define HEADER_LENGTH_OFFSET 4U
#define HEADER_FLAGS_OFFSET 5U
#define HEADER_PAYLOAD_LENGTH_OFFSET 6U
#define HEADER_SEQUENCE_OFFSET 8U
#define HEADER_TIMESTAMP_OFFSET 12U
#define HEADER_CRC_OFFSET 16U

static void PutBe16(uint8_t *output, uint16_t value)
{
  output[0] = (uint8_t)(value >> 8U);
  output[1] = (uint8_t)value;
}

static void PutBe32(uint8_t *output, uint32_t value)
{
  output[0] = (uint8_t)(value >> 24U);
  output[1] = (uint8_t)(value >> 16U);
  output[2] = (uint8_t)(value >> 8U);
  output[3] = (uint8_t)value;
}

static uint16_t GetBe16(const uint8_t *input)
{
  return (uint16_t)(((uint16_t)input[0] << 8U) | input[1]);
}

static uint32_t GetBe32(const uint8_t *input)
{
  return ((uint32_t)input[0] << 24U) | ((uint32_t)input[1] << 16U) |
         ((uint32_t)input[2] << 8U) | input[3];
}

uint32_t IcarusProtocol_Crc32(const uint8_t *data, size_t length)
{
  uint32_t crc = UINT32_C(0xFFFFFFFF);
  size_t index;

  if ((data == NULL) && (length != 0U))
  {
    return 0U;
  }

  for (index = 0U; index < length; ++index)
  {
    uint32_t bit;
    crc ^= data[index];
    for (bit = 0U; bit < 8U; ++bit)
    {
      crc = ((crc & 1U) != 0U) ? ((crc >> 1U) ^ UINT32_C(0xEDB88320))
                                : (crc >> 1U);
    }
  }
  return ~crc;
}

bool IcarusProtocol_IsKnownType(uint8_t message_type)
{
  return (message_type >= ICARUS_MESSAGE_HEARTBEAT) &&
         (message_type <= ICARUS_MESSAGE_ECHO_RESPONSE);
}

static uint32_t PacketCrc(const uint8_t *packet, size_t payload_length)
{
  uint32_t crc = UINT32_C(0xFFFFFFFF);
  size_t index;

  /* CRC covers the fixed header through timestamp, skips the CRC field, then
   * covers the payload at byte 20. */
  for (index = 0U; index < HEADER_CRC_OFFSET + payload_length; ++index)
  {
    uint32_t bit;
    const uint8_t byte = (index < HEADER_CRC_OFFSET)
                             ? packet[index]
                             : packet[ICARUS_PROTOCOL_HEADER_LENGTH +
                                      (index - HEADER_CRC_OFFSET)];
    crc ^= byte;
    for (bit = 0U; bit < 8U; ++bit)
    {
      crc = ((crc & 1U) != 0U) ? ((crc >> 1U) ^ UINT32_C(0xEDB88320))
                                : (crc >> 1U);
    }
  }
  return ~crc;
}

icarus_protocol_result_t IcarusProtocol_Encode(
    const icarus_packet_header_t *header,
    const uint8_t *payload,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length)
{
  const size_t total_length = ICARUS_PROTOCOL_HEADER_LENGTH +
                              ((header != NULL) ? header->payload_length : 0U);

  if ((header == NULL) || (output == NULL) || (output_length == NULL) ||
      ((header->payload_length != 0U) && (payload == NULL)))
  {
    return ICARUS_PROTOCOL_ARGUMENT_ERROR;
  }
  if ((header->payload_length > ICARUS_PROTOCOL_MAX_PAYLOAD_LENGTH) ||
      !IcarusProtocol_IsKnownType(header->message_type))
  {
    return ICARUS_PROTOCOL_ARGUMENT_ERROR;
  }
  if (output_capacity < total_length)
  {
    return ICARUS_PROTOCOL_BUFFER_TOO_SMALL;
  }

  PutBe16(&output[HEADER_MAGIC_OFFSET], ICARUS_PROTOCOL_MAGIC);
  output[HEADER_VERSION_OFFSET] = ICARUS_PROTOCOL_VERSION;
  output[HEADER_TYPE_OFFSET] = header->message_type;
  output[HEADER_LENGTH_OFFSET] = ICARUS_PROTOCOL_HEADER_LENGTH;
  output[HEADER_FLAGS_OFFSET] = header->flags;
  PutBe16(&output[HEADER_PAYLOAD_LENGTH_OFFSET], header->payload_length);
  PutBe32(&output[HEADER_SEQUENCE_OFFSET], header->sequence);
  PutBe32(&output[HEADER_TIMESTAMP_OFFSET], header->timestamp_ms);
  if (header->payload_length != 0U)
  {
    for (size_t index = 0U; index < header->payload_length; ++index)
    {
      output[ICARUS_PROTOCOL_HEADER_LENGTH + index] = payload[index];
    }
  }
  PutBe32(&output[HEADER_CRC_OFFSET], PacketCrc(output, header->payload_length));
  *output_length = total_length;
  return ICARUS_PROTOCOL_OK;
}

icarus_protocol_result_t IcarusProtocol_Decode(
    const uint8_t *input,
    size_t input_length,
    icarus_packet_header_t *header,
    const uint8_t **payload)
{
  uint16_t payload_length;
  uint32_t received_crc;

  if ((input == NULL) || (header == NULL) || (payload == NULL))
  {
    return ICARUS_PROTOCOL_ARGUMENT_ERROR;
  }
  if (input_length < ICARUS_PROTOCOL_HEADER_LENGTH)
  {
    return ICARUS_PROTOCOL_BAD_LENGTH;
  }
  if (GetBe16(&input[HEADER_MAGIC_OFFSET]) != ICARUS_PROTOCOL_MAGIC)
  {
    return ICARUS_PROTOCOL_BAD_MAGIC;
  }
  if (input[HEADER_VERSION_OFFSET] != ICARUS_PROTOCOL_VERSION)
  {
    return ICARUS_PROTOCOL_BAD_VERSION;
  }
  if (!IcarusProtocol_IsKnownType(input[HEADER_TYPE_OFFSET]))
  {
    return ICARUS_PROTOCOL_BAD_TYPE;
  }
  if (input[HEADER_LENGTH_OFFSET] != ICARUS_PROTOCOL_HEADER_LENGTH)
  {
    return ICARUS_PROTOCOL_BAD_HEADER_LENGTH;
  }
  payload_length = GetBe16(&input[HEADER_PAYLOAD_LENGTH_OFFSET]);
  if ((payload_length > ICARUS_PROTOCOL_MAX_PAYLOAD_LENGTH) ||
      (input_length != (size_t)ICARUS_PROTOCOL_HEADER_LENGTH + payload_length))
  {
    return ICARUS_PROTOCOL_BAD_LENGTH;
  }
  received_crc = GetBe32(&input[HEADER_CRC_OFFSET]);
  if (received_crc != PacketCrc(input, payload_length))
  {
    return ICARUS_PROTOCOL_BAD_CRC;
  }

  header->message_type = input[HEADER_TYPE_OFFSET];
  header->flags = input[HEADER_FLAGS_OFFSET];
  header->payload_length = payload_length;
  header->sequence = GetBe32(&input[HEADER_SEQUENCE_OFFSET]);
  header->timestamp_ms = GetBe32(&input[HEADER_TIMESTAMP_OFFSET]);
  *payload = &input[ICARUS_PROTOCOL_HEADER_LENGTH];
  return ICARUS_PROTOCOL_OK;
}
