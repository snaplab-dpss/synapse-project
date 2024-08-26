#!/usr/bin/env python3

import random
import socket
import struct

from scapy.sendrecv import AsyncSniffer, sendp
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet, bind_layers

from typing import Optional

# This modifies the input() function, allowing the user to
# use arrow-up to scroll through the input history.
try:
    import readline
except:
    pass #readline not available

IP_PROTOCOLS_UDP = 17
IP_PROTOCOLS_WARMUP = 146

CPU_IFACE = "veth250"

port_to_iface = {}
port_sending_packet: Optional[int] = None

packets: dict[dict, Packet] = {}

# All transport protocols are UDP
bind_layers(IP, UDP)

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

def flow_to_str(src_ip, dst_ip, src_port, dst_port, is_warmup_pkt):
    pkt_str = f"{src_ip}:{src_port} -> {dst_ip}:{dst_port}"
    if is_warmup_pkt:
        pkt_str += f" [WARMUP]"
    return pkt_str

def get_packet_flow(pkt: Packet):
    assert IP in pkt
    assert UDP in pkt
    
    src_ip = str(pkt["IP"].src)
    dst_ip = str(pkt["IP"].dst)
    src_port = str(pkt["UDP"].sport)
    dst_port = str(pkt["UDP"].dport)

    is_warmup_pkt = pkt["IP"].proto == IP_PROTOCOLS_WARMUP

    return flow_to_str(src_ip, dst_ip, src_port, dst_port, is_warmup_pkt)

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
    if flow_index not in packets:
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

    # Return a copy, so that future changes are not propagated.
    return packets[flow_index].copy()

def cmd_help():
    print(f"Usage: port index warmup")
    print(f"  port:  port number (int)")
    print(f"  index: flow index (int)")
    print(f"  warmup: warmup packet (bool: 0 or 1)")

def parse_cmd(cmd) -> Optional[tuple[int, int, bool]]:
    cmd = cmd.split(" ")

    try:
        port = int(cmd[0])
        index = int(cmd[1])
        warmup = int(cmd[2])
        return port, index, warmup != 0
        
    except ValueError:
        cmd_help()
        return None
    
    except IndexError:
        cmd_help()
        return None

def send(port: int, pkt: Packet) -> bool:
    global port_sending_packet

    if port not in port_to_iface:
        return False
    
    iface = port_to_iface[port]
    port_sending_packet = port

    print(f"[SENT {port:>3}] {get_packet_flow(pkt)}")
    print(pkt)
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
        
        port, flow_index, is_warmup_pkt = parsed_cmd
        pkt = get_pkt(flow_index)

        print(is_warmup_pkt, get_packet_flow(pkt))

        if is_warmup_pkt:
            pkt["IP"].proto = IP_PROTOCOLS_WARMUP
        else:
            pkt["IP"].proto = IP_PROTOCOLS_UDP

        if not send(port, pkt):
            print(f"Invalid port {port}")

if __name__ == "__main__":
    main()
