#!/usr/bin/env python3
"""
psd: port scan detector.

Reference: dpdk-nfs/psd/psd_main.c, built with the arguments in dpdk-nfs/psd/Makefile:
  - internal (LAN) devices are the even ones, so LAN = odd front panel ports (dev = port - 1);
  - forwarding rules pair device 2k with 2k+1, i.e. port p with p+1 (odd p);
  - source entries expire after 1 s, capacity 65536, max ports per source = 16,
    touched-port bloom filter 4x1024 cleaned every 10 s.

Semantics:
  - not IPv4, or IPv4 but not TCP/UDP: drop (from either side);
  - from LAN: always forwarded to the paired WAN port;
  - from WAN, keyed by src ip: an unknown source is installed with counter = 1 and its (src, dst port)
    marked in the bloom filter; a known source is refreshed (even if the packet ends up dropped), and
    if the (src, dst port) is not in the bloom filter the counter is incremented, unless it is
    already >= 16 in which case the packet is dropped. Everything else is forwarded to the paired
    LAN port.
So a source may reach 16 distinct destination ports; the 17th distinct port is dropped, while
already-touched ports keep passing. After the source entry expires the counter restarts at 1 (the
bloom filter is only cleaned every 10 s, so previously touched ports do not count again).
New sources and counter increments take the controller path in the synthesized solution.

The source entry expires after 1 s, so the scenario refreshes it (a touched port) after every
negative check and keeps those waits short.
"""

from time import sleep

from util import *

NF = "psd-f40000-c0-unif-hmax-tput"

LAN_PORTS = [p for p in NF_PORTS if p % 2 == 1]
WAN_OF = {p: p + 1 for p in LAN_PORTS}

EXPIRATION_SEC = 1.0
MAX_PORTS = 16
CPU_PATH_TIMEOUT = 3.0
DROP_WAIT = 0.5  # drops happen in the data plane, no need to wait a full second


def scan(ports: Ports, wan: int, src: str, dst_port: int, proto: str = "udp") -> Packet:
    pkt = build_packet(flow=build_flow(src_addr=src, dst_port=dst_port), proto=proto)
    ports.send(wan, pkt)
    return pkt


def expect_pass(ports: Ports, wan: int, lan: int, src: str, dst_port: int, proto: str = "udp") -> None:
    pkt = scan(ports, wan, src, dst_port, proto)
    expect_packet_from_port(ports, lan, pkt, timeout=CPU_PATH_TIMEOUT)


def expect_drop(ports: Ports, wan: int, src: str, dst_port: int, proto: str = "udp") -> None:
    scan(ports, wan, src, dst_port, proto)
    expect_no_packet(ports, timeout=DROP_WAIT)


def test(ports: Ports) -> None:
    for lan in LAN_PORTS:
        wan = WAN_OF[lan]
        step(f"LAN {lan} <-> WAN {wan}: LAN forwarded untouched, new WAN source admitted, then fast path")
        lan_pkt = build_packet(flow=build_flow())
        ports.send(lan, lan_pkt)
        expect_packet_from_port(ports, wan, lan_pkt)

        wan_pkt = build_packet(flow=build_flow())
        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)
        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt)

    lan, wan = LAN_PORTS[0], WAN_OF[LAN_PORTS[0]]
    src = build_flow().src_addr

    step("LAN traffic is never limited")
    for dst_port in range(7000, 7000 + MAX_PORTS + 4):
        pkt = build_packet(flow=build_flow(src_addr=src, dst_port=dst_port))
        ports.send(lan, pkt)
        expect_packet_from_port(ports, wan, pkt)

    # Everything about `src` below is kept within one TTL window: each touched port both passes and
    # refreshes the source entry, and the negative checks wait only DROP_WAIT (< TTL).
    step(f"a source touching {MAX_PORTS} distinct ports passes; further distinct ports are dropped")
    touched = list(range(5000, 5000 + MAX_PORTS))
    for dst_port in touched:
        expect_pass(ports, wan, lan, src, dst_port)
    expect_drop(ports, wan, src, 6000)

    step("already-touched ports keep passing for that source (and refresh it), new ones stay dropped")
    for dst_port in touched[:5]:
        expect_pass(ports, wan, lan, src, dst_port)
    expect_drop(ports, wan, src, 6001)

    step("TCP counts like UDP: a touched port over TCP passes, a new port over TCP is dropped")
    expect_pass(ports, wan, lan, src, touched[0], proto="tcp")
    expect_drop(ports, wan, src, 6002, proto="tcp")

    step("another source is counted independently")
    other = build_flow().src_addr
    for dst_port in (6000, 6001, 6002):
        expect_pass(ports, wan, lan, other, dst_port)

    step("IPv4 packets that are not TCP/UDP are dropped from both sides")
    ports.send(lan, build_icmp_packet())
    expect_no_packet(ports, timeout=DROP_WAIT)
    ports.send(wan, build_icmp_packet())
    expect_no_packet(ports, timeout=DROP_WAIT)

    step("non-IPv4 frames are dropped from both sides")
    ports.send(lan, build_non_ip_packet())
    expect_no_packet(ports, timeout=DROP_WAIT)
    ports.send(wan, build_non_ip_packet())
    expect_no_packet(ports, timeout=DROP_WAIT)

    step(f"after {4 * EXPIRATION_SEC}s idle the source entry expired: a new port is admitted again (counter restarted)")
    fresh = build_flow().src_addr
    for dst_port in range(8000, 8000 + MAX_PORTS):  # drive a fresh source to its limit
        expect_pass(ports, wan, lan, fresh, dst_port)
    expect_drop(ports, wan, fresh, 9000)
    sleep(4 * EXPIRATION_SEC)
    expect_pass(ports, wan, lan, fresh, 9001)


if __name__ == "__main__":
    run(test, NF)
