#!/usr/bin/env python3

from os import geteuid

from util import *

PORTS = [p for p in range(3, 33)]


def main():
    ports = Ports(PORTS)
    chosen_port = PORTS[0]

    flow = build_flow()
    pkt = build_packet(flow=flow)

    ports.send(chosen_port, pkt)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
