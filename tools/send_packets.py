#!/usr/bin/env python3

import random
import socket
import struct

from scapy.sendrecv import AsyncSniffer, sendp
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet

from typing import Optional

CPU_IFACE = "veth250"

port_to_iface = {}
port_sending_packet: Optional[int] = None

packets = {}

def build_ifaces():
    for port in range(32):
        iface = f"veth{port*2}"
        port_to_iface[port] = iface
    port_to_iface["CPU"] = CPU_IFACE

def show_available_ports():
    print("Available ports/interfaces:")
    for port, iface in port_to_iface.items():
        print(f"  Port {port} iface {iface}")
    print()

def random_value(bits):
    return random.randint(0, (1 << bits) - 1)

def random_ip():
    ip = random_value(32)
    return socket.inet_ntoa(struct.pack('!L', ip))

def random_port():
    return random_value(16)

def random_mac():
    bits = 48
    mac = random_value(bits)
    mac_bytes = [ f"{(mac >> offset) & 0xff:02x}" for offset in range(0, bits, 8) ]
    return ":".join(mac_bytes)

def flow_to_str(src_ip, dst_ip, src_port, dst_port):
    return f"{src_ip}:{src_port} -> {dst_ip}:{dst_port}"

def get_packet_flow(pkt: Packet):
    assert IP in pkt
    assert UDP in pkt
    
    src_ip = str(pkt["IP"].src)
    dst_ip = str(pkt["IP"].dst)
    src_port = str(pkt["UDP"].sport)
    dst_port = str(pkt["UDP"].dport)

    return flow_to_str(src_ip, dst_ip, src_port, dst_port)

def callback_generator(port):
    def callback(pkt):
        global port_sending_packet

        if port_sending_packet != None and port == port_sending_packet:
            port_sending_packet = None
            return
        
        print(f"[RECV {str(port):>3}] {get_packet_flow(pkt)}")

    return callback

def listen():
    sniffers = []
    for port, iface in port_to_iface.items():
        sniffer = AsyncSniffer(
            iface=iface,
            store=False,
            prn=callback_generator(port),
        )
        sniffer.start()
        sniffers.append(sniffer)
    return sniffers

def get_pkt(flow_index):
    if flow_index in packets:
        return packets[flow_index]
    
    smac = random_mac()
    dmac = random_mac()

    src_ip = random_ip()
    dst_ip = random_ip()
    src_port = random_port()
    dst_port = random_port()
    
    pkt = Ether(dst=dmac, src=smac)
    pkt = pkt / IP(src=src_ip, dst=dst_ip)
    pkt = pkt / UDP(sport=src_port, dport=dst_port)

    packets[flow_index] = pkt

    return pkt

def cmd_help():
    print(f"Usage: port index [*]")
    print(f"  port:  port number (int)")
    print(f"  index: flow index (int)")
    print(f"  *:     send CRC32 collision (int)")

def parse_cmd(cmd) -> Optional[tuple[int, int, bool]]:
    cmd = cmd.split(" ")

    try:
        port = int(cmd[0])
        index = int(cmd[1])

        if len(cmd) == 2:
            return (port, index, False)
        
        if cmd[2] != "*":
            cmd_help()
            return None
        
        return (port, index, True)
        
    except ValueError:
        cmd_help()
        return None
    
    except IndexError:
        cmd_help()
        return None

def send(port: int, pkt) -> bool:
    global port_sending_packet

    if port not in port_to_iface:
        return False
    
    iface = port_to_iface[port]
    port_sending_packet = port

    print(f"[SENT {port:>3}] {get_packet_flow(pkt)}")
    sendp(pkt, iface=iface, verbose=0)

    return True

def main():
    build_ifaces()
    show_available_ports()

    sniffers = listen()

    while True:
        cmd = input("Port to send packet: ")
        parsed_cmd = parse_cmd(cmd)

        if not parsed_cmd:
            continue
        
        port, flow_index, collision = parsed_cmd
        pkt = get_pkt(flow_index)

        if not send(port, pkt):
            print(f"Invalid port {port}")

if __name__ == "__main__":
    main()
