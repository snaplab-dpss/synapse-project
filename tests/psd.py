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
    marked in the bloom filter; a known source is refreshed, and if the (src, dst port) is not in the
    bloom filter the counter is incremented, unless it is already >= 16 in which case the packet is
    dropped. Everything else is forwarded to the paired LAN port.

APPROXIMATION: the "distinct ports touched" count is a bloom filter, so it can have false positives
-- a genuinely-new port may read as already-seen, so it is forwarded without incrementing the
counter. The limit therefore engages *around* 16 distinct ports, not exactly at the 17th, and more
so once the shared bloom has accumulated entries (the max-tput dataplane bloom and the gallium
controller-offloaded bloom both behave this way). So the limit checks below are tolerant: the first
several new ports must pass and the limit must clearly engage within a margin, rather than pinning
the exact boundary packet. Touched-port passing and expiry (counter reset) are still exact.
New sources and counter increments take the controller path in the synthesized solution.
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

# Tolerant limit probe: send well past the limit and require the limit to engage within a margin.
SCAN_COUNT = 24  # distinct new ports to send when probing the limit
EARLY_PASS = 10  # the first this-many new ports must pass (limit is not absurdly low)
MIN_LIMITED = 3  # at least this many of SCAN_COUNT must be dropped (the limit really engages)
# Per-port wait when scanning MUST be shorter than the source TTL: a forward returns in ~0.2 s, and
# a drop produces nothing, so a longer wait would let the source expire during a run of drops (each
# packet, drop included, refreshes the source) and the counter would restart mid-scan.
SCAN_TIMEOUT = 0.7


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


def drive_new_ports(ports: Ports, wan: int, lan: int, src: str, base_port: int, count: int, proto: str = "udp") -> list:
    """Send `count` distinct new ports from `src`; return a list of bools (True = forwarded to lan).

    Replies come back over the controller (CPU path) and can lag, so each port is classified by
    matching the reply's destination port rather than by whatever happens to arrive next."""
    l4 = TCP if proto == "tcp" else UDP
    forwarded = []
    for i in range(count):
        dst_port = base_port + i
        ports.drain()
        scan(ports, wan, src, dst_port, proto)
        received = ports.collect(SCAN_TIMEOUT)
        got = any(r.port == lan and l4 in r.pkt and r.pkt[l4].dport == dst_port for r in received)
        forwarded.append(got)
    return forwarded


def assert_rate_limited(forwarded: list, label: str) -> None:
    if not all(forwarded[:EARLY_PASS]):
        raise TestFailure(f"{label}: the first {EARLY_PASS} distinct ports must pass, got {forwarded[:EARLY_PASS]}")
    dropped = forwarded.count(False)
    if dropped < MIN_LIMITED:
        raise TestFailure(f"{label}: limit did not engage over {len(forwarded)} distinct ports (only {dropped} dropped)")


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

    step(f"a source scanning {SCAN_COUNT} distinct UDP ports is rate-limited (first {EARLY_PASS} pass, then drops engage)")
    assert_rate_limited(drive_new_ports(ports, wan, lan, src, 5000, SCAN_COUNT), "udp scan")

    step(f"TCP is counted like UDP: a fresh source scanning {SCAN_COUNT} distinct TCP ports is also rate-limited")
    tcp_src = build_flow().src_addr
    assert_rate_limited(drive_new_ports(ports, wan, lan, tcp_src, 5000, SCAN_COUNT, proto="tcp"), "tcp scan")

    step("another source is counted independently: its first ports pass")
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

    step(f"a fresh source is rate-limited, then after {4 * EXPIRATION_SEC}s idle its entry expires and it is admitted again")
    fresh = build_flow().src_addr
    forwarded = drive_new_ports(ports, wan, lan, fresh, 8000, SCAN_COUNT)
    assert_rate_limited(forwarded, "fresh-source scan")
    sleep(4 * EXPIRATION_SEC)  # source entry expires -> counter resets
    expect_pass(ports, wan, lan, fresh, 9001)


if __name__ == "__main__":
    run(test, NF)
