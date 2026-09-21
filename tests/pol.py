#!/usr/bin/env python3
"""
pol: per destination IP token bucket policer (a TNA DirectMeter on the flow table).

Reference: dpdk-nfs/pol/policer_main.c, built with the arguments in dpdk-nfs/pol/Makefile:
  - internal (LAN) devices are the even ones, so LAN = odd front panel ports (dev = port - 1);
  - forwarding rules pair device 2k with 2k+1, i.e. port p with p+1 (odd p);
  - buckets hold 131072 B and refill at 17179869184 B/s, table capacity 65536,
    a bucket expires after 1 s without traffic.

Semantics:
  - not IPv4, or IPv4 but not TCP/UDP: drop (from either side);
  - from LAN: always forwarded to the paired WAN port, never policed;
  - from WAN: a tracked destination is charged for the packet and forwarded, remarked to CS1
    (DSCP 8, so the TOS byte reads 0x20) if it is out of profile; an untracked destination starts
    a bucket and is forwarded uncharged.

Out of profile is unreachable here: the bucket refills at 17 GB/s, so a test would have to send
faster than that to empty it, and on the model packets are milliseconds apart. This script checks
everything else, and asserts the forwarded packets are NOT marked. Exercising the marking path
needs a BDD generated with a small POLICER_RATE and POLICER_BURST.
The first packet to an untracked destination takes the controller path in the synthesized solution.
"""

from time import sleep

from util import *

NF = "pol-f40000-c0-unif-hmax-tput"

LAN_PORTS = [p for p in NF_PORTS if p % 2 == 1]
WAN_OF = {p: p + 1 for p in LAN_PORTS}

EXPIRATION_SEC = 1.0
CPU_PATH_TIMEOUT = 3.0  # the first packet to an untracked destination goes through the controller

# POLICED_DSCP is 8 in policer_main.c, carried in the top six bits of the TOS byte.
POLICED_TOS = 0x20


def expect_unmarked(ports: Ports, port: int, pkt: Packet, timeout: float = DEFAULT_RX_TIMEOUT) -> None:
    """Forwarded whole and in profile: the policer left the DSCP alone."""
    received = expect_packet_from_port(ports, port, pkt, timeout=timeout)
    tos = received[IP].tos
    if tos & 0xFC == POLICED_TOS:
        raise TestFailure(f"packet from port {port} was marked out of profile (tos {hex(tos)}) but is within the rate")


def test(ports: Ports) -> None:
    for lan in LAN_PORTS:
        wan = WAN_OF[lan]
        step(f"LAN {lan} <-> WAN {wan}: LAN forwarded unpoliced, new destination admitted, then fast path")
        lan_pkt = build_packet(flow=build_flow())
        ports.send(lan, lan_pkt)
        expect_unmarked(ports, wan, lan_pkt)

        wan_pkt = build_packet(flow=build_flow())
        ports.send(wan, wan_pkt)
        expect_unmarked(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)
        ports.send(wan, wan_pkt)
        expect_unmarked(ports, lan, wan_pkt)

    lan, wan = LAN_PORTS[0], WAN_OF[LAN_PORTS[0]]

    step("the bucket is per destination: a second source to the same destination is already tracked")
    tracked = build_flow()
    ports.send(wan, build_packet(flow=tracked))
    expect_unmarked(ports, lan, build_packet(flow=tracked), timeout=CPU_PATH_TIMEOUT)

    other_source = tracked.clone(new_src_addr="9.9.9.9", new_src_port=4321)
    other_pkt = build_packet(flow=other_source)
    ports.send(wan, other_pkt)
    expect_unmarked(ports, lan, other_pkt)

    step("a different destination needs its own bucket, so it goes through the controller")
    other_dst = build_packet(flow=tracked.clone(new_dst_addr="8.8.8.8"))
    ports.send(wan, other_dst)
    expect_unmarked(ports, lan, other_dst, timeout=CPU_PATH_TIMEOUT)

    step("TCP flows are handled like UDP flows")
    tcp = build_packet(flow=build_flow(), proto="tcp")
    ports.send(wan, tcp)
    expect_unmarked(ports, lan, tcp, timeout=CPU_PATH_TIMEOUT)
    ports.send(wan, tcp)
    expect_unmarked(ports, lan, tcp)
    ports.send(lan, tcp)
    expect_unmarked(ports, wan, tcp)

    step("a packet that already carries a DSCP keeps it: the policer only marks what it drops")
    premarked = build_packet(flow=build_flow())
    premarked[IP].tos = 0x28
    ports.send(lan, premarked)
    expect_packet_from_port(ports, wan, premarked)

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

    step(f"a destination seen every {EXPIRATION_SEC / 2}s stays on the fast path; after {4 * EXPIRATION_SEC}s idle its bucket is gone")
    wan_pkt = build_packet(flow=build_flow())
    ports.send(wan, wan_pkt)
    expect_unmarked(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)
    for _ in range(int(4 * EXPIRATION_SEC / (EXPIRATION_SEC / 2))):
        sleep(EXPIRATION_SEC / 2)
        ports.send(wan, wan_pkt)
        expect_unmarked(ports, lan, wan_pkt)
    sleep(4 * EXPIRATION_SEC)
    ports.send(wan, wan_pkt)
    expect_unmarked(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)


if __name__ == "__main__":
    run(test, NF)
