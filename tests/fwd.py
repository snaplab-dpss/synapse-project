#!/usr/bin/env python3

from os import geteuid

from util import Ports, build_flow, build_packet, expect_packet_from_port

RECIRCULATION_PORT = 6
PORTS = [p for p in range(3, 33)]
CONNECTIONS = {p: (p + 1 if p % 2 != 0 else p - 1) for p in PORTS}


def main():
    ports = Ports(PORTS)

    for p_in, p_out in CONNECTIONS.items():
        print(f"[*] Testing port {p_in} -> {p_out}...")

        flow = build_flow()
        pkt = build_packet(flow)

        ports.send(p_in, pkt)
        expect_packet_from_port(ports, p_out, pkt)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
