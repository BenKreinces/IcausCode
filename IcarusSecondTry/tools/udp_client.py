#!/usr/bin/env python3
"""Dependency-free Icarus UDP echo and command diagnostic."""
from __future__ import annotations

import argparse
import socket
import time

from icarus_protocol import ARM, COMMAND, COMMAND_ACK, ECHO_REQUEST, SAFE, Packet, decode_command_ack, decode_packet, encode_command, encode_packet


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="192.168.1.50")
    parser.add_argument("--port", type=int, default=5005)
    parser.add_argument("--timeout", type=float, default=1.0)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--echo", action="store_true")
    action.add_argument("--safe", action="store_true")
    action.add_argument("--arm", action="store_true")
    args = parser.parse_args()
    now = int(time.monotonic() * 1000) & 0xFFFFFFFF
    if args.echo:
        packet = Packet(ECHO_REQUEST, 1, now, b"icarus echo")
    else:
        packet = Packet(COMMAND, 1, now, encode_command(1, SAFE if args.safe else ARM))
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(args.timeout)
        sock.sendto(encode_packet(packet), (args.host, args.port))
        reply, source = sock.recvfrom(1024)
    decoded = decode_packet(reply)
    print(f"{source[0]}:{source[1]} type={decoded.message_type}")
    if decoded.message_type == COMMAND_ACK:
        print(decode_command_ack(decoded.payload))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
