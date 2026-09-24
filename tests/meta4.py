#!/usr/bin/env python3
"""
meta4: per-domain traffic accounting learned from DNS responses.

Reference: dpdk-nfs/meta4/meta4_main.c, and for the expert tofino/meta4/p4/meta4.p4 with the
controller tofino/meta4/meta4.py. Both are configured with the same watch list
(dpdk-nfs/meta4/domains.txt == tofino/meta4/known_domains_v1.txt) and ignored prefix 10.9.0.0/24.

Per packet:
  a DNS response (UDP, port 53, QR set) answering a watched name with an A record teaches the
  {client = ip.dst, server = A} pair its domain id, unless the client is in an ignored prefix;
  dns_total_queried[id] += 1 either way once the name matched.
  any other IPv4 packet whose {client = ip.dst, server = ip.src} pair is known counts:
  packet_counts[id] += 1, byte_counts[id] += its length.
  every packet goes back out its ingress port, unchanged.

What this test can observe on the expert is exactly that: the frame coming back, and the three
per-domain registers (read over bfrt after every step). Matching semantics checked: the most
specific pattern wins, wildcards match one whole label, a pattern matches only names with exactly
its label count and exact labels, ignored clients are refused, CNAMEs before the A record are
skipped, the client->server direction is not accounted, and a repeat response refreshes a pair.

Expert-specific, kept as the expert does it: an idle pair is never expired on the data path (its
timeout is only consulted when a DNS response lands on its slot), so data after a pause is still
attributed. Byte counts are eg_intr_md.pkt_length - 4 (the FCS); the model reports the frame
without one, so the count is measured on the first attributed packet and required consistent.

State is per pipe (registers), so everything runs on a single port.
"""

import os
import struct
import sys
from glob import glob
from pathlib import Path
from socket import inet_aton
from time import sleep

from scapy.layers.dns import DNS  # noqa: F401  (registers the DNS dissector for printing)

from util import *

PORT = 3

RESOLVER = "8.8.8.8"
IGNORED_CLIENT = "10.9.0.5"
DNS_PORT = 53
QTYPE_A = 1
QTYPE_CNAME = 5
QCLASS_IN = 1

WATCH_LIST = testbed.PROJECT_DIR / "tofino" / "meta4" / "known_domains_v1.txt"
QUERIED = "SwitchIngress.dns_total_queried"
PACKETS = "SwitchEgress.packet_counts_table"
BYTES = "SwitchEgress.byte_counts_table"

PAUSE_BEYOND_TIMEOUT = 1.5  # the NFs' timeout is 1 s


def watch_list() -> list[str]:
    with open(WATCH_LIST) as f:
        return [line.strip() for line in f if line.strip() and not line.startswith("#")]


DOMAINS = watch_list()
ID = {name: i for i, name in enumerate(DOMAINS)}


# --- DNS messages, laid out as the NFs walk them: header, one question, then the answers, with
# each answer's name a compression pointer back to the question (what resolvers send and what both
# parsers assume). scapy's DNS layer would spell the name out again.


def encode_name(name: str) -> bytes:
    out = b""
    for label in name.split("."):
        out += bytes([len(label)]) + label.encode()
    return out + b"\x00"


def answer(rr_type: int, rdata: bytes, ttl: int = 60) -> bytes:
    return struct.pack("!HHHIH", 0xC00C, rr_type, QCLASS_IN, ttl, len(rdata)) + rdata


def cname(name: str) -> bytes:
    return answer(QTYPE_CNAME, encode_name(name))


def address(ip: str) -> bytes:
    return answer(QTYPE_A, inet_aton(ip))


def dns_message(name: str, answers: list[bytes], is_response: bool = True) -> bytes:
    flags = 0x8180 if is_response else 0x0100
    msg = struct.pack("!HHHHHH", 0x1234, flags, 1, len(answers), 0, 0)
    msg += encode_name(name)
    msg += struct.pack("!HH", QTYPE_A, QCLASS_IN)
    return msg + b"".join(answers)


def dns_packet(payload: bytes, src: str = RESOLVER, dst: str = "10.0.0.1", dport: int = 33333) -> Packet:
    return build_packet(flow=Flow(src_addr=src, dst_addr=dst, src_port=DNS_PORT, dst_port=dport), payload=payload)


def data_packet(server: str, client: str, proto: str = "udp", payload_len: int = 64) -> Packet:
    return build_packet(flow=Flow(src_addr=server, dst_addr=client, src_port=80, dst_port=44444), payload=b"x" * payload_len, proto=proto)


# --- the switch's registers, over bfrt


class Counters:
    def __init__(self):
        sde_install = os.environ.get("SDE_INSTALL")
        if not sde_install:
            raise TestFailure("SDE_INSTALL is not set; the registers are read over bfrt")
        for site_packages in glob(f"{sde_install}/lib/python*/site-packages"):
            if Path(site_packages, "tofino").is_dir():
                sys.path[:0] = [f"{site_packages}/tofino", site_packages]
                break
        import bfrt_grpc.client as gc

        self.gc = gc
        # The controller was client 0 and has exited; a fresh client id avoids stepping on it.
        self.client = gc.ClientInterface("127.0.0.1:50052", 1, 0)
        self.client.bind_pipeline_config("meta4")
        info = self.client.bfrt_info_get("meta4")
        self.tables = {name: info.table_get(name) for name in (QUERIED, PACKETS, BYTES)}
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)

    def read(self, table: str, index: int) -> int:
        t = self.tables[table]
        key = t.make_key([self.gc.KeyTuple("$REGISTER_INDEX", index)])
        data = next(t.entry_get(self.target, [key], {"from_hw": True}))[0].to_dict()
        values = next(v for k, v in data.items() if not k.startswith("$") and k not in ("action_name", "is_default_entry"))
        return sum(values) if isinstance(values, list) else values  # one value per pipe

    def snapshot(self) -> dict:
        return {(table, i): self.read(table, i) for table in (QUERIED, PACKETS, BYTES) for i in range(len(DOMAINS))}


def expect_counters(before: dict, after: dict, deltas: dict, what: str) -> None:
    """Every counter moved by exactly its delta (0 for the ones not listed)."""
    for key in before:
        expected = before[key] + deltas.get(key, 0)
        if after[key] != expected:
            table, i = key
            raise TestFailure(f"{what}: {table.split('.')[-1]}[{i} = {DOMAINS[i]}] is {after[key]}, expected {expected} (was {before[key]})")


# --- one packet in, the same packet out


def send_and_expect_back(ports: Ports, pkt: Packet, what: str, intact: bool = True) -> Packet:
    ports.send(PORT, pkt)
    received = ports.collect()
    if len(received) != 1 or received[0].port != PORT:
        raise TestFailure(f"{what}: expected the packet back on port {PORT}, got {[(r.port, pkt_to_string(r.pkt)) for r in received]}")
    if intact:
        assert_equal_packets(pkt, received[0].pkt)
    else:
        # Headers untouched; the DNS message may have lost the records the parser stepped over.
        sent, got = bytes(pkt), received[0].raw
        headers = 14 + 20 + 8
        if got[:headers] != sent[:headers]:
            raise TestFailure(f"{what}: Ethernet/IPv4/UDP headers changed:\n  sent {sent[:headers].hex()}\n  got  {got[:headers].hex()}")
        if got != sent:
            print(f"    (came back {len(got) - len(sent):+d} bytes: the CNAME the parser stepped over is gone)")
    return received[0].pkt


class Meta4Test:
    def __init__(self, ports: Ports):
        self.ports = ports
        self.counters = Counters()
        self.bytes_per_frame_offset = None  # what byte_counts adds for a frame of length L: L + this

    def step(self, what: str, pkt: Packet, deltas: dict, unchanged: bool = False, intact: bool = True) -> None:
        step(what)
        before = self.counters.snapshot()
        send_and_expect_back(self.ports, pkt, what, intact)
        after = self.counters.snapshot()
        expect_counters(before, after, {} if unchanged else deltas, what)

    def attributed(self, what: str, pkt: Packet, domain: str) -> None:
        """A data packet counted for `domain`: one packet, and its bytes as the expert counts them."""
        step(what)
        before = self.counters.snapshot()
        send_and_expect_back(self.ports, pkt, what)
        after = self.counters.snapshot()
        i = ID[domain]
        added = after[(BYTES, i)] - before[(BYTES, i)]
        if self.bytes_per_frame_offset is None:
            self.bytes_per_frame_offset = added - len(pkt)
            if self.bytes_per_frame_offset not in (-4, 0, 4):
                raise TestFailure(f"{what}: byte_counts added {added} for a {len(pkt)}-byte frame")
            print(f"    byte_counts adds frame length {self.bytes_per_frame_offset:+d} (the expert subtracts 4 for an FCS)")
        expect_counters(before, after, {(PACKETS, i): 1, (BYTES, i): len(pkt) + self.bytes_per_frame_offset}, what)

    def response(self, what: str, name: str, server: str, client: str, domain: str = None, answers: list = None, dport: int = 33333) -> None:
        """A DNS response; `domain` is the watched pattern it must be accounted to, None for none."""
        pkt = dns_packet(dns_message(name, answers if answers is not None else [address(server)]), dst=client, dport=dport)
        self.step(what, pkt, {(QUERIED, ID[domain]): 1} if domain else {}, intact=answers is None)


def test(ports: Ports) -> None:
    t = Meta4Test(ports)
    c1, s1 = "10.0.0.1", "93.184.216.34"
    c2, s2 = "10.0.0.2", "93.184.216.35"
    c3, s3 = "10.0.0.3", "93.184.216.36"
    c4, s4 = "10.0.0.4", "93.184.216.37"

    t.step("data for a pair nothing has taught yet: reflected, not counted", data_packet(s1, c1), {}, unchanged=True)

    t.step("a DNS query, not a response: reflected, ignored", dns_packet(dns_message("www.google.com", [address(s1)], is_response=False), src=c1, dst=RESOLVER), {}, unchanged=True)

    t.response("a response for an unwatched name", "unwatched.org", s1, c1, domain=None)

    t.response("www.google.com: the exact pattern wins over *.google.com", "www.google.com", s1, c1, domain="www.google.com")
    t.attributed("data server -> client on the learned pair", data_packet(s1, c1), "www.google.com")
    t.step("the same pair client -> server: not the tracked direction", data_packet(c1, s1), {}, unchanged=True)
    t.attributed("TCP on the same pair: the transport does not matter", data_packet(s1, c1, proto="tcp"), "www.google.com")

    t.response("xyz.google.com: matched by *.google.com", "xyz.google.com", s2, c2, domain="*.google.com")
    t.attributed("its data goes to *.google.com", data_packet(s2, c2), "*.google.com")

    t.response("a.b.google.com: four labels, *.google.com does not cover it", "a.b.google.com", s3, c3, domain=None)
    t.response("google.com: two labels, *.google.com does not cover it either", "google.com", s3, c3, domain=None)
    t.response("wwwabcd.google.com: not www.google.com (exact labels), so *.google.com", "wwwabcd.google.com", s3, c3, domain="*.google.com")

    t.response("a.b.skype.com: *.*.skype.com, not *.skype.com", "a.b.skype.com", s3, c3, domain="*.*.skype.com")
    t.response("teams.skype.com: listed in its own right, wins over *.skype.com", "teams.skype.com", s3, c3, domain="teams.skype.com")

    t.response("a watched name answered to an ignored client: refused", "www.google.com", s4, IGNORED_CLIENT, domain=None)
    t.step("data to the ignored client: not counted", data_packet(s4, IGNORED_CLIENT), {}, unchanged=True)

    t.response("mail.google.com behind a CNAME, as a CDN answers", "mail.google.com", s4, c4, domain="mail.google.com",
               answers=[cname("edge.cdn.net"), address(s4)])
    t.attributed("data from the address behind the CNAME", data_packet(s4, c4), "mail.google.com")

    t.response("a second response for a pair already tracked: counted, pair refreshed", "www.google.com", s1, c1, domain="www.google.com")
    t.attributed("its data is still attributed", data_packet(s1, c1), "www.google.com")

    step(f"a pair idle for {PAUSE_BEYOND_TIMEOUT} s is still attributed: the expert only expires on a DNS collision")
    sleep(PAUSE_BEYOND_TIMEOUT)
    t.attributed("data after the pause", data_packet(s2, c2), "*.google.com")

    step("a non-IPv4 frame comes back on its port, untouched")
    arp = build_non_ip_packet()
    ports.send(PORT, arp)
    received = ports.collect()
    if len(received) != 1 or received[0].port != PORT:
        raise TestFailure(f"expected the ARP frame back on port {PORT}, got {[(r.port, pkt_to_string(r.pkt)) for r in received]}")
    if received[0].raw != bytes(arp):
        raise TestFailure(f"the ARP frame came back changed ({len(received[0].raw) - len(bytes(arp)):+d} bytes)")


if __name__ == "__main__":
    run(test)
