from scapy.all import *
import sys

class NetCacheHeader(Packet):
    fields_desc = [
        ByteField("op", 0),
        ByteField("seq", 0),
        ShortField("key", 0),
        IntField("val", 0)
    ]

def send_pkt(veth, cur_op, cur_key, cur_val):
    dest_ip = "10.0.0.10"
    dest_port = 50000

    # dst_mac = ''

    # try:
    #     dst_mac = get_if_hwaddr(veth)
    # except Exception as e:
    #     print(f"Error retrieving MAC address for {veth}: {e}")

    nc_header = NetCacheHeader(op=cur_op, seq=0, key=cur_key, val=cur_val)

    ether = Ether(src="01:02:03:04:05:06", dst="11:22:33:44:55:66")
    ip = IP(src="192.168.1.10", dst="192.168.1.1")
    tcp = TCP(sport=12345, dport=50000, flags="S", seq=1000)

    pkt = ether / ip / tcp / nc_header

    sendp(pkt, iface=veth)

def write_pkt(file_name, cur_op, cur_key, cur_val):
    dest_ip = "10.0.0.10"
    dest_port = 50000

    nc_header = NetCacheHeader(op=cur_op, seq=0, key=cur_key, val=cur_val)

    ether = Ether(src="01:02:03:04:05:06", dst="11:22:33:44:55:66")
    ip = IP(src="192.168.1.10", dst="192.168.1.1")
    tcp = TCP(sport=12345, dport=50000, flags="S", seq=1000)

    pkt = ether / ip / tcp / nc_header

    wrpcap(f'{file_name}.pcap', pkt, append=True)

if __name__ == "__main__":
    mode    = str(sys.argv[1])
    # mode s: veth; mode w: pcap name
    veth    = str(sys.argv[2])
    pkt_num = int(sys.argv[3])
    cur_op  = int(sys.argv[4])
    cur_key = int(sys.argv[5])
    cur_val = int(sys.argv[6])

    if mode == 'w':
        for i in range(pkt_num):
            write_pkt(veth, cur_op, cur_key, cur_val)
    elif mode == 's':
        for i in range(pkt_num):
            send_pkt(veth, cur_op, cur_key, cur_val)
    else:
        print('Available modes are: write (w) or send(s).')
