#pragma once

#include "util.hpp"

namespace sycon {

typedef byte_t mac_addr_t[6];
typedef u32 ipv4_addr_t;
typedef u16 port_t;

struct cpu_hdr_t {
  u16 code_path;
  u16 in_port;
  u16 out_port;
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

struct tcpudp_hdr_t {
  port_t src_port;
  port_t dst_port;
} __attribute__((packed));

byte_t *packet_consume(byte_t *packet, u16 bytes);

void packet_hexdump(byte_t *pkt, u16 size);

void packet_log(const cpu_hdr_t *cpu_hdr);
void packet_log(const eth_hdr_t *eth_hdr);
void packet_log(const ipv4_hdr_t *ipv4_hdr);
void packet_log(const tcpudp_hdr_t *tcpudp_hdr);

unsigned ether_addr_hash(mac_addr_t addr);
u16 ipv4_cksum(const ipv4_hdr_t *ipv4_hdr);
u16 update_ipv4_tcpudp_checksums(const ipv4_hdr_t *ipv4_hdr, const void *l4_hdr);

} // namespace sycon