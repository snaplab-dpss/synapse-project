#pragma once

#include <arpa/inet.h>

#include "bytes.h"
#include "util.h"

namespace sycon {

typedef byte_t mac_addr_t[6];
typedef uint32_t ipv4_addr_t;
typedef uint16_t port_t;

struct cpu_hdr_t {
  uint16_t code_path;
  uint16_t in_port;
  uint16_t out_port;
} __attribute__((packed));

struct eth_hdr_t {
  mac_addr_t dst_mac;
  mac_addr_t src_mac;
  uint16_t eth_type;
} __attribute__((packed));

struct ipv4_hdr_t {
  uint8_t version_ihl;
  uint8_t ecn_dscp;
  uint16_t tot_len;
  uint16_t id;
  uint16_t frag_off;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t check;
  ipv4_addr_t src_ip;
  ipv4_addr_t dst_ip;
} __attribute__((packed));

struct tcpudp_hdr_t {
  port_t src_port;
  port_t dst_port;
} __attribute__((packed));

byte_t *packet_consume(uint16_t bytes);
byte_t *packet_release(uint16_t bytes);

void print(const cpu_hdr_t *cpu_hdr);
void print(const eth_hdr_t *eth_hdr);
void print(const ipv4_hdr_t *ipv4_hdr);
void print(const tcpudp_hdr_t *tcpudp_hdr);

unsigned ether_addr_hash(mac_addr_t addr);
uint16_t ipv4_cksum(const ipv4_hdr_t *ipv4_hdr);
uint16_t update_ipv4_tcpudp_checksums(const ipv4_hdr_t *ipv4_hdr,
                                      const void *l4_hdr);

}  // namespace sycon