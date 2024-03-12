#!/usr/bin/env python3

import argparse
import struct
import socket

from pathlib import Path

from scapy.layers.inet import IP, UDP
from scapy.packet import Packet
from scapy.utils import PcapReader

from crc import Calculator, Configuration

class Flow:
    def __init__(self, src_ip: int, dst_ip: int, src_port: int, dst_port: int):
        self.src_ip = src_ip
        self.dst_ip = dst_ip
        self.src_port = src_port
        self.dst_port = dst_port
    
    def get_id(self) -> int:
        id = self.src_ip
        id = id << 32
        id = self.dst_ip
        id = id << 32
        id = self.src_port
        id = id << 16
        id = self.dst_port
        return id

class CRC():
    def __init__(
        self,
        width=32,
        poly=0x04C11DB7,
        reflect_in=True,
        xor_in=0xffffffff,
        reflect_out=True,
        xor_out=0xffffffff
    ):
        super().__init__()
        config = Configuration(
            width=width,
            polynomial=poly,
            init_value=xor_in,
            final_xor_value=xor_out,
            reverse_input=reflect_in,
            reverse_output=reflect_out,
        )

        self.calculator = Calculator(config)
    
    def hash(self, flow: Flow, bits: int) -> int:
        stream = struct.pack("!LLHH", flow.src_ip, flow.dst_ip, flow.src_port, flow.dst_port)
        mask = ((1<<bits)-1)
        return self.calculator.checksum(stream) & mask

def get_flow(pkt: Packet) -> Flow:
    assert IP in pkt
    assert UDP in pkt
    
    src_ip = struct.unpack("!L", socket.inet_aton(pkt["IP"].src))[0]
    dst_ip = struct.unpack("!L", socket.inet_aton(pkt["IP"].dst))[0]
    src_port = pkt["UDP"].sport
    dst_port = pkt["UDP"].dport

    return Flow(src_ip, dst_ip, src_port, dst_port)

def analyze_pcap(pcap: Path, hash_size: int):
    crc = CRC()
    hashes = set()

    total_pkts = 0
    collisions = 0

    for pkt in PcapReader(str(pcap)):
        flow = get_flow(pkt)
        hash = crc.hash(flow, hash_size)

        if hash in hashes:
            collisions += 1

        total_pkts += 1
        hashes.add(hash)

        # print(pkt)
        # print(f"Hash: 0x{hash:x}")
        # input()
    
    print(f"Total packets:  {total_pkts:,}")
    print(f"Hash capacity:  {(1<<hash_size)-1:,}")
    print(f"Collisions:     {collisions:,} ({100.0*collisions/total_pkts:5.2f}%)")
    print(f"Collision free: {(total_pkts-collisions):,} ({100.0*(1-collisions/total_pkts):5.2f}%)")

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("pcap", type=str, help="Traffic sample")
    parser.add_argument("--size", type=int, default=32, help="Hash size (bits)")
    
    args = parser.parse_args()

    pcap = Path(args.pcap)
    assert pcap.exists()

    analyze_pcap(pcap, args.size)

if __name__ == "__main__":
    main()
