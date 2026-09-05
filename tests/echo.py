#!/usr/bin/env python3
"""
synapse-echo: every packet goes back out of the device it came in on, untouched.

Reference: dpdk-nfs/echo/echo_main.c (nf_process returns `device`).
"""

from util import *

NF = "synapse-echo"

UNUSED_PORTS = [p for p in ALL_PORTS if p not in NF_PORTS]


def test(ports: Ports) -> None:
    for port in NF_PORTS:
        step(f"port {port}: packet is echoed back on the same port")
        pkt = build_packet(flow=build_flow())
        ports.send(port, pkt)
        expect_packet_from_port(ports, port, pkt)

    step("a burst of distinct packets on one port comes back in order, nothing lost or duplicated")
    port = NF_PORTS[0]
    pkts = [build_packet(flow=build_flow()) for _ in range(10)]
    for pkt in pkts:
        ports.send(port, pkt)
    received = ports.collect()
    if [r.port for r in received] != [port] * len(pkts):
        raise TestFailure(f"expected {len(pkts)} packets from port {port}, got ports {[r.port for r in received]}")
    for expected, got in zip(pkts, received):
        assert_equal_packets(expected, got.pkt)

    for port in UNUSED_PORTS:
        step(f"port {port} is not wired to any NF device: packet is dropped")
        ports.send(port, build_packet(flow=build_flow()))
        expect_no_packet(ports)


if __name__ == "__main__":
    run(test, NF)
