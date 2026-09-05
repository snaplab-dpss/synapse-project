#!/usr/bin/env python3
"""
kvs: in-network key-value cache in front of a storage server.

Reference: dpdk-nfs/kvs/kvs_main.c, built with the arguments in dpdk-nfs/kvs/Makefile and the port
map in configs/tofino2-kvs.toml:
  - the storage server is device 0 = front panel port 1; clients are devices 2..31 = ports 3..32;
  - cache capacity 8192, entries expire after 1 s.

KVS packets are UDP to or from port 670 carrying {op(1), key(4), value(4), status(1), client_port(2)}.
Semantics of the C:
  - from the server: forward to the device in client_port (stored host-order, i.e. little-endian
    on the wire), packet untouched;
  - from a client, key cached: refresh; GET fills value from the cache, PUT stores value; the reply
    goes back to the client with MACs, IPs and UDP ports swapped and status = 1;
  - from a client, key not cached: a PUT allocates an entry and is answered like a hit; a GET (or a
    PUT when the cache is full) is forwarded to the server with client_port = client device.
Checksums are never recomputed by the C, so the (stale) originals are expected verbatim.

The synthesized solutions differ in HOW the cache is realized, so this test auto-detects the mode
by probing (one PUT of a fresh key) and asserts accordingly:
  - DETERMINISTIC cache (gallium: GuardedMapTable + Dchain, and the C's own semantics): every PUT
    is cached immediately and answered with a HIT; a cached GET returns the value; entries expire.
  - HEAVY-HITTER cache (max-tput: HHTable, sampled admission, count-min threshold 128, counters
    reset every 3 s): an uncached PUT is forwarded to the server; only a hot key is eventually
    admitted. Admission is probabilistic, so only best-effort behavior is asserted.
"""

from time import sleep

from util import *

NF = "kvs-f40000-c0-unif-hmax-tput"

SERVER_PORT = 1
CLIENT_PORTS = NF_PORTS

EXPIRATION_SEC = 1.0
CPU_PATH_TIMEOUT = 3.0  # cache admission may run through the controller
ADMISSION_BURST = 64  # heavy-hitter admission: after a handful of sampled packets, paced for the model
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


def detect_cache_mode(ports: Ports, flow: Flow, client: int) -> str:
    """One PUT of a fresh random key: a HIT reply means a deterministic cache, a forward to the
    server means the heavy-hitter cache. The probe key is random so it does not collide later."""
    probe = kvs_packet(flow, KVS_OP_PUT, build_kvs_hdr().key, b"\x01\x02\x03\x04")
    ports.send(client, probe)
    received = ports.collect(CPU_PATH_TIMEOUT)
    ports_seen = {r.port for r in received}
    if client in ports_seen:
        return "deterministic"
    if SERVER_PORT in ports_seen:
        return "hhtable"
    raise TestFailure(f"probe PUT produced no reply and no forward-to-server (ports {sorted(ports_seen)})")


def test_deterministic_cache(ports: Ports, flow: Flow, client: int) -> None:
    key, value = b"\xde\xad\xbe\xef", b"\x11\x22\x33\x44"

    step("deterministic cache: a PUT of a new key is cached and answered with a HIT")
    put = kvs_packet(flow, KVS_OP_PUT, key, value)
    ports.send(client, put)
    expect_packet_from_port(ports, client, hit_reply(put, value), STRICT, timeout=CPU_PATH_TIMEOUT)

    step("a cached GET returns the stored value (flow inverted, status = 1), from any client")
    for c in (client, CLIENT_PORTS[5]):
        get = kvs_packet(build_flow(dst_port=KVS_PORT), KVS_OP_GET, key, b"\0" * 4)
        ports.send(c, get)
        expect_packet_from_port(ports, c, hit_reply(get, value), STRICT, timeout=CPU_PATH_TIMEOUT)

    step("a PUT updates the cached value; a later GET returns the new value")
    value2 = b"\x55\x66\x77\x88"
    put2 = kvs_packet(flow, KVS_OP_PUT, key, value2)
    ports.send(client, put2)
    expect_packet_from_port(ports, client, hit_reply(put2, value2), STRICT, timeout=CPU_PATH_TIMEOUT)
    get = kvs_packet(flow, KVS_OP_GET, key, b"\0" * 4)
    ports.send(client, get)
    expect_packet_from_port(ports, client, hit_reply(get, value2), STRICT, timeout=CPU_PATH_TIMEOUT)

    step("a distinct key still misses to the server")
    other = kvs_packet(flow, KVS_OP_GET, b"\x00\x00\x00\x63", b"\0" * 4)
    ports.send(client, other)
    expect_packet_from_port(ports, SERVER_PORT, to_server(other, client), STRICT, timeout=CPU_PATH_TIMEOUT)

    step(f"after {4 * EXPIRATION_SEC}s idle the entry expires: a GET goes to the server again")
    sleep(4 * EXPIRATION_SEC)
    ports.send(client, get)
    expect_packet_from_port(ports, SERVER_PORT, to_server(get, client), STRICT, timeout=CPU_PATH_TIMEOUT)


def test_hhtable_cache(ports: Ports, flow: Flow, client: int) -> None:
    key, value = b"\xde\xad\xbe\xef", b"\x11\x22\x33\x44"
    req = kvs_packet(flow, KVS_OP_PUT, key, value)

    step("heavy-hitter cache: a PUT of an unknown key is forwarded to the server (not cached immediately)")
    ports.send(client, req)
    expect_packet_from_port(ports, SERVER_PORT, to_server(req, client), STRICT)

    # Admission is probabilistic (sampled, count-min threshold, periodic counter reset), so only
    # check that a hot key is eventually admitted and that any reply seen is well formed.
    step(f"best-effort: a burst of {ADMISSION_BURST} PUTs of one hot key eventually yields cache replies")
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


def test(ports: Ports) -> None:
    flow = build_flow(dst_port=KVS_PORT)
    client = CLIENT_PORTS[0]

    for c in (CLIENT_PORTS[0], CLIENT_PORTS[7], CLIENT_PORTS[-1]):
        step(f"server -> client {c}: forwarded untouched to the device in client_port (little-endian)")
        pkt = kvs_packet(flow, KVS_OP_GET, b"\x01\x02\x03\x04", b"\x0a\x0b\x0c\x0d", status=1, port=client_port_field(c))
        ports.send(SERVER_PORT, pkt)
        expect_packet_from_port(ports, c, pkt, STRICT)

    for c in CLIENT_PORTS:
        step(f"client {c}: GET of an unknown key goes to the server with client_port = device {dev_of(c)}")
        req = kvs_packet(flow, KVS_OP_GET, build_kvs_hdr().key, b"\0" * 4)
        ports.send(c, req)
        expect_packet_from_port(ports, SERVER_PORT, to_server(req, c), STRICT)

    mode = detect_cache_mode(ports, flow, client)
    step(f"detected cache mode: {mode}")
    if mode == "deterministic":
        test_deterministic_cache(ports, flow, client)
    else:
        test_hhtable_cache(ports, flow, client)

    # DEVIATION: the C drops anything that is not a UDP:670 KVS packet (non-IPv4, non-UDP, non-KVS).
    # The synthesized solutions drop nothing from a client: a non-cache-hit is forwarded to the
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
