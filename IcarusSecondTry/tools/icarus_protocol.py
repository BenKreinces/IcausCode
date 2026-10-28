"""Version-1 Icarus UDP protocol codec shared by host diagnostics and GUIs."""

from __future__ import annotations

from dataclasses import dataclass
import struct
import zlib

MAGIC = 0x4943
VERSION = 1
HEADER_LENGTH = 20
MAX_PAYLOAD = 512

HEARTBEAT = 1
COMMAND = 2
COMMAND_ACK = 3
TELEMETRY = 4
ECHO_REQUEST = 5
ECHO_RESPONSE = 6

SAFE = 1
ARM = 2
SET_MOTOR_RPM = 3
SET_SOLENOID = 4
CLEAR_FAULT = 5


class ProtocolError(ValueError):
    """Raised when a datagram is not a complete valid Icarus frame."""


@dataclass(frozen=True)
class Packet:
    message_type: int
    sequence: int
    timestamp_ms: int
    payload: bytes
    flags: int = 0


def encode_packet(packet: Packet) -> bytes:
    if packet.message_type not in range(HEARTBEAT, ECHO_RESPONSE + 1):
        raise ProtocolError("unsupported message type")
    if len(packet.payload) > MAX_PAYLOAD:
        raise ProtocolError("payload exceeds protocol limit")
    if not all(0 <= value <= 0xFFFFFFFF for value in
               (packet.sequence, packet.timestamp_ms)):
        raise ProtocolError("sequence or timestamp out of range")
    if not 0 <= packet.flags <= 0xFF:
        raise ProtocolError("flags out of range")
    prefix = struct.pack("!HBBBBHII", MAGIC, VERSION, packet.message_type,
                         HEADER_LENGTH, packet.flags, len(packet.payload),
                         packet.sequence, packet.timestamp_ms)
    crc = zlib.crc32(prefix + packet.payload) & 0xFFFFFFFF
    return prefix + struct.pack("!I", crc) + packet.payload


def decode_packet(datagram: bytes) -> Packet:
    if len(datagram) < HEADER_LENGTH:
        raise ProtocolError("truncated header")
    magic, version, message_type, header_length, flags, payload_length, sequence, timestamp = (
        struct.unpack("!HBBBBHII", datagram[:16]))
    received_crc, = struct.unpack("!I", datagram[16:20])
    if magic != MAGIC:
        raise ProtocolError("bad magic")
    if version != VERSION:
        raise ProtocolError("unsupported version")
    if message_type not in range(HEARTBEAT, ECHO_RESPONSE + 1):
        raise ProtocolError("unsupported message type")
    if header_length != HEADER_LENGTH:
        raise ProtocolError("unsupported header length")
    if payload_length > MAX_PAYLOAD or len(datagram) != HEADER_LENGTH + payload_length:
        raise ProtocolError("invalid payload length")
    payload = datagram[HEADER_LENGTH:]
    calculated_crc = zlib.crc32(datagram[:16] + payload) & 0xFFFFFFFF
    if calculated_crc != received_crc:
        raise ProtocolError("bad CRC")
    return Packet(message_type, sequence, timestamp, payload, flags)


def encode_command(command_id: int, opcode: int, *, motor_rpm: int = 0,
                   solenoid_index: int = 0, solenoid_open: bool = False) -> bytes:
    if not 0 <= command_id <= 0xFFFFFFFF:
        raise ProtocolError("command ID out of range")
    if opcode in (SAFE, ARM, CLEAR_FAULT):
        arguments = b""
    elif opcode == SET_MOTOR_RPM:
        if not -12000 <= motor_rpm <= 12000:
            raise ProtocolError("motor RPM out of range")
        arguments = struct.pack("!i", motor_rpm)
    elif opcode == SET_SOLENOID:
        if not 0 <= solenoid_index < 24:
            raise ProtocolError("solenoid index out of range")
        arguments = bytes((solenoid_index, int(solenoid_open)))
    else:
        raise ProtocolError("unsupported command")
    return struct.pack("!IBB", command_id, opcode, len(arguments)) + arguments


def decode_command_ack(payload: bytes) -> tuple[int, int, int, int]:
    if len(payload) != 8:
        raise ProtocolError("invalid command acknowledgement length")
    return struct.unpack("!IBBH", payload)
