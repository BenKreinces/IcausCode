/**
 * @file protocol.h
 * @brief Versioned Icarus UDP packet framing and explicit serialization.
 *
 * No wire format in this interface is represented by a C structure.  All
 * multi-byte values are serialized in network byte order by protocol.c.
 */
#ifndef ICARUS_PROTOCOL_H
#define ICARUS_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ICARUS_PROTOCOL_MAGIC              UINT16_C(0x4943) /* "IC" */
#define ICARUS_PROTOCOL_VERSION            UINT8_C(1)
#define ICARUS_PROTOCOL_HEADER_LENGTH      UINT8_C(20)
#define ICARUS_PROTOCOL_MAX_PAYLOAD_LENGTH UINT16_C(512)

typedef enum
{
  ICARUS_MESSAGE_HEARTBEAT = 1,
  ICARUS_MESSAGE_COMMAND = 2,
  ICARUS_MESSAGE_COMMAND_ACK = 3,
  ICARUS_MESSAGE_TELEMETRY = 4,
  ICARUS_MESSAGE_ECHO_REQUEST = 5,
  ICARUS_MESSAGE_ECHO_RESPONSE = 6
} icarus_message_type_t;

typedef enum
{
  ICARUS_PROTOCOL_OK = 0,
  ICARUS_PROTOCOL_ARGUMENT_ERROR,
  ICARUS_PROTOCOL_BUFFER_TOO_SMALL,
  ICARUS_PROTOCOL_BAD_MAGIC,
  ICARUS_PROTOCOL_BAD_VERSION,
  ICARUS_PROTOCOL_BAD_TYPE,
  ICARUS_PROTOCOL_BAD_HEADER_LENGTH,
  ICARUS_PROTOCOL_BAD_LENGTH,
  ICARUS_PROTOCOL_BAD_CRC
} icarus_protocol_result_t;

/* Logical header only; it is never copied directly onto the wire. */
typedef struct
{
  uint8_t message_type;
  uint8_t flags;
  uint16_t payload_length;
  uint32_t sequence;
  uint32_t timestamp_ms;
} icarus_packet_header_t;

/** Encode header and payload into @p output.  Returns the total byte count. */
icarus_protocol_result_t IcarusProtocol_Encode(
    const icarus_packet_header_t *header,
    const uint8_t *payload,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length);

/** Validate and decode exactly one complete UDP datagram. */
icarus_protocol_result_t IcarusProtocol_Decode(
    const uint8_t *input,
    size_t input_length,
    icarus_packet_header_t *header,
    const uint8_t **payload);

uint32_t IcarusProtocol_Crc32(const uint8_t *data, size_t length);
bool IcarusProtocol_IsKnownType(uint8_t message_type);

#endif /* ICARUS_PROTOCOL_H */
