#!/usr/bin/env python3
"""
fwd: static forwarding by ingress device (stateless).

Reference: dpdk-nfs/fwd/fwd_main.c (nf_process returns get_dst_dev(device); no packet inspection),
built with the fwd rules in dpdk-nfs/fwd/Makefile that pair device 2k with 2k+1. With the
front-panel -> device mapping (port p -> device p-1), that pairs front-panel ports 3<->4, 5<->6,
..., 31<->32. Every packet is forwarded to the paired port untouched, regardless of type, since the
NF never looks past the ingress device.
"""

from util import *

NF = "synapse-fwd"

PAIR = {p: (p + 1 if p % 2 == 1 else p - 1) for p in NF_PORTS}


def test(ports: Ports) -> None:
    for p_in, p_out in PAIR.items():
        step(f"port {p_in} -> {p_out}: packet forwarded to the paired port, untouched")
        pkt = build_packet(flow=build_flow())
        ports.send(p_in, pkt)
        expect_packet_from_port(ports, p_out, pkt)

    step("forwarding ignores packet contents: TCP, ICMP and non-IPv4 frames are all forwarded")
    p_in, p_out = NF_PORTS[0], PAIR[NF_PORTS[0]]
    for pkt in (build_packet(flow=build_flow(), proto="tcp"), build_icmp_packet(), build_non_ip_packet()):
        ports.send(p_in, pkt)
        expect_packet_from_port(ports, p_out, pkt, [])


if __name__ == "__main__":
    run(test, NF)
