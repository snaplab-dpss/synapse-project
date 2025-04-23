#!/usr/bin/env python3

from os import geteuid

from util import Ports, build_flow, build_packet, expect_packet_from_port

RECIRCULATION_PORT = 6
PORTS = [p for p in range(3, 33)]


def main():
    ports = Ports(PORTS)

    for port in PORTS:
        print(f"[*] Testing port {port}...")

        flow = build_flow()
        pkt = build_packet(flow=flow)

        ports.send(port, pkt)
        expect_packet_from_port(ports, port, pkt)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
