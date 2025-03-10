from sys import exit
from binascii import hexlify, unhexlify
from dataclasses import dataclass
from socket import socket, AF_PACKET, SOCK_RAW, ntohs, inet_aton
from select import select
from random import randint
from typing import Optional
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet


ETH_P_ALL = 3
PACKET_RECEIVE_TIMEOUT = 1  # seconds

SRC_MAC = "02:00:00:DD:EE:FF"
DST_MAC = "02:00:00:AA:BB:CC"


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

    def send(self, port: int, pkt: Packet) -> None:
        src_addr = pkt[IP].src
        dst_addr = pkt[IP].dst
        src_port = pkt[UDP].sport
        dst_port = pkt[UDP].dport

        print(f"Sent {port}: {src_addr}:{src_port} -> {dst_addr}:{dst_port}")
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
                if data[:6] != unhexlify(DST_MAC.replace(":", "")):
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
                src_addr = pkt[IP].src
                dst_addr = pkt[IP].dst
                src_port = pkt[UDP].sport
                dst_port = pkt[UDP].dport
                print(f"Recv {port}: {src_addr}:{src_port} -> {dst_addr}:{dst_port}")

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


def expect_packet_from_port(ports: Ports, port: int, pkt: Packet) -> Packet:
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
    if recv_pkt.build() != pkt.build():
        print("*** ASSERTION FAILED ***")
        print(f"Packet comparison failed!")
        print(f"Expected: {pkt.build().hex()}")
        print(f"Received: {recv_pkt.build().hex()}")
        exit(1)

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
    if not src_addr:
        src_addr = f"{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}"
    if not dst_addr:
        dst_addr = f"{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}"
    if not src_port:
        src_port = randint(0, 0xFFFF)
    if not dst_port:
        dst_port = randint(0, 0xFFFF)

    return Flow(src_addr=src_addr, dst_addr=dst_addr, src_port=src_port, dst_port=dst_port)


def build_packet(flow: Flow) -> Packet:
    pkt = Ether(dst=DST_MAC, src=SRC_MAC)
    pkt /= IP(src=flow.src_addr, dst=flow.dst_addr)
    pkt /= UDP(sport=flow.src_port, dport=flow.dst_port)

    # Pad packet to minimum ethernet frame without CRC (60B)
    if len(pkt) < 60:
        pkt /= b"\0" * (60 - len(pkt))

    return pkt
