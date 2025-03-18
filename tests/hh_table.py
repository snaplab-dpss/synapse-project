#!/usr/bin/env python3

from os import geteuid
from time import sleep, time
from random import choice

from util import *

PORTS = [p for p in range(3, 33)]


def main():
    ports = Ports(PORTS)

    in_port = 3

    total_heavy_hitter_flows = 60
    total_mice_flows = 20

    heavy_hitter_flows = [build_flow() for _ in range(total_heavy_hitter_flows)]
    mice_flows = [build_flow() for _ in range(total_mice_flows)]

    print("Heavy Hitter Flows:")
    for i, flow in enumerate(heavy_hitter_flows):
        print(f"[({i+1}) Flow: {flow} ({flow.hex()})")

    print("Sending traffic, press Ctrl+C to stop...")

    while True:
        packets = [build_packet(choice(heavy_hitter_flows)) for _ in range(10)]
        packets += build_packet(choice(mice_flows))

        for pkt in packets:
            ports.send(in_port, pkt, quiet=True)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
