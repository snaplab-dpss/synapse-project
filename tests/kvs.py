#!/usr/bin/env python3
"""
kvs: in-network key-value cache in front of a storage server.

Reference: dpdk-nfs/kvs/kvs_main.c, built with the arguments in dpdk-nfs/kvs/Makefile and the port
map in configs/tofino2-kvs.toml:
  - the storage server is device 0 = front panel port 1; clients are devices 2..31 = ports 3..32;
  - cache capacity 8192, entries expire after 1 s.

KVS packets are UDP to or from port 670 carrying {op(1), key(4), value(4), status(1), client_port(2)}.
Semantics of the C:
  - not IPv4 / not UDP / not a KVS packet: drop;
  - from the server: forward to the device in client_port (stored host-order, i.e. little-endian
    on the wire), packet untouched;
  - from a client, key cached: refresh; GET fills value from the cache, PUT stores value; the reply
    goes back to the client with MACs, IPs and UDP ports swapped and status = 1;
  - from a client, key not cached: a PUT allocates an entry and is answered like a hit; a GET (or a
    PUT when the cache is full) is forwarded to the server with client_port = client device.
Checksums are never recomputed by the C, so the (stale) originals are expected verbatim.

KNOWN DEVIATION of the max-tput solution: the cache is a heavy-hitter table. A key is admitted
out of band, once its sampled request count crosses a threshold (every 4th packet sampled, count-min
threshold 128, counters reset every 3 s), so an uncached PUT is forwarded to the server instead of
being cached immediately. Steps marked "synthesized:" assert that behavior rather than the C's.
"""

from time import sleep

from util import *

NF = "kvs-f40000-c0-unif-hmax-tput"

SERVER_PORT = 1
CLIENT_PORTS = NF_PORTS

EXPIRATION_SEC = 1.0
ADMISSION_BURST = 64  # admission happens after a handful of sampled packets; paced so the model keeps up
BURST_PACING_SEC = 0.02

STRICT: list = []  # the C leaves checksums untouched, and so must the switch


def dev_of(port: int) -> int:
    return port - 1


def client_port_field(port: int) -> int:
    """client_port as parsed by scapy (big-endian field) for a device stored little-endian."""
    return bswap16(dev_of(port))


def kvs_packet(flow: Flow, op: int, key: bytes, value: bytes, status: int = 0, port: int = 0) -> Packet:
    return build_packet(flow=flow, kvs_hdr=build_kvs_hdr(op=op, key=key, value=value, status=status, port=port))


def to_server(req: Packet, client: int) -> Packet:
    p = req.copy()
    p[KVSHeader].port = client_port_field(client)
    return Ether(bytes(p))


def hit_reply(req: Packet, value: bytes) -> Packet:
    p = req.copy()
    p[Ether].src, p[Ether].dst = req[Ether].dst, req[Ether].src
    p[IP].src, p[IP].dst = req[IP].dst, req[IP].src
    p[UDP].sport, p[UDP].dport = req[UDP].dport, req[UDP].sport
    p[KVSHeader].status = KVS_STATUS_OK
    p[KVSHeader].value = value
    return Ether(bytes(p))


def test(ports: Ports) -> None:
    flow = build_flow(dst_port=KVS_PORT)

    for client in (CLIENT_PORTS[0], CLIENT_PORTS[7], CLIENT_PORTS[-1]):
        step(f"server -> client {client}: forwarded untouched to the device in client_port (little-endian)")
        pkt = kvs_packet(flow, KVS_OP_GET, b"\x01\x02\x03\x04", b"\x0a\x0b\x0c\x0d", status=1, port=client_port_field(client))
        ports.send(SERVER_PORT, pkt)
        expect_packet_from_port(ports, client, pkt, STRICT)

    for client in CLIENT_PORTS:
        step(f"client {client}: GET of an unknown key goes to the server with client_port = device {dev_of(client)}")
        req = kvs_packet(flow, KVS_OP_GET, build_kvs_hdr().key, b"\0" * 4)
        ports.send(client, req)
        expect_packet_from_port(ports, SERVER_PORT, to_server(req, client), STRICT)

    client = CLIENT_PORTS[0]
    key, value = b"\xde\xad\xbe\xef", b"\x11\x22\x33\x44"

    step("synthesized: a PUT of an unknown key is forwarded to the server (the C would cache it and reply)")
    req = kvs_packet(flow, KVS_OP_PUT, key, value)
    ports.send(client, req)
    expect_packet_from_port(ports, SERVER_PORT, to_server(req, client), STRICT)

    # The cache is a heavy-hitter table (probabilistic admission, sampled, threshold 128, counters
    # reset every 3 s), not the C's deterministic cache-every-PUT. So admission and eviction cannot
    # be asserted per-packet; this only checks that a hot key is eventually admitted and that, once
    # a cache reply is seen, it is well formed. Which specific packet gets the reply is not asserted.
    step(f"best-effort: a burst of {ADMISSION_BURST} PUTs of one hot key eventually yields cache replies (heavy-hitter admission)")
    for _ in range(ADMISSION_BURST):
        ports.send(client, req)
        sleep(BURST_PACING_SEC)
    received = ports.collect(timeout=5.0, settle=1.0)
    server_side = [r for r in received if r.port == SERVER_PORT]
    replies = [r for r in received if r.port == client]
    others = [r for r in received if r.port not in (SERVER_PORT, client)]
    print(f"    {len(server_side)} forwarded to the server, {len(replies)} answered from the cache, {len(others)} elsewhere")
    if others:
        raise TestFailure(f"burst: unexpected packets on ports {[r.port for r in others]}")
    for r in server_side:
        assert_equal_packets(to_server(req, client), r.pkt, STRICT)
    for r in replies:
        assert_equal_packets(hit_reply(req, value), r.pkt, STRICT)
    if not replies:
        raise TestFailure(f"burst of {ADMISSION_BURST} PUTs produced no cache reply; the hot key was never admitted")

    # DEVIATION: the C drops anything that is not a UDP:670 KVS packet (non-IPv4, non-UDP, non-KVS).
    # The synthesized solution drops nothing from a client: a non-cache-hit is forwarded to the
    # server regardless of type, so non-KVS UDP / TCP / ICMP / even ARP go to the server. This
    # observes and reports that behavior rather than asserting the C's drop.
    step("DEVIATION (observed, not asserted): non-KVS client traffic is forwarded to the server, not dropped")
    for label, pkt in (
        ("non-KVS UDP (dst 1234)", build_packet(flow=build_flow(dst_port=1234))),
        ("TCP to port 670", build_packet(flow=build_flow(dst_port=KVS_PORT), proto="tcp")),
        ("ICMP", build_icmp_packet()),
        ("non-IPv4 (ARP)", build_non_ip_packet()),
    ):
        ports.send(client, pkt)
        received = ports.collect()
        dests = [r.port for r in received]
        print(f"    {label}: {'dropped' if not dests else 'sent to ports ' + str(dests)}")


if __name__ == "__main__":
    run(test, NF)
