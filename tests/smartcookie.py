#!/usr/bin/env python3
"""
smartcookie: SYN-cookie offload, checked against dpdk-nfs/smartcookie.

Semantics (smartcookie_main.c):
  - a client SYN is answered with a SYN-ACK back out the ingress port, with the addresses and
    ports swapped, ack = seq + 1 and seq = the cookie;
  - cookie = cookie_time ^ HalfSipHash-2-4(src, dst, sport++dport, seq), where
    cookie_time = (ticks(now) - stored_delta) >> 12 and ticks are 2^16 ns units;
  - a client's later non-SYN packet is passed to the server if the bloom filter already holds its
    flow; otherwise its cookie is checked as cookie_val = (ack - 1) ^ hash(..., seq - 1) and the
    packet is accepted only if cookie_time - cookie_val is 0, 1 or 2. Accepted packets go to the
    server with seq -= 1 and the ECE flag set;
  - an ECE-tagged packet from the server records its flow in the bloom filter and is dropped;
  - a UDP packet from the server to port 5555 carries the server's clock and sets the delta;
  - anything else IPv4 is routed by the first octet of the destination address.

Both crafted packets are cut down to a 20-byte IPv4 header and a 20-byte TCP header with the
checksums recomputed, so this also exercises the deparser checksums.

Topology: front panel port N is NF device N - 1, so the server (device 2) is port 3 and routing
by the destination's first octet D lands on port D + 1.

The cookie itself cannot be predicted, because cookie_time comes from the switch's own clock. It
is checked two ways instead: the hash is recomputed here, so the cookie_time recovered from two
cookies minted back to back must agree; and a clock update announcing a known value pins
cookie_time to a value this test can name.
"""

from os import environ

from util import *

# Defaults to the hand-written ground truth. A synthesized solution is tested by naming it here
# together with its topology, e.g.
#   SC_NF=smartcookie-f40000-c0-unif-hmax-tput SC_SERVER_PORT=1 SC_SERVER_DEV=0 ./smartcookie.py
NF = environ.get("SC_NF", "smartcookie-manual")

# Which front panel port the server sits on, and the NF device it is known by. The hand-written
# solutions put it on port 3 / device 2; a synthesized solution follows configs/tofino2-smartcookie
# .toml, which uses port 1 / device 0. Routing is by the destination's first octet, so the device
# number also picks the address a client uses to reach the server.
SERVER_PORT = int(environ.get("SC_SERVER_PORT", "3"))
SERVER_DEV = int(environ.get("SC_SERVER_DEV", "2"))
CLIENT_PORT = int(environ.get("SC_CLIENT_PORT", "5"))
OTHER_CLIENT_PORT = int(environ.get("SC_OTHER_CLIENT_PORT", "7"))
TIMESYNC_PORT = 5555

SYN = 0x02
ACK = 0x10
ECE = 0x40

SIP_KEY_0 = 0x33323130
SIP_KEY_1 = 0x42413938

M32 = 0xFFFFFFFF


# --- the NF's hash, recomputed here -------------------------------------------------------------


def rotl(x: int, n: int) -> int:
    return ((x << n) | (x >> (32 - n))) & M32


def sipround(v: list[int]) -> None:
    v[0] = (v[0] + v[1]) & M32
    v[2] = (v[2] + v[3]) & M32
    v[1] = rotl(v[1], 5)
    v[3] = rotl(v[3], 8)
    v[1] ^= v[0]
    v[3] ^= v[2]
    v[0] = rotl(v[0], 16)
    v[2] = (v[2] + v[1]) & M32
    v[0] = (v[0] + v[3]) & M32
    v[1] = rotl(v[1], 13)
    v[3] = rotl(v[3], 7)
    v[1] ^= v[2]
    v[3] ^= v[0]
    v[2] = rotl(v[2], 16)


def halfsiphash(words: list[int]) -> int:
    """HalfSipHash-2-4 as the switch computes it: no v[2] ^= 0xff finalization constant."""
    v = [
        SIP_KEY_0 ^ 0x70736575,
        SIP_KEY_1 ^ 0x6E646F6D,
        SIP_KEY_0 ^ 0x6E657261,
        SIP_KEY_1 ^ 0x79746573,
    ]
    for m in words:
        v[3] ^= m
        sipround(v)
        sipround(v)
        v[0] ^= m
    for _ in range(4):
        sipround(v)
    return v[0] ^ v[1] ^ v[2] ^ v[3]


def ip_to_int(addr: str) -> int:
    return int.from_bytes(bytes(int(b) for b in addr.split(".")), "big")


def cookie_hash(flow: Flow, seq: int) -> int:
    ports = ((flow.src_port << 16) | flow.dst_port) & M32
    return halfsiphash([ip_to_int(flow.src_addr), ip_to_int(flow.dst_addr), ports, seq & M32])


# --- packet helpers -----------------------------------------------------------------------------


def pad_frame(pkt: Packet) -> Packet:
    """Serialize (filling in lengths and checksums), pad to 60B, and re-parse.

    The padding is re-parsed as trailing bytes rather than TCP payload, because the IPv4 total
    length already says 40, which is what both the switch and scapy checksum over.
    """
    raw = bytes(pkt)
    return Ether(raw + b"\0" * max(0, 60 - len(raw)))


def tcp_packet(flow: Flow, seq: int = 0, ack: int = 0, flags: int = SYN) -> Packet:
    return pad_frame(
        Ether(dst=DST_MAC, src=SRC_MAC)
        / IP(src=flow.src_addr, dst=flow.dst_addr)
        / TCP(sport=flow.src_port, dport=flow.dst_port, seq=seq, ack=ack, flags=flags)
    )


def expected_synack(flow: Flow, seq: int, cookie: int, flags: int = SYN) -> Packet:
    back = flow.invert()
    return pad_frame(
        Ether(dst=DST_MAC, src=SRC_MAC)
        / IP(src=back.src_addr, dst=back.dst_addr)
        / TCP(
            sport=back.src_port,
            dport=back.dst_port,
            seq=cookie,
            ack=(seq + 1) & M32,
            flags=flags | SYN | ACK,
        )
    )


def expected_tagged(flow: Flow, seq: int, ack: int, flags: int) -> Packet:
    """What a verified cookie sends on to the server: seq - 1 and the ECE flag."""
    return pad_frame(
        Ether(dst=DST_MAC, src=SRC_MAC)
        / IP(src=flow.src_addr, dst=flow.dst_addr)
        / TCP(sport=flow.src_port, dport=flow.dst_port, seq=(seq - 1) & M32, ack=ack, flags=flags | ECE)
    )


def timesync_packet(ticks: int) -> Packet:
    """The server agent's clock update: UDP to port 5555 carrying its tick count."""
    return pad_frame(
        Ether(dst=DST_MAC, src=SRC_MAC)
        / IP(src="9.9.9.9", dst="9.9.9.8")
        / UDP(sport=1234, dport=TIMESYNC_PORT)
        / ticks.to_bytes(4, "big")
    )


def client_flow(dst_octet: Optional[int] = None) -> Flow:
    """A client -> server flow. The first octet of the destination is the server's NF device."""
    return build_flow(dst_addr=f"{SERVER_DEV if dst_octet is None else dst_octet}.1.1.1")


# --- the test -----------------------------------------------------------------------------------


def do_syn(ports: Ports, flow: Flow, seq: int, client: int = CLIENT_PORT) -> int:
    """Send a SYN, check the SYN-ACK exactly, and return the cookie it carries."""
    ports.send(client, tcp_packet(flow, seq=seq, flags=SYN))
    received = ports.collect()
    if len(received) != 1 or received[0].port != client:
        summary = ", ".join(f"port {r.port}: {pkt_to_string(r.pkt)}" for r in received)
        raise TestFailure(f"expected exactly one SYN-ACK back from port {client}, got {len(received)}: {summary}")

    cookie = received[0].pkt[TCP].seq
    # Compared with the checksums included: the switch recomputes both in the deparser.
    assert_equal_packets(expected_synack(flow, seq, cookie), received[0].pkt, [])
    return cookie


def test(ports: Ports) -> None:
    step("a client SYN is answered with a SYN-ACK carrying a cookie")
    flow_a = client_flow()
    seq_a = 0x11223344
    cookie_a = do_syn(ports, flow_a, seq_a)

    step("the client's ACK with a valid cookie reaches the server, ECE-tagged and with seq - 1")
    ack_pkt = tcp_packet(flow_a, seq=seq_a + 1, ack=(cookie_a + 1) & M32, flags=ACK)
    ports.send(CLIENT_PORT, ack_pkt)
    expect_packet_from_port(ports, SERVER_PORT, expected_tagged(flow_a, seq_a + 1, (cookie_a + 1) & M32, ACK), [])

    step("an ACK whose cookie is wrong is dropped")
    # Far from the real cookie on purpose: an ack a count or two off can still xor into an age
    # inside the accepted window, which is a property of the cookie scheme, not a bug.
    ports.send(CLIENT_PORT, tcp_packet(flow_a, seq=seq_a + 1, ack=(cookie_a ^ 0xDEADBEEF) & M32, flags=ACK))
    expect_no_packet(ports)

    step("a cookie is bound to its flow: replaying it on another flow is dropped")
    other = flow_a.clone(new_src_port=(flow_a.src_port ^ 0x0100) & 0xFFFF)
    ports.send(CLIENT_PORT, tcp_packet(other, seq=seq_a + 1, ack=(cookie_a + 1) & M32, flags=ACK))
    expect_no_packet(ports)

    step("the cookie is cookie_time ^ HalfSipHash of the 4-tuple and the sequence number")
    # cookie_time is the switch's own clock, so it cannot be predicted; but it only advances once
    # an epoch (2^28 ns, about 268 ms), so recovering it from two cookies minted back to back
    # checks the hash itself. The NF accepts an age of up to 2, so allow the same slack here.
    flow_b = client_flow()
    seq_b = 0x55667788
    cookie_b = do_syn(ports, flow_b, seq_b)
    flow_c = client_flow()
    seq_c = 0x99AABBCC
    cookie_c = do_syn(ports, flow_c, seq_c, client=OTHER_CLIENT_PORT)

    ctime_b = cookie_b ^ cookie_hash(flow_b, seq_b)
    ctime_c = cookie_c ^ cookie_hash(flow_c, seq_c)
    if not 0 <= (ctime_c - ctime_b) & M32 <= 2:
        raise TestFailure(
            "the hash the switch computes does not match the reference: two cookies minted "
            f"moments apart give cookie_time 0x{ctime_b:08x} and 0x{ctime_c:08x}"
        )

    step("a SYN-ACK from a client is dropped")
    ports.send(CLIENT_PORT, tcp_packet(client_flow(), seq=1, ack=1, flags=SYN | ACK))
    expect_no_packet(ports)

    step("an ECE-tagged packet from the server is dropped and records the flow")
    flow_v = client_flow()
    ports.send(SERVER_PORT, tcp_packet(flow_v, seq=7, ack=9, flags=ACK | ECE))
    expect_no_packet(ports)

    step("a packet on a recorded flow goes to the server untouched, cookie or no cookie")
    passthrough = tcp_packet(flow_v, seq=123, ack=456, flags=ACK)
    ports.send(CLIENT_PORT, passthrough)
    expect_packet_from_port(ports, SERVER_PORT, passthrough, [])

    step("an untagged packet from the server is routed by the destination's first octet")
    to_client = build_flow(dst_addr="10.4.5.6")
    plain = tcp_packet(to_client, seq=1, ack=1, flags=ACK)
    ports.send(SERVER_PORT, plain)
    expect_packet_from_port(ports, 11, plain, [])

    step("a non-TCP packet is routed by the destination's first octet")
    udp = build_packet(flow=build_flow(dst_addr="12.4.5.6"))
    ports.send(CLIENT_PORT, udp)
    expect_packet_from_port(ports, 13, udp, [])

    step("a UDP packet to port 5555 that is not from the server is routed, not consumed")
    sync_from_client = timesync_packet(0)
    sync_from_client[IP].dst = "14.4.5.6"
    sync_from_client = pad_frame(sync_from_client)
    ports.send(CLIENT_PORT, sync_from_client)
    expect_packet_from_port(ports, 15, sync_from_client, [])

    step("the server's clock update is consumed and moves cookie_time to the clock it announces")
    # delta = ticks(now) - announced, so afterwards cookie_time is (announced + ticks elapsed
    # since) >> 12. One epoch is 2^28 ns, about 268 ms, so a handful of epochs may pass before
    # the cookie below is minted, but the announced clock fixes everything above that.
    announced = 0x40000000
    ports.send(SERVER_PORT, timesync_packet(announced))
    expect_no_packet(ports)

    flow_d = client_flow()
    seq_d = 0x99AABBCC
    cookie_d = do_syn(ports, flow_d, seq_d)
    ctime_d = cookie_d ^ cookie_hash(flow_d, seq_d)
    if not 0 <= ctime_d - (announced >> 12) <= 8:
        raise TestFailure(
            f"after a clock update announcing 0x{announced:08x} ticks, cookie_time should be "
            f"0x{announced >> 12:08x} plus a few epochs, got 0x{ctime_d:08x}"
        )

    step("cookies minted against the updated clock still verify")
    ports.send(CLIENT_PORT, tcp_packet(flow_d, seq=seq_d + 1, ack=(cookie_d + 1) & M32, flags=ACK))
    expect_packet_from_port(ports, SERVER_PORT, expected_tagged(flow_d, seq_d + 1, (cookie_d + 1) & M32, ACK), [])


if __name__ == "__main__":
    run(test, NF)
