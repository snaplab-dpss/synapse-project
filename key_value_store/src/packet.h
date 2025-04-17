#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "constants.h"
#include "log.h"

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

namespace netcache {

struct netcache_hdr_t {
  uint8_t op;
  uint8_t key[KV_KEY_SIZE];
  uint8_t val[KV_VAL_SIZE];
  uint8_t status;
  uint16_t port;
} __attribute__((packed));

inline std::string byte_array_to_string(const uint8_t *array, size_t size) {
  std::stringstream ss;

  ss << "0x";
  for (size_t i = 0; i < size; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)array[i];
  }

  return ss.str();
}

inline void log_pkt(const rte_mbuf *mbuf) {
  const size_t pkt_len     = mbuf->pkt_len;
  size_t pkt_len_remaining = pkt_len;
  uint8_t *pkt_ptr         = rte_pktmbuf_mtod(mbuf, uint8_t *);

  auto log_payload = [pkt_ptr, pkt_len, pkt_len_remaining]() {
    if (pkt_len_remaining == 0) {
      return;
    }
    const uint8_t *payload   = pkt_ptr;
    const size_t payload_len = pkt_len_remaining;

    LOG_DEBUG("###[ Payload ]###");
    LOG_DEBUG("  len %zu", payload_len);
    LOG_DEBUG("  data %s", byte_array_to_string(payload, payload_len).c_str());
  };

  rte_ether_hdr *rte_ether_header = (rte_ether_hdr *)pkt_ptr;
  pkt_ptr += sizeof(rte_ether_hdr);
  pkt_len_remaining -= sizeof(rte_ether_hdr);

  LOG_DEBUG("###[ Ethernet ]###");
  LOG_DEBUG("  dst  %02x:%02x:%02x:%02x:%02x:%02x", rte_ether_header->dst_addr.addr_bytes[0], rte_ether_header->dst_addr.addr_bytes[1],
            rte_ether_header->dst_addr.addr_bytes[2], rte_ether_header->dst_addr.addr_bytes[3], rte_ether_header->dst_addr.addr_bytes[4],
            rte_ether_header->dst_addr.addr_bytes[5]);
  LOG_DEBUG("  src  %02x:%02x:%02x:%02x:%02x:%02x", rte_ether_header->src_addr.addr_bytes[0], rte_ether_header->src_addr.addr_bytes[1],
            rte_ether_header->src_addr.addr_bytes[2], rte_ether_header->src_addr.addr_bytes[3], rte_ether_header->src_addr.addr_bytes[4],
            rte_ether_header->src_addr.addr_bytes[5]);
  LOG_DEBUG("  type 0x%x", rte_bswap16(rte_ether_header->ether_type));

  if (rte_be_to_cpu_16(rte_ether_header->ether_type) != RTE_ETHER_TYPE_IPV4) {
    log_payload();
    return;
  }

  rte_ipv4_hdr *rte_ipv4_header = (rte_ipv4_hdr *)pkt_ptr;
  pkt_ptr += sizeof(rte_ipv4_hdr);
  pkt_len_remaining -= sizeof(rte_ipv4_hdr);

  LOG_DEBUG("###[ IP ]###");
  LOG_DEBUG("  ihl     %u", (rte_ipv4_header->version_ihl & 0x0f));
  LOG_DEBUG("  version %u", (rte_ipv4_header->version_ihl & 0xf0) >> 4);
  LOG_DEBUG("  tos     %u", rte_ipv4_header->type_of_service);
  LOG_DEBUG("  len     %u", rte_bswap16(rte_ipv4_header->total_length));
  LOG_DEBUG("  id      %u", rte_bswap16(rte_ipv4_header->packet_id));
  LOG_DEBUG("  off     %u", rte_bswap16(rte_ipv4_header->fragment_offset));
  LOG_DEBUG("  ttl     %u", rte_ipv4_header->time_to_live);
  LOG_DEBUG("  proto   %u", rte_ipv4_header->next_proto_id);
  LOG_DEBUG("  chksum  0x%x", rte_bswap16(rte_ipv4_header->hdr_checksum));
  LOG_DEBUG("  src     %u.%u.%u.%u", (rte_ipv4_header->src_addr >> 0) & 0xff, (rte_ipv4_header->src_addr >> 8) & 0xff,
            (rte_ipv4_header->src_addr >> 16) & 0xff, (rte_ipv4_header->src_addr >> 24) & 0xff);
  LOG_DEBUG("  dst     %u.%u.%u.%u", (rte_ipv4_header->dst_addr >> 0) & 0xff, (rte_ipv4_header->dst_addr >> 8) & 0xff,
            (rte_ipv4_header->dst_addr >> 16) & 0xff, (rte_ipv4_header->dst_addr >> 24) & 0xff);

  if (rte_ipv4_header->next_proto_id != IPPROTO_UDP) {
    log_payload();
    return;
  }

  rte_udp_hdr *udp_header = (rte_udp_hdr *)pkt_ptr;
  pkt_ptr += sizeof(rte_udp_hdr);
  pkt_len_remaining -= sizeof(rte_udp_hdr);

  LOG_DEBUG("###[ UDP ]###");
  LOG_DEBUG("  sport   %u", rte_bswap16(udp_header->src_port));
  LOG_DEBUG("  dport   %u", rte_bswap16(udp_header->dst_port));

  if (rte_be_to_cpu_16(udp_header->dst_port) != KVSTORE_PORT && rte_be_to_cpu_16(udp_header->src_port) != KVSTORE_PORT) {
    log_payload();
    return;
  }

  netcache_hdr_t *nc_hdr = (netcache_hdr_t *)pkt_ptr;
  pkt_ptr += sizeof(netcache_hdr_t);
  pkt_len_remaining -= sizeof(netcache_hdr_t);

  LOG_DEBUG("###[ KVS ]###");
  LOG_DEBUG("  op     %u", nc_hdr->op);
  LOG_DEBUG("  key    %s", byte_array_to_string(nc_hdr->key, sizeof(nc_hdr->key)).c_str());
  LOG_DEBUG("  val    %s", byte_array_to_string(nc_hdr->val, sizeof(nc_hdr->val)).c_str());
  LOG_DEBUG("  status %u", nc_hdr->status);
  LOG_DEBUG("  port   %u", rte_bswap16(nc_hdr->port));

  log_payload();
}

}; // namespace netcache
