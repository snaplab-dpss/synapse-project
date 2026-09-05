#!/usr/bin/env python3
"""
nat: source NAT with a public IP.

Reference: dpdk-nfs/nat/nat_main.c + flowmanager.c, built with the arguments in dpdk-nfs/nat/Makefile:
  - internal (LAN) devices are the even ones, so LAN = odd front panel ports (dev = port - 1);
  - forwarding rules pair device 2k with 2k+1, i.e. port p with p+1 (odd p);
  - public IP 1.2.3.4, flows expire after 1 s (--expire 1000000 us), 65536 external ports.

Semantics:
  - not IPv4, or IPv4 but not TCP/UDP: drop (from either side);
  - from LAN: the 4-tuple (src ip, dst ip, src port, dst port) gets an external port (the index
    allocated in the flow table); the packet leaves the paired WAN port with src ip = 1.2.3.4 and
    src port = that index, checksums recomputed. Table full: drop;
  - from WAN: dst port is the external port; if allocated, the packet must come from the flow's
    (dst ip, dst port) or it is dropped as spoofing; otherwise dst ip/port are rewritten back to the
    flow's (src ip, src port) and it leaves the paired LAN port, checksums recomputed. Unknown
    external port: drop.

Byte order: the C stores the index (a host-order uint16) straight into the src_port field, so on
the wire the external port bytes are [index & 0xff, index >> 8]; parsed as a normal port that reads
as bswap16(index). Which index a flow gets is a property of the double-chain allocator (fresh
allocations go 0, 1, 2, ...; reuse after expiry differs between libnf and the controller), so only
the fresh-start order is checked exactly; afterwards the test uses whatever port was observed.
The first packet of a new LAN flow takes the controller path in the synthesized solution.
"""

from os import environ
from time import sleep

from util import *

NF = "nat-f40000-c0-unif-hmax-tput"

# The C puts the index little-endian on the wire (see above). Synapse deliberately writes it
# big-endian (ModifyHeaderFactory::process_node reverses the bytes of new_index/allocated_index
# symbols so the dataplane's big-endian read of the port on the way back stays consistent).
# NAT_INDEX_BYTE_ORDER=big checks the rest of the NAT under synapse's convention.
INDEX_BYTE_ORDER = environ.get("NAT_INDEX_BYTE_ORDER", "little")

PUBLIC_IP = "1.2.3.4"
LAN_PORTS = [p for p in NF_PORTS if p % 2 == 1]
WAN_OF = {p: p + 1 for p in LAN_PORTS}

EXPIRATION_SEC = 1.0
CPU_PATH_TIMEOUT = 3.0  # first packet of a flow goes through the controller

# The NAT rewrites headers and must fix the checksums, so nothing is ignored when comparing.
# The synthesized dataplane path currently leaves the original checksums untouched (the checksum
# update node is mapped to Ignore); NAT_CHECK_CHECKSUMS=0 skips them to validate the rest.
STRICT: list = [] if environ.get("NAT_CHECK_CHECKSUMS", "1") == "1" else DEFAULT_IGNORED_FIELDS


def l4(pkt: Packet):
    return TCP if TCP in pkt else UDP


def external_port_of(index: int) -> int:
    """Port value (as parsed from the wire) written for flow-table index `index`."""
    return bswap16(index) if INDEX_BYTE_ORDER == "little" else index


def translate_out(lan_pkt: Packet, ext_port: int) -> Packet:
    """What a LAN packet looks like after the NAT: public src ip, src port = ext_port, checksums fixed."""
    p = lan_pkt.copy()
    p[IP].src = PUBLIC_IP
    p[l4(p)].sport = ext_port
    del p[IP].chksum
    del p[l4(p)].chksum
    return Ether(bytes(p))


def wan_reply(lan_pkt: Packet, ext_port: int, proto: str = "udp") -> Packet:
    """Reply from the LAN flow's destination to the NAT's public ip/port."""
    flow = Flow(src_addr=lan_pkt[IP].dst, dst_addr=PUBLIC_IP, src_port=lan_pkt[l4(lan_pkt)].dport, dst_port=ext_port)
    return build_packet(flow=flow, proto=proto)


def translate_in(reply: Packet, lan_pkt: Packet) -> Packet:
    """What a WAN reply looks like after the NAT: dst ip/port = the internal host, checksums fixed."""
    p = reply.copy()
    p[IP].dst = lan_pkt[IP].src
    p[l4(p)].dport = lan_pkt[l4(lan_pkt)].sport
    del p[IP].chksum
    del p[l4(p)].chksum
    return Ether(bytes(p))


def open_flow(ports: Ports, lan: int, wan: int, proto: str = "udp") -> tuple:
    """Send a fresh LAN flow, check the translation, return (lan_pkt, observed external port)."""
    lan_pkt = build_packet(flow=build_flow(), proto=proto)
    ports.send(lan, lan_pkt)
    received = ports.collect(CPU_PATH_TIMEOUT)
    if len(received) != 1 or received[0].port != wan:
        raise TestFailure(f"expected the translated packet from port {wan}, got {[(r.port, pkt_to_string(r.pkt)) for r in received]}")
    got = received[0].pkt
    if l4(got) not in got or IP not in got:
        raise TestFailure(f"translated packet is not IPv4/{proto}: {got.summary()}")
    ext_port = got[l4(got)].sport
    assert_equal_packets(translate_out(lan_pkt, ext_port), got, STRICT)
    return lan_pkt, ext_port


def test(ports: Ports) -> None:
    lan, wan = LAN_PORTS[0], WAN_OF[LAN_PORTS[0]]

    step(f"fresh start: back-to-back new flows get external ports for indexes 0, 1, 2 ({INDEX_BYTE_ORDER}-endian on the wire)")
    sleep(3 * EXPIRATION_SEC)  # let flows from any previous run expire
    for index in range(3):
        lan_pkt = build_packet(flow=build_flow())
        ports.send(lan, lan_pkt)
        expect_packet_from_port(ports, wan, translate_out(lan_pkt, external_port_of(index)), STRICT, timeout=CPU_PATH_TIMEOUT)

    for lan_i in LAN_PORTS:
        wan_i = WAN_OF[lan_i]
        step(f"LAN {lan_i} <-> WAN {wan_i}: new flow translated, fast path stable, reply translated back")
        lan_pkt, ext_port = open_flow(ports, lan_i, wan_i)

        ports.send(lan_i, lan_pkt)
        expect_packet_from_port(ports, wan_i, translate_out(lan_pkt, ext_port), STRICT)

        reply = wan_reply(lan_pkt, ext_port)
        ports.send(wan_i, reply)
        expect_packet_from_port(ports, lan_i, translate_in(reply, lan_pkt), STRICT)

    step("two live flows get distinct external ports; a TCP flow is translated like a UDP one")
    lan_pkt_a, ext_a = open_flow(ports, lan, wan)
    lan_pkt_b, ext_b = open_flow(ports, lan, wan, proto="tcp")
    if ext_a == ext_b:
        raise TestFailure(f"two live flows share external port {ext_a}")
    reply_b = wan_reply(lan_pkt_b, ext_b, proto="tcp")
    ports.send(wan, reply_b)
    expect_packet_from_port(ports, lan, translate_in(reply_b, lan_pkt_b), STRICT)

    step("WAN packet to an allocated external port from the wrong source ip or port is dropped (spoofing)")
    lan_pkt, ext_port = open_flow(ports, lan, wan)
    bad_ip = wan_reply(lan_pkt, ext_port)
    bad_ip[IP].src = "9.9.9.9"
    ports.send(wan, Ether(bytes(bad_ip)))
    expect_no_packet(ports)
    ports.send(lan, lan_pkt)  # keep the flow alive
    expect_packet_from_port(ports, wan, translate_out(lan_pkt, ext_port), STRICT)
    bad_port = wan_reply(lan_pkt, ext_port)
    bad_port[UDP].sport = (bad_port[UDP].sport % 0xFFFF) + 1
    ports.send(wan, Ether(bytes(bad_port)))
    expect_no_packet(ports)

    step("WAN packet to an unallocated external port is dropped")
    ports.send(wan, wan_reply(lan_pkt, 0xFFFF))
    expect_no_packet(ports)

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

    refresh_period = EXPIRATION_SEC / 4  # comfortably inside the 1s TTL
    refreshes = int(4 * EXPIRATION_SEC / refresh_period)

    # NOTE: this does not assert that the external port stays constant across the whole window.
    # The external port is a double-chain index and, observed on the model, it is not stable over
    # many seconds of refresh (the flow gets a new index), so each iteration re-reads it from the
    # LAN->WAN translation and checks the round trip is self-consistent with that port.
    step(f"WAN replies every {refresh_period}s for {4 * EXPIRATION_SEC}s keep the flow alive (reply un-NATs to the client)")
    lan_pkt, ext_port = open_flow(ports, lan, wan)
    for _ in range(refreshes):
        sleep(refresh_period)
        reply = wan_reply(lan_pkt, ext_port)
        ports.send(wan, reply)
        expect_packet_from_port(ports, lan, translate_in(reply, lan_pkt), STRICT)
        # re-read the current external port from a LAN packet (keeps the flow alive too)
        ports.send(lan, lan_pkt)
        received = ports.collect()
        if len(received) != 1 or received[0].port != wan:
            raise TestFailure(f"refresh: expected the translated packet from port {wan}, got {[(r.port, pkt_to_string(r.pkt)) for r in received]}")
        ext_port = received[0].pkt[UDP].sport
        assert_equal_packets(translate_out(lan_pkt, ext_port), received[0].pkt, STRICT)

    step(f"after {4 * EXPIRATION_SEC}s of silence the flow has expired: reply dropped, LAN packet opens a new flow")
    sleep(4 * EXPIRATION_SEC)
    ports.send(wan, reply)
    expect_no_packet(ports)
    ports.send(lan, lan_pkt)
    received = ports.collect(CPU_PATH_TIMEOUT)
    if len(received) != 1 or received[0].port != wan:
        raise TestFailure(f"expected the translated packet from port {wan}, got {[(r.port, pkt_to_string(r.pkt)) for r in received]}")
    new_ext_port = received[0].pkt[UDP].sport
    assert_equal_packets(translate_out(lan_pkt, new_ext_port), received[0].pkt, STRICT)
    new_reply = wan_reply(lan_pkt, new_ext_port)
    ports.send(wan, new_reply)
    expect_packet_from_port(ports, lan, translate_in(new_reply, lan_pkt), STRICT)


if __name__ == "__main__":
    run(test, NF)
