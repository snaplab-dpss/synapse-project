#!/usr/bin/env python3

from os import geteuid
from time import sleep, time

from util import *

PORTS = [p for p in range(3, 33)]


def main():
    ports = Ports(PORTS)

    in_port = 3
    hit_port = 3
    miss_port = 4

    flow = build_flow()
    pkt = build_packet(flow)

    print(f"Flow: {flow} ({flow.hex()})")

    for _ in range(5):
        ports.send(in_port, pkt)
        expect_packet_from_port(ports, miss_port, pkt)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
