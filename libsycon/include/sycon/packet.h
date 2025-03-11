#pragma once

#include "util.h"

namespace sycon {

typedef u8 mac_addr_t[6];
typedef u32 ipv4_addr_t;
typedef u16 port_t;

struct cpu_hdr_t {
  u16 code_path;
  u16 egress_dev;
} __attribute__((packed));

struct eth_hdr_t {
  mac_addr_t dst_mac;
  mac_addr_t src_mac;
  u16 eth_type;
} __attribute__((packed));

struct ipv4_hdr_t {
  u8 version_ihl;
  u8 ecn_dscp;
  u16 tot_len;
  u16 id;
  u16 frag_off;
  u8 ttl;
  u8 protocol;
  u16 check;
  ipv4_addr_t src_ip;
  ipv4_addr_t dst_ip;
} __attribute__((packed));

struct udp_hdr_t {
  u16 src_port;
  u16 dst_port;
  u16 dgram_len;
  u16 dgram_cksum;
} __attribute__((packed));

struct tcp_hdr_t {
  u16 src_port;
  u16 dst_port;
  u32 sent_seq;
  u32 recv_ack;
  u8 data_off;
  u8 tcp_flags;
  u16 rx_win;
  u16 cksum;
  u16 tcp_urp;
} __attribute__((packed));

struct tcpudp_hdr_t {
  port_t src_port;
  port_t dst_port;
} __attribute__((packed));

extern bytes_t packet_consumed;
extern bytes_t packet_size;

template <typename T> T *packet_consume(u8 *packet_base) {
  bytes_t size = sizeof(T);
  assert(packet_consumed + size <= packet_size);
  u8 *header = packet_base + packet_consumed;
  packet_consumed += size;
  return reinterpret_cast<T *>(header);
}

u8 *packet_consume(u8 *packet_base, bytes_t size);
void packet_hexdump(u8 *pkt, u16 size);

void packet_log(const cpu_hdr_t *cpu_hdr);
void packet_log(const eth_hdr_t *eth_hdr);
void packet_log(const ipv4_hdr_t *ipv4_hdr);
void packet_log(const tcpudp_hdr_t *tcpudp_hdr);

unsigned ether_addr_hash(mac_addr_t addr);
void update_ipv4_tcpudp_checksums(void *ipv4_hdr, void *l4_hdr);

} // namespace sycon