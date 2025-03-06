#!/usr/bin/env python3

from dataclasses import dataclass
from random import randint
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet

from socket import socket, AF_PACKET, SOCK_RAW, ntohs
from select import select

import os
import binascii

ETH_P_ALL = 3
PACKET_RECEIVE_TIMEOUT = 1  # seconds
RECIRCULATION_PORT = 6
LAN_PORTS = [p for p in range(3, 33)]

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
                if data[:6] != binascii.unhexlify(DST_MAC.replace(":", "")):
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


@dataclass
class Flow:
    src_addr: str
    dst_addr: str
    src_port: int
    dst_port: int


def build_random_flow() -> Flow:
    return Flow(
        src_addr=f"{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}",
        dst_addr=f"{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}.{randint(0, 0xFF)}",
        src_port=randint(0, 0xFFFF),
        dst_port=randint(0, 0xFFFF),
    )


def build_packet(flow: Flow) -> Packet:
    pkt = Ether(dst=DST_MAC, src=SRC_MAC)
    pkt /= IP(src=flow.src_addr, dst=flow.dst_addr)
    pkt /= UDP(sport=flow.src_port, dport=flow.dst_port)

    # Pad packet to 60 bytes
    if len(pkt) < 60:
        pkt /= b"\0" * (60 - len(pkt))

    return pkt


def main():
    ports = Ports(LAN_PORTS)

    for port in LAN_PORTS:
        print(f"[*] Testing port {port}...")

        flow = build_random_flow()
        pkt = build_packet(flow)

        ports.send(port, pkt)
        data = ports.poll()

        assert len(data.keys()) == 1
        assert port in data
        assert len(data[port]) == 1
        assert data[port][0].build() == pkt.build()


if __name__ == "__main__":
    if os.geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
