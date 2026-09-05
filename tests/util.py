"""
Black-box packet I/O against the Tofino 2 model, plus helpers shared by the NF test scripts.

Port model
----------
Front panel port N (1..32) is backed by the veth pair (veth{2(N-1)}, veth{2(N-1)+1}).
The tofino-model owns the even end; we inject and capture on the odd end, exactly like
PTF does. Frames we transmit ourselves show up on the same raw socket flagged as
PACKET_OUTGOING, so they are filtered out: everything reported here came out of the switch.

Test scripts call `run(test_fn, nf)`; `test_fn` receives a `Ports` object and raises
`TestFailure` (through the expect_* helpers) when the switch misbehaves.
"""

from __future__ import annotations  # keep 3.9+ annotation syntax valid on Python 3.8

import sys
from argparse import ArgumentParser
from binascii import hexlify
from dataclasses import dataclass
from enum import Enum
from random import getrandbits, randint
from select import select
from socket import AF_PACKET, SOCK_RAW, inet_aton, ntohs, socket
from time import monotonic
from typing import Callable, Optional

from scapy.fields import ByteField, ShortField, StrFixedLenField
from scapy.layers.inet import ICMP, IP, TCP, UDP
from scapy.layers.l2 import ARP, Ether
from scapy.packet import Packet, bind_layers

import testbed

ETH_P_ALL = 3
PACKET_OUTGOING = 4  # sll_pkttype of frames we sent ourselves

ALL_PORTS = list(range(1, 33))
NF_PORTS = list(range(3, 33))  # front panel ports wired to NF devices (configs/tofino2.toml)

# Data plane latency on the model is a few ms, but under load a frame can occasionally take up to a
# second, so the default wait is generous to avoid false "got nothing" failures; CPU-path packets
# take longer still and pass their own timeout.
DEFAULT_RX_TIMEOUT = 2.0  # seconds to wait for the first frame (positive checks: tolerate a slow model)
DEFAULT_SETTLE_TIME = 0.2  # seconds of silence after the last frame before we stop collecting
# Negative checks ("no packet") must stay short: several NFs expire flows after ~1s, so a long wait
# here would let a flow expire mid-scenario and change later behavior. A drop is decided in the data
# plane in milliseconds, so a fraction of a second is plenty.
NO_PACKET_TIMEOUT = 0.7

SRC_MAC = "02:00:00:DD:EE:FF"
DST_MAC = "02:00:00:AA:BB:CC"

KVS_PORT = 670
KVS_KEY_SIZE_BYTES = 4
KVS_VALUE_SIZE_BYTES = 4
KVS_OP_GET = 0
KVS_OP_PUT = 1
KVS_STATUS_OK = 1
KVS_STATUS_FAIL = 0


class TestFailure(Exception):
    pass


class HeaderField(Enum):
    IP_CHECKSUM = 0
    UDP_CHECKSUM = 1
    KVS_CLIENT_PORT = 2


DEFAULT_IGNORED_FIELDS = [HeaderField.IP_CHECKSUM, HeaderField.UDP_CHECKSUM]


class KVSHeader(Packet):
    name = "KVSHeader"
    fields_desc = [
        ByteField("op", 0),
        StrFixedLenField("key", b"\x00" * KVS_KEY_SIZE_BYTES, KVS_KEY_SIZE_BYTES),
        StrFixedLenField("value", b"\x00" * KVS_VALUE_SIZE_BYTES, KVS_VALUE_SIZE_BYTES),
        ByteField("status", 0),
        ShortField("port", 0),
    ]

    @staticmethod
    def guess_payload_class(pkt, **kargs):
        if isinstance(pkt, UDP) and (pkt.dport == KVS_PORT or pkt.sport == KVS_PORT):
            return KVSHeader
        return None


bind_layers(UDP, KVSHeader, dport=KVS_PORT)
bind_layers(UDP, KVSHeader, sport=KVS_PORT)


def injector_iface(port: int) -> str:
    return f"veth{(port - 1) * 2 + 1}"


def pkt_to_string(pkt: Packet) -> str:
    if KVSHeader in pkt:
        return f"KVS{{key={pkt[KVSHeader].key.hex()}, value={pkt[KVSHeader].value.hex()}}}"
    for l4, name in ((UDP, "udp"), (TCP, "tcp")):
        if IP in pkt and l4 in pkt:
            return f"{name} {pkt[IP].src}:{pkt[l4].sport} -> {pkt[IP].dst}:{pkt[l4].dport}"
    if IP in pkt:
        return f"{pkt[IP].src} -> {pkt[IP].dst} (proto {pkt[IP].proto})"
    return f"{pkt.summary()} ({len(pkt)}B)"


@dataclass
class Received:
    port: int
    pkt: Packet
    raw: bytes


class Ports:
    """One raw socket per front panel port, on the injector end of the veth pair."""

    def __init__(self, ports: list[int] = ALL_PORTS, verbose: bool = True):
        self.verbose = verbose
        self.port_to_socket: dict[int, socket] = {}
        self.socket_to_port: dict[socket, int] = {}

        for port in ports:
            s = socket(AF_PACKET, SOCK_RAW, ntohs(ETH_P_ALL))
            s.bind((injector_iface(port), 0))
            s.setblocking(False)
            self.port_to_socket[port] = s
            self.socket_to_port[s] = port

        self.drain()

    def close(self) -> None:
        for s in self.socket_to_port:
            s.close()

    def _recv_incoming(self, s: socket) -> Optional[bytes]:
        data, addr = s.recvfrom(65535)
        if addr[2] == PACKET_OUTGOING:
            return None
        return data

    def drain(self) -> int:
        """Discard everything currently queued on every port."""
        dropped = 0
        while True:
            ready, _, _ = select(list(self.socket_to_port), [], [], 0)
            if not ready:
                return dropped
            for s in ready:
                if self._recv_incoming(s) is not None:
                    dropped += 1

    def send(self, port: int, pkt: Packet) -> None:
        if port not in self.port_to_socket:
            raise ValueError(f"port {port} not opened")
        if self.verbose:
            print(f"  send {port:>2}: {pkt_to_string(pkt)}")
        self.port_to_socket[port].send(bytes(pkt))

    def collect(self, timeout: float = DEFAULT_RX_TIMEOUT, settle: float = DEFAULT_SETTLE_TIME) -> list[Received]:
        """
        Frames emitted by the switch, in arrival order.

        Waits up to `timeout` for the first frame, then keeps collecting until `settle`
        seconds pass without a new one, so extra/duplicate frames are caught too.
        """
        received: list[Received] = []
        deadline = monotonic() + timeout

        while True:
            remaining = deadline - monotonic()
            if remaining <= 0:
                break
            ready, _, _ = select(list(self.socket_to_port), [], [], remaining)
            if not ready:
                break
            for s in ready:
                data = self._recv_incoming(s)
                if data is None:
                    continue
                port = self.socket_to_port[s]
                pkt = Ether(data)
                received.append(Received(port, pkt, data))
                if self.verbose:
                    print(f"  recv {port:>2}: {pkt_to_string(pkt)}")
                deadline = monotonic() + settle

        return received


def _normalize(pkt: Packet, ignore_fields: list[HeaderField]) -> bytes:
    pkt = pkt.copy()
    for field in ignore_fields:
        if field == HeaderField.IP_CHECKSUM and IP in pkt:
            pkt[IP].chksum = 0
        elif field == HeaderField.UDP_CHECKSUM and UDP in pkt:
            pkt[UDP].chksum = 0
        elif field == HeaderField.UDP_CHECKSUM and TCP in pkt:  # "L4 checksum"
            pkt[TCP].chksum = 0
        elif field == HeaderField.KVS_CLIENT_PORT and KVSHeader in pkt:
            pkt[KVSHeader].port = 0
    return bytes(pkt)


def _describe_mismatch(expected: Packet, received: Packet, ignore_fields: list[HeaderField]) -> str:
    lines = ["Packet mismatch", "", "Expected:"]
    lines += ["  " + line for line in expected.show(dump=True).splitlines()]
    lines += ["", "Received:"]
    lines += ["  " + line for line in received.show(dump=True).splitlines()]
    lines += ["", "Hexdump (ignored fields zeroed):"]
    lines.append(f"  expected: {_normalize(expected, ignore_fields).hex()}")
    lines.append(f"  received: {_normalize(received, ignore_fields).hex()}")
    return "\n".join(lines)


def assert_equal_packets(expected: Packet, received: Packet, ignore_fields: list[HeaderField] = DEFAULT_IGNORED_FIELDS) -> None:
    if _normalize(expected, ignore_fields) != _normalize(received, ignore_fields):
        raise TestFailure(_describe_mismatch(expected, received, ignore_fields))


def expect_no_packet(ports: Ports, timeout: float = NO_PACKET_TIMEOUT) -> None:
    received = ports.collect(timeout)
    if received:
        summary = ", ".join(f"port {r.port}: {pkt_to_string(r.pkt)}" for r in received)
        raise TestFailure(f"Expected no packet, got {len(received)}: {summary}")


def expect_packet_from_port(
    ports: Ports,
    port: int,
    pkt: Packet,
    ignore_fields: list[HeaderField] = DEFAULT_IGNORED_FIELDS,
    timeout: float = DEFAULT_RX_TIMEOUT,
) -> Packet:
    """Exactly one frame must come out, from `port`, equal to `pkt` modulo `ignore_fields`."""
    received = ports.collect(timeout)

    if not received:
        raise TestFailure(f"Expected a packet from port {port}, got nothing within {timeout}s")

    if len(received) != 1 or received[0].port != port:
        summary = ", ".join(f"port {r.port}: {pkt_to_string(r.pkt)}" for r in received)
        raise TestFailure(f"Expected exactly one packet from port {port}, got {len(received)}: {summary}")

    assert_equal_packets(pkt, received[0].pkt, ignore_fields)
    return received[0].pkt


@dataclass
class Flow:
    src_addr: str
    dst_addr: str
    src_port: int
    dst_port: int

    def __str__(self) -> str:
        return f"{self.src_addr}:{self.src_port} -> {self.dst_addr}:{self.dst_port}"

    def __repr__(self) -> str:
        return str(self)

    def clone(
        self,
        new_src_addr: Optional[str] = None,
        new_dst_addr: Optional[str] = None,
        new_src_port: Optional[int] = None,
        new_dst_port: Optional[int] = None,
    ) -> "Flow":
        return Flow(
            src_addr=new_src_addr if new_src_addr is not None else self.src_addr,
            dst_addr=new_dst_addr if new_dst_addr is not None else self.dst_addr,
            src_port=new_src_port if new_src_port is not None else self.src_port,
            dst_port=new_dst_port if new_dst_port is not None else self.dst_port,
        )

    def invert(self) -> "Flow":
        return Flow(src_addr=self.dst_addr, dst_addr=self.src_addr, src_port=self.dst_port, dst_port=self.src_port)

    def hex(self) -> str:
        src_addr = hexlify(inet_aton(self.src_addr)).decode()
        dst_addr = hexlify(inet_aton(self.dst_addr)).decode()
        return f"{src_addr}:{self.src_port:04X} -> {dst_addr}:{self.dst_port:04X}"


def build_flow(
    src_addr: Optional[str] = None,
    dst_addr: Optional[str] = None,
    src_port: Optional[int] = None,
    dst_port: Optional[int] = None,
) -> Flow:
    def rand_ip() -> str:
        return ".".join(str(randint(1, 254)) for _ in range(4))

    return Flow(
        src_addr=src_addr if src_addr is not None else rand_ip(),
        dst_addr=dst_addr if dst_addr is not None else rand_ip(),
        src_port=src_port if src_port is not None else randint(1, 0xFFFF),
        dst_port=dst_port if dst_port is not None else randint(1, 0xFFFF),
    )


def build_kvs_hdr(
    op: int = KVS_OP_PUT,
    key: Optional[bytes] = None,
    value: Optional[bytes] = None,
    status: int = KVS_STATUS_FAIL,
    port: int = 0,
) -> KVSHeader:
    if key is None:
        key = bytes(getrandbits(8) for _ in range(KVS_KEY_SIZE_BYTES))
    if value is None:
        value = bytes(getrandbits(8) for _ in range(KVS_VALUE_SIZE_BYTES))
    return KVSHeader(op=op, key=key, value=value, status=status, port=port)


def _finish_frame(pkt: Packet) -> Packet:
    # Pad to the minimum ethernet frame size without FCS (60B).
    if len(pkt) < 60:
        pkt /= b"\0" * (60 - len(pkt))
    # Re-parse so every length/checksum field is populated.
    return Ether(bytes(pkt))


def build_packet(
    src_mac: str = SRC_MAC,
    dst_mac: str = DST_MAC,
    flow: Optional[Flow] = None,
    kvs_hdr: Optional[KVSHeader] = None,
    payload: Optional[bytes] = None,
    proto: str = "udp",
) -> Packet:
    """Ethernet/IPv4/{UDP,TCP} packet for `flow` (random flow if None)."""
    flow = flow if flow is not None else build_flow()

    pkt = Ether(dst=dst_mac, src=src_mac)
    pkt /= IP(src=flow.src_addr, dst=flow.dst_addr)
    if proto == "udp":
        pkt /= UDP(sport=flow.src_port, dport=flow.dst_port)
    elif proto == "tcp":
        pkt /= TCP(sport=flow.src_port, dport=flow.dst_port)
    else:
        raise ValueError(f"unsupported proto {proto}")

    if kvs_hdr is not None:
        pkt /= kvs_hdr
    if payload is not None:
        pkt /= payload

    return _finish_frame(pkt)


def build_icmp_packet(flow: Optional[Flow] = None, src_mac: str = SRC_MAC, dst_mac: str = DST_MAC) -> Packet:
    """IPv4 packet that is neither TCP nor UDP."""
    flow = flow if flow is not None else build_flow()
    return _finish_frame(Ether(dst=dst_mac, src=src_mac) / IP(src=flow.src_addr, dst=flow.dst_addr) / ICMP())


def build_non_ip_packet(src_mac: str = SRC_MAC, dst_mac: str = DST_MAC) -> Packet:
    """Ethernet frame that is not IPv4 (an ARP request)."""
    return _finish_frame(Ether(dst=dst_mac, src=src_mac) / ARP(pdst="10.0.0.1", psrc="10.0.0.2"))


def bswap16(n: int) -> int:
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8)


def bswap32(n: int) -> int:
    return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24)


def step(msg: str) -> None:
    print(f"[*] {msg}", flush=True)


def run(test: Callable[[Ports], None], nf: str) -> None:
    """
    Entry point for NF test scripts.

    By default the testbed (model + controller for `nf`) must already be running
    (`testbed.py up <nf>`). With --up it is brought up before the test and torn down
    after it, unless --keep is given.
    """
    parser = ArgumentParser(description=f"Black-box test for {nf} on the Tofino 2 model")
    parser.add_argument("--nf", default=nf, help=f"synthesized solution to test (default: {nf})")
    parser.add_argument("--up", action="store_true", help="build (unless --no-build) and start the testbed first")
    parser.add_argument("--no-build", action="store_true", help="with --up: skip the build step")
    parser.add_argument("--keep", action="store_true", help="with --up: leave the testbed running afterwards")
    parser.add_argument("--quiet", action="store_true", help="don't print every packet sent/received")
    args = parser.parse_args()
    nf = args.nf

    try:
        testbed.require_root()
        if args.up:
            testbed.up(nf, do_build=not args.no_build)
        else:
            testbed.assert_up(nf)
    except testbed.TestbedError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(2)

    rc = 0
    ports = Ports(verbose=not args.quiet)
    try:
        test(ports)
        print(f"\n{nf}: PASSED")
    except TestFailure as e:
        print(f"\n*** TEST FAILED ***\n{e}", file=sys.stderr)
        print(f"\n{nf}: FAILED (logs in {testbed.nf_log_dir(nf)})", file=sys.stderr)
        rc = 1
    finally:
        ports.close()
        if args.up and not args.keep:
            testbed.down()

    sys.exit(rc)
