#!/usr/bin/env python3
"""
Send UDP to the STM32 and wait for echo reply. No Scapy/Npcap needed.
STM32 IP: 192.168.1.50, UDP echo port: 12345 (see lwip.c).
"""

import socket
import sys
import argparse

DEFAULT_IP = "192.168.1.50"
DEFAULT_PORT = 12345


def main():
    ap = argparse.ArgumentParser(description="UDP echo test to STM32")
    ap.add_argument("--ip", "-i", default=DEFAULT_IP, help=f"STM32 IP (default {DEFAULT_IP})")
    ap.add_argument("--port", "-p", type=int, default=DEFAULT_PORT,
                    help=f"UDP echo port (default {DEFAULT_PORT})")
    ap.add_argument("--payload", "-d", default="HELLO", help="Payload string (default HELLO)")
    ap.add_argument("--timeout", "-t", type=float, default=2.0, help="Receive timeout in seconds")
    args = ap.parse_args()

    payload = args.payload.encode("utf-8")
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(args.timeout)
    try:
        sock.sendto(payload, (args.ip, args.port))
        print(f"Sent {len(payload)} bytes to {args.ip}:{args.port}")
        data, addr = sock.recvfrom(4096)
        print(f"Reply from {addr}: {data!r}")
    except socket.timeout:
        print("No reply (timeout). Is the STM32 on the network and running UDP echo on this port?")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
    finally:
        sock.close()


if __name__ == "__main__":
    main()
