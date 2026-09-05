#!/usr/bin/env python3
"""
cl: connection limiter (per-client count-min sketch of new flows).

Reference: dpdk-nfs/cl/cl_main.c, built with the arguments in dpdk-nfs/cl/Makefile:
  - internal (LAN) devices are the even ones, so LAN = odd front panel ports (dev = port - 1);
  - forwarding rules pair device 2k with 2k+1, i.e. port p with p+1 (odd p);
  - flows expire after 1 s, table capacity 65536, sketch 4x1024 cleaned every 10 s,
    max clients per (src ip, dst ip) = 131071.

Semantics:
  - not IPv4, or IPv4 but not TCP/UDP: drop (from either side);
  - from LAN: always forwarded to the paired WAN port;
  - from WAN: a known 4-tuple is refreshed and forwarded; a new one increments the sketch for
    (src ip, dst ip) and is dropped only if the estimate exceeds 131071, otherwise the flow is
    installed (table full: still forwarded) and the packet forwarded to the paired LAN port.

With the limit at 131071 the limiting path is unreachable in a test (it needs 131072 new flows
from one client, each through the controller), so this script checks everything else. Exercising
the drop path needs a BDD generated with a small --max-clients.
The first packet of a new WAN flow takes the controller path in the synthesized solution.
"""

from time import sleep

from util import *

NF = "cl-f40000-c0-unif-hmax-tput"

LAN_PORTS = [p for p in NF_PORTS if p % 2 == 1]
WAN_OF = {p: p + 1 for p in LAN_PORTS}

EXPIRATION_SEC = 1.0
CPU_PATH_TIMEOUT = 3.0  # first packet of a new WAN flow goes through the controller


def test(ports: Ports) -> None:
    for lan in LAN_PORTS:
        wan = WAN_OF[lan]
        step(f"LAN {lan} <-> WAN {wan}: LAN forwarded untouched, new WAN flow admitted, then fast path")
        lan_pkt = build_packet(flow=build_flow())
        ports.send(lan, lan_pkt)
        expect_packet_from_port(ports, wan, lan_pkt)

        wan_pkt = build_packet(flow=build_flow())
        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)
        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt)

    lan, wan = LAN_PORTS[0], WAN_OF[LAN_PORTS[0]]

    step("many new WAN flows from the same client (src ip, dst ip) are all admitted (limit is 131071)")
    client = build_flow()
    for i in range(20):
        pkt = build_packet(flow=client.clone(new_src_port=1000 + i, new_dst_port=2000 + i))
        ports.send(wan, pkt)
        expect_packet_from_port(ports, lan, pkt, timeout=CPU_PATH_TIMEOUT)

    step("TCP flows are handled like UDP flows")
    tcp = build_packet(flow=build_flow(), proto="tcp")
    ports.send(wan, tcp)
    expect_packet_from_port(ports, lan, tcp, timeout=CPU_PATH_TIMEOUT)
    ports.send(wan, tcp)
    expect_packet_from_port(ports, lan, tcp)
    ports.send(lan, tcp)
    expect_packet_from_port(ports, wan, tcp)

    step("IPv4 packets that are not TCP/UDP are dropped from both sides")
    ports.send(lan, build_icmp_packet())
    expect_no_packet(ports)
    ports.send(wan, build_icmp_packet())
    expect_no_packet(ports)

    step("non-IPv4 frames are dropped from both sides")
    ports.send(lan, build_non_ip_packet())
    expect_no_packet(ports)
    ports.send(wan, build_non_ip_packet())
    expect_no_packet(ports)

    step(f"a WAN flow refreshed every {EXPIRATION_SEC / 2}s stays on the fast path; after {4 * EXPIRATION_SEC}s idle it is re-admitted")
    wan_pkt = build_packet(flow=build_flow())
    ports.send(wan, wan_pkt)
    expect_packet_from_port(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)
    for _ in range(int(4 * EXPIRATION_SEC / (EXPIRATION_SEC / 2))):
        sleep(EXPIRATION_SEC / 2)
        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt)
    sleep(4 * EXPIRATION_SEC)
    ports.send(wan, wan_pkt)
    expect_packet_from_port(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)


if __name__ == "__main__":
    run(test, NF)
