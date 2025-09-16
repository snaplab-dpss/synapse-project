#!/usr/bin/env python3

from os import geteuid
from time import sleep, time

from util import *

PORTS = [p for p in range(3, 33)]
LAN_PORTS = [p for p in PORTS if p % 2 != 0]
WAN_PORTS = [p for p in PORTS if p not in LAN_PORTS]
CONNECTIONS = {p: (p + 1 if p % 2 != 0 else p - 1) for p in PORTS}
EXPIRATION_TIME_SEC = 5
PUBLIC_IP = "1.2.3.4"


class PortAllocator:
    def __init__(self):
        self.capacity = 65536
        self.allocated = set()
        self.available = list(range(0, self.capacity))

    def allocate(self) -> int:
        port = self.available.pop(0)
        self.allocated.add(port)
        return port

    def free(self, port: int):
        assert port in self.allocated
        assert port not in self.available
        self.allocated.remove(port)
        self.available.insert(0, port)


def main():
    ports = Ports(PORTS)
    port_allocator = PortAllocator()

    for lan_port in LAN_PORTS:
        wan_port = CONNECTIONS[lan_port]
        assert wan_port in WAN_PORTS

        allocated_port = port_allocator.allocate()
        lan_egress_flow = build_flow()
        wan_ingress_flow = lan_egress_flow.clone(new_src_addr=PUBLIC_IP, new_src_port=bswap16(allocated_port))
        wan_egress_flow = wan_ingress_flow.invert()
        lan_ingress_flow = wan_egress_flow.clone(new_dst_addr=lan_egress_flow.src_addr, new_dst_port=lan_egress_flow.src_port)

        lan_egress_pkt = build_packet(flow=lan_egress_flow)
        wan_ingress_pkt = build_packet(flow=wan_ingress_flow)
        wan_egress_pkt = build_packet(flow=wan_egress_flow)
        lan_ingress_pkt = build_packet(flow=lan_ingress_flow)

        print()
        print("========================================")
        print(f"LAN port {lan_port} <-> WAN port {wan_port}")
        print(f"Lan eg flow: {lan_egress_flow} ({lan_egress_flow.hex()})")
        print(f"Wan ig flow: {wan_ingress_flow} ({wan_ingress_flow.hex()})")
        print(f"Wan eg flow: {wan_egress_flow} ({wan_egress_flow.hex()})")
        print(f"Lan ig flow: {lan_ingress_flow} ({lan_ingress_flow.hex()})")
        print("========================================")

        print()
        print(f"[{lan_port}<->{wan_port}] Testing wan packet (port={wan_port}) is dropped...")
        ports.send(wan_port, wan_egress_pkt)
        expect_no_packet(ports)

        print()
        print(f"[{lan_port}<->{wan_port}] Testing lan packet (port={lan_port}) is forwarded...")
        ports.send(lan_port, lan_egress_pkt)
        expect_packet_from_port(ports, wan_port, wan_ingress_pkt)

        print()
        print(f"[{lan_port}<->{wan_port}] Testing wan packet (port={wan_port}) is now allowed...")
        ports.send(wan_port, wan_egress_pkt)
        expect_packet_from_port(ports, lan_port, lan_ingress_pkt)

        print()
        print(f"[{lan_port}<->{wan_port}] Testing flow rejuvenation...")
        elapsed = 0
        start = time()
        while elapsed < EXPIRATION_TIME_SEC * 2:
            ports.send(lan_port, lan_egress_pkt)
            expect_packet_from_port(ports, wan_port, wan_ingress_pkt)
            ports.send(wan_port, wan_egress_pkt)
            expect_packet_from_port(ports, lan_port, lan_ingress_pkt)
            sleep(0.1)
            elapsed = time() - start

        print()
        print(f"[{lan_port}<->{wan_port}] Testing flow expiration...")
        sleep(EXPIRATION_TIME_SEC * 2)
        ports.send(wan_port, wan_egress_pkt)
        expect_no_packet(ports)

        port_allocator.free(allocated_port)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
