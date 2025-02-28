#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <chrono>

#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

#include "log.h"
#include "packet.h"
#include "store.h"
#include "constants.h"

namespace netcache {

Store::Store(const int64_t _processing_delay_ns, const int in, const int out) {
  processing_delay_ns = _processing_delay_ns;
  port_in             = in;
  port_out            = out;
}

Store::~Store() {}

void Store::run() {
  while (1) {
    struct rte_mbuf *mbufs[BURST_SIZE];
    uint16_t nb_rx    = rte_eth_rx_burst(port_in, 0, mbufs, BURST_SIZE);
    uint16_t tx_count = 0;

    for (uint16_t n = 0; n < nb_rx; n++) {
      LOG_DEBUG();
      LOG_DEBUG("Grabbing packet %u/%u.", n + 1, nb_rx);

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      int64_t elapsed_ns                          = 0;
      do {
        elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count();
      } while (processing_delay_ns > 0 && elapsed_ns < processing_delay_ns);

      LOG_DEBUG("Processing packet (elapsed %lu ns).", elapsed_ns);

      if (!check_pkt(mbufs[n])) {
        rte_pktmbuf_free(mbufs[n]);
        continue;
      }

      LOG_DEBUG("Processing netcache query...");
      modify_pkt(mbufs[n]);

      tx_count++;
    }

    uint16_t nb_tx = rte_eth_tx_burst(port_out, 0, mbufs, tx_count);
    for (uint16_t n = nb_tx; n < tx_count; n++) {
      rte_pktmbuf_free(mbufs[n]);
    }
  }
}

bool Store::check_pkt(rte_mbuf *mbuf) {
  uint32_t pkt_len = mbuf->pkt_len;

  constexpr const uint32_t min_size = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr) + sizeof(netcache_hdr_t);

  if (pkt_len < min_size) {
    LOG_DEBUG("Too small for a netcache packet (%u).", pkt_len);
    return false;
  }

  rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbuf, rte_ether_hdr *);

  if (rte_be_to_cpu_16(eth_hdr->ether_type) != ETHER_TYPE_IPV4) {
    LOG_DEBUG("Not IPv4 packet.");
    return false;
  }

  rte_ipv4_hdr *ipv4_hdr = (rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(rte_ether_hdr));

  if (ipv4_hdr->next_proto_id != UDP_PROTO && ipv4_hdr->next_proto_id != TCP_PROTO) {
    LOG_DEBUG("Not a UDP or TCP packet.");
    return false;
  }

  if (ipv4_hdr->next_proto_id == UDP_PROTO) {
    rte_udp_hdr *udp_hdr = (rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(rte_ipv4_hdr));
    if (rte_be_to_cpu_16(udp_hdr->dst_port) != KVSTORE_PORT) {
      LOG_DEBUG("KVS port mismatch (%u).", rte_be_to_cpu_16(udp_hdr->dst_port));
      return false;
    }
  } else if (ipv4_hdr->next_proto_id == TCP_PROTO) {
    if (pkt_len < sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_tcp_hdr) + sizeof(netcache_hdr_t)) {
      LOG_DEBUG("Not enough data for a netcache packet on top of TCP.");
      return false;
    }

    rte_tcp_hdr *tcp_hdr = (rte_tcp_hdr *)((uint8_t *)ipv4_hdr + sizeof(rte_ipv4_hdr));
    if (rte_be_to_cpu_16(tcp_hdr->dst_port) != KVSTORE_PORT) {
      LOG_DEBUG("KVS port mismatch (%u).", rte_be_to_cpu_16(tcp_hdr->dst_port));
      return false;
    }
  }

  return true;
}

void Store::modify_pkt(rte_mbuf *mbuf) {
  uint8_t *pkt_ptr = rte_pktmbuf_mtod(mbuf, uint8_t *);

  rte_ether_hdr *eth_hdr = (rte_ether_hdr *)pkt_ptr;
  pkt_ptr += sizeof(rte_ether_hdr);

  rte_ether_addr eth_tmp = eth_hdr->src_addr;
  eth_hdr->src_addr      = eth_hdr->dst_addr;
  eth_hdr->dst_addr      = eth_tmp;

  rte_ipv4_hdr *ip_hdr = (rte_ipv4_hdr *)pkt_ptr;
  pkt_ptr += sizeof(rte_ipv4_hdr);

  uint32_t temp_src = ip_hdr->src_addr;
  ip_hdr->src_addr  = ip_hdr->dst_addr;
  ip_hdr->dst_addr  = temp_src;

  ip_hdr->hdr_checksum = 0;
  ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

  if (ip_hdr->next_proto_id == IPPROTO_UDP) {
    rte_udp_hdr *udp_hdr = (rte_udp_hdr *)pkt_ptr;
    pkt_ptr += sizeof(rte_udp_hdr);

    uint16_t temp_port = udp_hdr->src_port;
    udp_hdr->src_port  = udp_hdr->dst_port;
    udp_hdr->dst_port  = temp_port;
  } else if (ip_hdr->next_proto_id == IPPROTO_TCP) {
    rte_tcp_hdr *tcp_hdr = (rte_tcp_hdr *)pkt_ptr;
    pkt_ptr += sizeof(rte_tcp_hdr);

    uint16_t temp_port = tcp_hdr->src_port;
    tcp_hdr->src_port  = tcp_hdr->dst_port;
    tcp_hdr->dst_port  = temp_port;
  }

  std::array<uint8_t, KV_KEY_SIZE> kv_key;

  netcache_hdr_t *nc_hdr = (netcache_hdr_t *)pkt_ptr;
  std::copy(nc_hdr->key, nc_hdr->key + sizeof(nc_hdr->key), kv_key.begin());

  auto it = kv_map.find(kv_key);
  if (it == kv_map.end()) {
    memset(nc_hdr->val, 0, sizeof(nc_hdr->val));
    nc_hdr->status = 1;
  } else {
    memcpy(nc_hdr->val, it->second, sizeof(nc_hdr->val));
    nc_hdr->status = 0;
  }
}

} // namespace netcache
