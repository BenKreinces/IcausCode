#!/usr/bin/env python3
"""
Send raw Ethernet frames to the STM32 and (optionally) listen for echo replies.
Uses Scapy; on Windows install Npcap and: pip install scapy

STM32 MAC is 02:00:00:00:00:01 by default (see ethernetif.c).
Custom EtherType 0x88B5 is used so LwIP ignores the frame and firmware echoes it.
"""

import sys
import argparse

# EtherType we use so firmware echoes and LwIP ignores
ETHERTYPE_ECHO = 0x88B5

# Default STM32 MAC from ethernetif.c
STM32_MAC_DEFAULT = "02:00:00:00:00:01"


def get_local_mac(iface_name):
    """Get first interface MAC if no interface specified."""
    try:
        from scapy.all import get_if_hwaddr
        return get_if_hwaddr(iface_name)
    except Exception:
        return "02:00:00:00:00:02"  # fallback


def send_echo(iface, dst_mac, payload=b"HELLO", count=1):
    try:
        from scapy.all import Ether, Raw, sendp, sniff
    except ImportError:
        print("Install scapy: pip install scapy")
        print("On Windows also install Npcap for raw Ethernet: https://npcap.com/")
        sys.exit(1)

    src = get_local_mac(iface)
    # Ether(dst=..., src=..., type=...) / Raw(payload)
    frame = Ether(dst=dst_mac, src=src, type=ETHERTYPE_ECHO) / Raw(load=payload)
    print(f"Sending {count} frame(s) to {dst_mac} on {iface} (payload: {payload!r})")
    for _ in range(count):
        sendp(frame, iface=iface, verbose=0)
    print("Sent.")


def send_and_listen(iface, dst_mac, payload=b"HELLO", timeout=2):
    try:
        from scapy.all import Ether, Raw, sendp, sniff
    except ImportError:
        print("Install scapy: pip install scapy")
        print("On Windows also install Npcap: https://npcap.com/")
        sys.exit(1)

    src = get_local_mac(iface)
    frame = Ether(dst=dst_mac, src=src, type=ETHERTYPE_ECHO) / Raw(load=payload)
    print(f"Sending to {dst_mac} on {iface}, listening for {timeout}s...")
    sendp(frame, iface=iface, verbose=0)
    # Sniff for reply: same EtherType, dest = our MAC (src of what we sent)
    pkts = sniff(iface=iface, filter=f"ether proto {ETHERTYPE_ECHO}", timeout=timeout)
    for p in pkts:
        if p.haslayer(Raw):
            print(f"Reply: {bytes(p[Raw].load)!r}")
        else:
            print(f"Reply: (no payload)")
    if not pkts:
        print("No reply received.")
    return len(pkts)


def list_ifaces():
    try:
        from scapy.all import get_if_list
        for name in get_if_list():
            print(name)
    except ImportError:
        print("Install scapy: pip install scapy")


def main():
    ap = argparse.ArgumentParser(description="Raw Ethernet echo test to STM32")
    ap.add_argument("--iface", "-i", default=None, help="Interface name (e.g. Ethernet, eth0). Use --list to see.")
    ap.add_argument("--mac", "-m", default=STM32_MAC_DEFAULT, help=f"STM32 MAC (default {STM32_MAC_DEFAULT})")
    ap.add_argument("--payload", "-p", default=b"HELLO", type=lambda s: s.encode(), help="Payload bytes (default HELLO)")
    ap.add_argument("--count", "-c", type=int, default=1, help="Number of frames to send")
    ap.add_argument("--listen", "-l", action="store_true", help="After sending, sniff for echo reply")
    ap.add_argument("--timeout", "-t", type=float, default=2.0, help="Sniff timeout when --listen")
    ap.add_argument("--list", action="store_true", help="List interfaces and exit")
    args = ap.parse_args()

    if args.list:
        list_ifaces()
        return

    if not args.iface:
        print("Specify --iface (e.g. -i Ethernet). Use --list to see interfaces.")
        sys.exit(1)

    if args.listen:
        send_and_listen(args.iface, args.mac, args.payload, args.timeout)
    else:
        send_echo(args.iface, args.mac, args.payload, args.count)


if __name__ == "__main__":
    main()
