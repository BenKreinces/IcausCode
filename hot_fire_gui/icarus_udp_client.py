"""Icarus protocol client used by the hot-fire GUI control layer."""
from __future__ import annotations

import pathlib
import socket
import sys
import time

_TOOLS = pathlib.Path(__file__).resolve().parents[1] / "IcarusSecondTry" / "tools"
sys.path.insert(0, str(_TOOLS))
from icarus_protocol import COMMAND, COMMAND_ACK, Packet, decode_command_ack, decode_packet, encode_packet


class IcarusUdpClient:
    def __init__(self, host="192.168.1.50", port=5005, timeout=0.5):
        self.host, self.port, self.timeout, self.sequence = host, port, timeout, 0

    def command(self, payload: bytes):
        self.sequence = (self.sequence + 1) & 0xFFFFFFFF
        packet = Packet(COMMAND, self.sequence,
                        int(time.monotonic() * 1000) & 0xFFFFFFFF, payload)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.settimeout(self.timeout)
            sock.sendto(encode_packet(packet), (self.host, self.port))
            reply, _ = sock.recvfrom(1024)
        response = decode_packet(reply)
        if response.message_type != COMMAND_ACK:
            raise RuntimeError("unexpected Icarus response")
        return decode_command_ack(response.payload)
