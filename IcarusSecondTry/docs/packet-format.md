# Icarus UDP protocol, version 1

This is the sole firmware/host wire contract. Every integer is unsigned or
two's-complement signed as stated and encoded in network byte order
(big-endian). No sender or receiver may send an in-memory C structure.

## Datagram frame

Every UDP datagram contains exactly one frame. Frames with trailing bytes,
truncated headers, unsupported versions/types, oversized payloads, or an
invalid CRC are rejected before dispatch.

| Offset | Width | Field |
|---:|---:|---|
| 0 | 2 | Magic `0x4943` (`IC`) |
| 2 | 1 | Version (`1`) |
| 3 | 1 | Message type |
| 4 | 1 | Header length (`20`) |
| 5 | 1 | Flags (zero until assigned) |
| 6 | 2 | Payload length, maximum 512 |
| 8 | 4 | Per-message-class sequence number |
| 12 | 4 | Monotonic firmware timestamp in milliseconds |
| 16 | 4 | CRC-32/ISO-HDLC over bytes 0–15 followed by payload |
| 20 | N | Payload |

Message types are heartbeat `1`, command `2`, command acknowledgement `3`,
telemetry `4`, echo request `5`, and echo response `6`.

## Commands and acknowledgements

A command payload begins with command ID (`u32`), opcode (`u8`), and argument
length (`u8`). The datagram payload length must exactly equal `6 + argument
length`.

| Opcode | Arguments | Meaning |
|---:|---|---|
| 1 | none | Force safe outputs |
| 2 | none | Arm only when interlocks are ready and no fault is active |
| 3 | `i32` RPM | Set motor target, range -12000 through 12000 RPM |
| 4 | `u8` index, `u8` open | Set solenoid 0–23 to closed/open |
| 5 | none | Clear fault only when interlocks are ready |

The acknowledgement payload is command ID (`u32`), result (`u8`), safety
state (`u8`), and fault mask (`u16`). Duplicates and stale command IDs are
acknowledged but never re-applied. Invalid input never reaches an actuator.

## Telemetry and heartbeat

Telemetry is lossy and may be reordered; consumers must use its frame sequence
and sample counter. Its fixed 142-byte payload contains a sample counter,
safety/link/task-health state, fault and validity masks, commanded/measured
motor RPM, solenoid state, raw load-cell code, 24 `i32` pressures in 0.1 kPa,
and four `i32` flows in mL/s. A validity bit is clear when a value is stale or
unavailable; consumers must not treat it as a zero measurement.

Heartbeat is separate from telemetry. Its 12-byte payload reports uptime,
last accepted command time, fault mask, safety state, and task-health mask.
It participates in host connection supervision, while the firmware's 500 ms
command timeout independently forces safe outputs.
