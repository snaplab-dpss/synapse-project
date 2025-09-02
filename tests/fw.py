#!/usr/bin/env python3

from os import geteuid
from time import sleep, time

from util import *

PORTS = [p for p in range(3, 33)]
LAN_PORTS = [p for p in PORTS if p % 2 != 0]
WAN_PORTS = [p for p in PORTS if p not in LAN_PORTS]
CONNECTIONS = {p: (p + 1 if p % 2 != 0 else p - 1) for p in PORTS}
EXPIRATION_TIME_SEC = 1


def main():
    ports = Ports(PORTS)

    for lan_port in LAN_PORTS:
        wan_port = CONNECTIONS[lan_port]
        assert wan_port in WAN_PORTS

        lan_flow = build_flow()
        wan_flow = lan_flow.invert()

        lan_pkt = build_packet(flow=lan_flow)
        wan_pkt = build_packet(flow=wan_flow)

        print()
        print("========================================")
        print(f"LAN port {lan_port} <-> WAN port {wan_port}")
        print(f"Lan flow: {lan_flow} ({lan_flow.hex()})")
        print(f"Wan flow: {wan_flow} ({wan_flow.hex()})")
        print("========================================")

        print()
        print(f"[{lan_port}<->{wan_port}] Testing wan packet (port={wan_port}) is dropped...")
        ports.send(wan_port, wan_pkt)
        expect_no_packet(ports)

        print()
        print(f"[{lan_port}<->{wan_port}] Testing lan packet (port={lan_port}) is forwarded...")
        ports.send(lan_port, lan_pkt)
        expect_packet_from_port(ports, wan_port, lan_pkt)

        print()
        print(f"[{lan_port}<->{wan_port}] Testing wan packet (port={wan_port}) is now allowed...")
        ports.send(wan_port, wan_pkt)
        expect_packet_from_port(ports, lan_port, wan_pkt)

        print()
        print(f"[{lan_port}<->{wan_port}] Testing flow rejuvenation...")
        elapsed = 0
        start = time()
        while elapsed < EXPIRATION_TIME_SEC:
            ports.send(lan_port, lan_pkt)
            expect_packet_from_port(ports, wan_port, lan_pkt)
            ports.send(wan_port, wan_pkt)
            expect_packet_from_port(ports, lan_port, wan_pkt)
            sleep(0.1)
            elapsed = time() - start

        print()
        print(f"[{lan_port}<->{wan_port}] Testing flow expiration...")
        sleep(EXPIRATION_TIME_SEC * 3)
        ports.send(wan_port, wan_pkt)
        expect_no_packet(ports)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
