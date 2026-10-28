/** @file command.h @brief Explicit command and acknowledgment payloads. */
#ifndef ICARUS_COMMAND_H
#define ICARUS_COMMAND_H

#include <stddef.h>
#include <stdint.h>

#include "protocol.h"

#define ICARUS_COMMAND_MAX_SOLENOIDS UINT8_C(24)
#define ICARUS_COMMAND_MAX_MOTOR_RPM INT32_C(12000)

typedef enum
{
  ICARUS_COMMAND_SAFE = 1,
  ICARUS_COMMAND_ARM = 2,
  ICARUS_COMMAND_SET_MOTOR_RPM = 3,
  ICARUS_COMMAND_SET_SOLENOID = 4,
  ICARUS_COMMAND_CLEAR_FAULT = 5
} icarus_command_opcode_t;

typedef enum
{
  ICARUS_COMMAND_ACCEPTED = 0,
  ICARUS_COMMAND_REJECTED_MALFORMED = 1,
  ICARUS_COMMAND_REJECTED_STATE = 2,
  ICARUS_COMMAND_REJECTED_RANGE = 3,
  ICARUS_COMMAND_REJECTED_STALE = 4,
  ICARUS_COMMAND_REJECTED_DUPLICATE = 5,
  ICARUS_COMMAND_REJECTED_INTERLOCK = 6
} icarus_command_result_t;

typedef struct
{
  uint32_t command_id;
  uint8_t opcode;
  int32_t motor_rpm;
  uint8_t solenoid_index;
  uint8_t solenoid_open;
} icarus_command_t;

typedef struct
{
  uint32_t command_id;
  uint8_t result;
  uint8_t safety_state;
  uint16_t fault_mask;
} icarus_command_ack_t;

icarus_command_result_t IcarusCommand_Decode(const uint8_t *payload,
                                              size_t payload_length,
                                              icarus_command_t *command);
icarus_command_result_t IcarusCommand_Encode(const icarus_command_t *command,
                                              uint8_t *payload,
                                              size_t payload_capacity,
                                              size_t *payload_length);
void IcarusCommand_EncodeAck(const icarus_command_ack_t *ack,
                             uint8_t payload[8]);

#endif /* ICARUS_COMMAND_H */
