#!/usr/bin/env python3
"""
fw: stateful firewall.

Reference: dpdk-nfs/fw/fw_main.c + flowmanager.c, built with the arguments in dpdk-nfs/fw/Makefile:
  - internal (LAN) devices are the even ones, so LAN = odd front panel ports (dev = port - 1);
  - forwarding rules pair device 2k with 2k+1, i.e. port p with p+1 (odd p);
  - flows expire after 1 s (--expire 1000000 us), table capacity 65536.

Semantics:
  - not IPv4, or IPv4 but not TCP/UDP: drop (from either side);
  - from LAN: remember the 4-tuple (src ip, dst ip, src port, dst port; no protocol!) and forward
    to the paired WAN port (even if the table is full, the packet still goes out);
  - from WAN: forward to the paired LAN port only if the *inverted* 4-tuple is known, refreshing it;
    drop otherwise.
The first packet of a new flow takes the controller path in the synthesized solution.
"""

from time import sleep

from util import *

NF = "fw-f40000-c0-unif-hmax-tput"

LAN_PORTS = [p for p in NF_PORTS if p % 2 == 1]
WAN_OF = {p: p + 1 for p in LAN_PORTS}

EXPIRATION_SEC = 1.0
CPU_PATH_TIMEOUT = 3.0  # first packet of a flow goes through the controller

# Flows expire after 1s, and every negative check ("no packet") waits ~1s, so a flow must be
# refreshed (refresh_flow) before any positive check that follows negative ones.


def lan_creates_flow(ports: Ports, lan: int, wan: int) -> tuple:
    """Returns (lan_pkt, wan_reply_pkt) for a fresh flow that has just been installed by a LAN packet."""
    flow = build_flow()
    lan_pkt = build_packet(flow=flow)
    wan_pkt = build_packet(flow=flow.invert())

    ports.send(wan, wan_pkt)
    expect_no_packet(ports)

    ports.send(lan, lan_pkt)
    expect_packet_from_port(ports, wan, lan_pkt, timeout=CPU_PATH_TIMEOUT)

    return lan_pkt, wan_pkt


def refresh_flow(ports: Ports, lan: int, wan: int, lan_pkt: Packet) -> None:
    """LAN packet re-opens or refreshes the flow (controller path if it had expired)."""
    ports.send(lan, lan_pkt)
    expect_packet_from_port(ports, wan, lan_pkt, timeout=CPU_PATH_TIMEOUT)


def test(ports: Ports) -> None:
    for lan in LAN_PORTS:
        wan = WAN_OF[lan]
        step(f"LAN {lan} <-> WAN {wan}: unknown WAN flow dropped, LAN opens it, WAN reply allowed, fast path works")
        lan_pkt, wan_pkt = lan_creates_flow(ports, lan, wan)

        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt)

        # Both directions now hit the data plane table.
        ports.send(lan, lan_pkt)
        expect_packet_from_port(ports, wan, lan_pkt)
        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt)

    lan, wan = LAN_PORTS[0], WAN_OF[LAN_PORTS[0]]

    step("WAN packet with the LAN flow's own 4-tuple (not inverted) is dropped")
    lan_pkt, wan_pkt = lan_creates_flow(ports, lan, wan)
    ports.send(wan, lan_pkt)
    expect_no_packet(ports)

    step("WAN reply with one differing field is dropped")
    flow = lan_pkt[IP].src, lan_pkt[IP].dst, lan_pkt[UDP].sport, lan_pkt[UDP].dport
    base = Flow(*flow).invert()
    for variant in (
        base.clone(new_src_addr="1.1.1.1"),
        base.clone(new_dst_addr="1.1.1.1"),
        base.clone(new_src_port=(base.src_port % 0xFFFF) + 1),
        base.clone(new_dst_port=(base.dst_port % 0xFFFF) + 1),
    ):
        ports.send(wan, build_packet(flow=variant))
        expect_no_packet(ports)

    step("the flow key has no protocol: a TCP reply to a UDP flow is allowed")
    refresh_flow(ports, lan, wan, lan_pkt)
    tcp_reply = build_packet(flow=base, proto="tcp")
    ports.send(wan, tcp_reply)
    expect_packet_from_port(ports, lan, tcp_reply)

    step("a TCP LAN flow is installed too, and a UDP reply to it is allowed")
    flow = build_flow()
    tcp_lan = build_packet(flow=flow, proto="tcp")
    ports.send(lan, tcp_lan)
    expect_packet_from_port(ports, wan, tcp_lan, timeout=CPU_PATH_TIMEOUT)
    udp_reply = build_packet(flow=flow.invert())
    ports.send(wan, udp_reply)
    expect_packet_from_port(ports, lan, udp_reply)

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

    step(f"WAN traffic refreshes the flow: replies every {EXPIRATION_SEC / 2}s for {4 * EXPIRATION_SEC}s all pass")
    lan_pkt, wan_pkt = lan_creates_flow(ports, lan, wan)
    for _ in range(int(4 * EXPIRATION_SEC / (EXPIRATION_SEC / 2))):
        sleep(EXPIRATION_SEC / 2)
        ports.send(wan, wan_pkt)
        expect_packet_from_port(ports, lan, wan_pkt)

    step("LAN traffic refreshes the flow as well")
    for _ in range(int(4 * EXPIRATION_SEC / (EXPIRATION_SEC / 2))):
        sleep(EXPIRATION_SEC / 2)
        ports.send(lan, lan_pkt)
        expect_packet_from_port(ports, wan, lan_pkt)  # fast path: the flow must still be there
    ports.send(wan, wan_pkt)
    expect_packet_from_port(ports, lan, wan_pkt)

    step(f"after {4 * EXPIRATION_SEC}s of silence the flow has expired: WAN reply dropped, LAN re-opens it")
    sleep(4 * EXPIRATION_SEC)
    ports.send(wan, wan_pkt)
    expect_no_packet(ports)
    ports.send(lan, lan_pkt)
    expect_packet_from_port(ports, wan, lan_pkt, timeout=CPU_PATH_TIMEOUT)
    ports.send(wan, wan_pkt)
    expect_packet_from_port(ports, lan, wan_pkt)


if __name__ == "__main__":
    run(test, NF)
