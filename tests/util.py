from sys import exit
from binascii import hexlify, unhexlify
from dataclasses import dataclass
from socket import socket, AF_PACKET, SOCK_RAW, ntohs, inet_aton
from select import select
from random import randint, getrandbits
from typing import Optional
from enum import Enum

from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet, bind_layers
from scapy.fields import ByteField, StrFixedLenField, ShortField

ETH_P_ALL = 3
PACKET_RECEIVE_TIMEOUT = 1  # seconds

SRC_MAC = "02:00:00:DD:EE:FF"
DST_MAC = "02:00:00:AA:BB:CC"

KVS_PORT = 670
KVS_KEY_SIZE_BYTES = 16
KVS_VALUE_SIZE_BYTES = 128
KVS_OP_GET = 0
KVS_OP_PUT = 1
KVS_STATUS_OK = 1
KVS_STATUS_FAIL = 0


class HeaderField(Enum):
    IP_CHECKSUM = 0
    UDP_CHECKSUM = 1
    KVS_CLIENT_PORT = 2


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
        return KVSHeader if isinstance(pkt, UDP) and pkt.dport == KVS_PORT else None


bind_layers(UDP, KVSHeader)


def pkt_to_string(pkt: Packet) -> str:
    assert IP in pkt
    assert UDP in pkt

    if KVSHeader in pkt:
        return f"KVS{{key={pkt[KVSHeader].key.hex()}, value={pkt[KVSHeader].value.hex()}}}"
    else:
        return f"{pkt[IP].src}:{pkt[UDP].sport} -> {pkt[IP].dst}:{pkt[UDP].dport}"


class Ports:
    def __init__(self, ports: list[int]):
        self.port_to_iface: dict[int, str] = {}
        self.iface_to_port: dict[str, int] = {}

        self.port_to_socket: dict[int, socket] = {}
        self.socket_to_port: dict[socket, int] = {}

        for port in ports:
            iface = f"veth{(port-1)*2}"
            self.port_to_iface[port] = iface
            self.iface_to_port[iface] = port

            s = socket(AF_PACKET, SOCK_RAW, ntohs(ETH_P_ALL))
            s.bind((iface, 0))
            s.settimeout(PACKET_RECEIVE_TIMEOUT)

            self.port_to_socket[port] = s
            self.socket_to_port[s] = port

    def send(self, port: int, pkt: Packet, quiet: bool = False) -> None:
        if not quiet:
            print(f"Sent {port}: {pkt_to_string(pkt)}")

        self.port_to_socket[port].send(pkt.build())

    def poll(self) -> dict[int, list[Packet]]:
        result: dict[int, list[Packet]] = {}
        raw_data: dict[int, bytes] = {}

        pending = True
        while pending:
            pending = False
            sockets_ready, _, _ = select(self.socket_to_port.keys(), [], [], PACKET_RECEIVE_TIMEOUT)
            for socket in sockets_ready:
                pending = True
                data = socket.recv(1500 * 8)
                port = self.socket_to_port[socket]
                raw_data[port] = raw_data.get(port, b"") + data

        for port, data in raw_data.items():
            packets: list[Packet] = []

            while data:
                if data[:6] != unhexlify(DST_MAC.replace(":", "")) and data[:6] != unhexlify(SRC_MAC.replace(":", "")):
                    data = data[1:]
                    continue

                pkt = Ether(data)
                pktlen = len(pkt)

                if Ether not in pkt or IP not in pkt or UDP not in pkt:
                    data = data[pktlen:]
                    continue

                packets.append(pkt)

                data = data[pktlen:]

            for pkt in packets:
                print(f"Recv {port}: {pkt_to_string(pkt)}")

                if port not in result:
                    result[port] = []

                result[port].append(pkt)

        return result


def expect_no_packet(ports: Ports) -> None:
    data = ports.poll()
    if data:
        print("*** ASSERTION FAILED ***")
        print(f"Expected no packets, got packets from ports {list(data.keys())}")
        exit(1)


def assert_equal_packets(pkt1: Packet, pkt2: Packet, ignore_fields: list[HeaderField]):
    pkt1_copy = pkt1.copy()
    pkt2_copy = pkt2.copy()

    for field in ignore_fields:
        if field == HeaderField.IP_CHECKSUM:
            assert IP in pkt1_copy and IP in pkt2_copy
            pkt1_copy[IP].chksum = 0
            pkt2_copy[IP].chksum = 0
        elif field == HeaderField.UDP_CHECKSUM:
            assert UDP in pkt1_copy and UDP in pkt2_copy
            pkt1_copy[UDP].chksum = 0
            pkt2_copy[UDP].chksum = 0
        elif field == HeaderField.KVS_CLIENT_PORT:
            assert KVSHeader in pkt1_copy and KVSHeader in pkt2_copy
            pkt1_copy[KVSHeader].port = 0
            pkt2_copy[KVSHeader].port = 0

    pkt1_bytes = pkt1_copy.build()
    pkt2_bytes = pkt2_copy.build()

    if pkt1_bytes != pkt2_bytes:
        print("*** ASSERTION FAILED ***")
        print(f"Packet comparison failed!")

        print()
        print("Expected:")
        pkt1_copy.show()

        print()
        print("Received:")
        pkt2_copy.show()

        print()
        print("Hexdump:")
        print(f"  Expected: {pkt1_bytes.hex()}")
        print(f"  Received: {pkt2_bytes.hex()}")

        exit(1)


def expect_packet_from_port(
    ports: Ports,
    port: int,
    pkt: Packet,
    ignore_fields: list[HeaderField] = [HeaderField.IP_CHECKSUM, HeaderField.UDP_CHECKSUM],
) -> Packet:
    data = ports.poll()

    if len(data.keys()) != 1 or port not in data:
        print("*** ASSERTION FAILED ***")
        print(f"Expected a packet from port {port}, but got packets from ports {list(data.keys())}")
        exit(1)

    if len(data[port]) != 1:
        print("*** ASSERTION FAILED ***")
        print(f"Expected a single packet, got {len(data[port])} packets")
        assert False and "Expected a single packet"

    recv_pkt = data[port][0]
    assert_equal_packets(pkt, recv_pkt, ignore_fields)

    return recv_pkt


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
        src_addr = new_src_addr if new_src_addr is not None else self.src_addr
        dst_addr = new_dst_addr if new_dst_addr is not None else self.dst_addr
        src_port = new_src_port if new_src_port is not None else self.src_port
        dst_port = new_dst_port if new_dst_port is not None else self.dst_port
        return Flow(src_addr=src_addr, dst_addr=dst_addr, src_port=src_port, dst_port=dst_port)

    def invert(self) -> "Flow":
        return Flow(
            src_addr=self.dst_addr,
            dst_addr=self.src_addr,
            src_port=self.dst_port,
            dst_port=self.src_port,
        )

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
    if src_addr is None:
        src_addr = f"{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}"
    if dst_addr is None:
        dst_addr = f"{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}"
    if src_port is None:
        src_port = randint(0, 0xFFFF)
    if dst_port is None:
        dst_port = randint(0, 0xFFFF)

    return Flow(src_addr=src_addr, dst_addr=dst_addr, src_port=src_port, dst_port=dst_port)


def build_kvs_hdr(
    op: Optional[int] = None,
    key: Optional[bytes] = None,
    value: Optional[bytes] = None,
    status: Optional[int] = None,
    port: Optional[int] = None,
) -> KVSHeader:
    if op is None:
        op = KVS_OP_PUT
    if key is None:
        key = bytes(getrandbits(8) for _ in range(KVS_KEY_SIZE_BYTES))
    if value is None:
        value = bytes(getrandbits(8) for _ in range(KVS_VALUE_SIZE_BYTES))
    if status is None:
        status = KVS_STATUS_FAIL
    if port is None:
        port = 0

    return KVSHeader(op=op, key=key, value=value, status=status, port=port)


def build_packet(
    src_mac: str = SRC_MAC,
    dst_mac: str = DST_MAC,
    flow: Optional[Flow] = None,
    kvs_hdr: Optional[KVSHeader] = None,
) -> Packet:
    flow = flow if flow is not None else build_flow()

    pkt = Ether(dst=dst_mac, src=src_mac)
    pkt /= IP(src=flow.src_addr, dst=flow.dst_addr)
    pkt /= UDP(sport=flow.src_port, dport=flow.dst_port)

    if kvs_hdr is not None:
        pkt /= kvs_hdr
    else:
        # Pad packet to minimum ethernet frame without CRC (60B)
        if len(pkt) < 60:
            pkt /= b"\0" * (60 - len(pkt))

    # Force population of fields
    pkt = Ether(pkt.build())

    return pkt


def bswap16(n: int) -> int:
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8)


def bswap32(n: int) -> int:
    return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24)
