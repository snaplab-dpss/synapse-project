#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

#include "clock.h"
#include "log.h"
#include "packet.h"
#include "store.h"
#include "constants.h"

namespace netcache {

Store::Store(const int64_t _processing_delay_ns, const int in, const int out, const uint16_t rx_queue)
    : processing_delay_ns(_processing_delay_ns), port_in(in), port_out(out), queue(rx_queue), kv_map(KVSTORE_CAPACITY) {}

Store::~Store() {}

void Store::run() {
  set_delta_ticks(processing_delay_ns * BURST_SIZE);

  struct rte_mbuf *rx_mbufs[BURST_SIZE];
  struct rte_mbuf *tx_mbufs[BURST_SIZE];

  while (1) {
    const uint64_t start = now();
    const uint64_t goal  = start + delta_ticks;

    uint16_t nb_rx    = 0;
    uint16_t tx_count = 0;

    do {
      nb_rx = rte_eth_rx_burst(port_in, queue, rx_mbufs, BURST_SIZE);
    } while (nb_rx == 0);

    while (__builtin_expect((now() < goal), 0)) {
      // prevent the compiler from removing this loop
      __asm__ __volatile__("");
    }

    for (uint16_t n = 0; n < nb_rx; n++) {
      LOG_DEBUG();
      LOG_DEBUG("Grabbing packet %u/%u.", n + 1, nb_rx);

      if (!check_pkt(rx_mbufs[n])) {
        LOG_DEBUG("Not a NetCache packet, dropping!");
        rte_pktmbuf_free(rx_mbufs[n]);
        continue;
      }

      process_netcache_query(rx_mbufs[n]);

      tx_mbufs[tx_count] = rx_mbufs[n];
      tx_count++;
    }

    uint16_t nb_tx = rte_eth_tx_burst(port_out, queue, tx_mbufs, tx_count);
    for (uint16_t n = nb_tx; n < tx_count; n++) {
      rte_pktmbuf_free(tx_mbufs[n]);
    }
  }
}

bool Store::check_pkt(rte_mbuf *mbuf) {
  constexpr const uint32_t min_size = sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr) + sizeof(netcache_hdr_t);

  if (mbuf->pkt_len < min_size) {
    LOG_DEBUG("Too small for a netcache packet (%u).", mbuf->pkt_len);
    return false;
  }

  rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbuf, rte_ether_hdr *);

  if (rte_be_to_cpu_16(eth_hdr->ether_type) != RTE_ETHER_TYPE_IPV4) {
    LOG_DEBUG("Not IPv4 packet.");
    return false;
  }

  rte_ipv4_hdr *ipv4_hdr = (rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(rte_ether_hdr));

  if (ipv4_hdr->next_proto_id != IPPROTO_UDP) {
    LOG_DEBUG("Not a UDP packet.");
    return false;
  }

  rte_udp_hdr *udp_hdr = (rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(rte_ipv4_hdr));
  if (rte_be_to_cpu_16(udp_hdr->dst_port) != KVSTORE_PORT) {
    LOG_DEBUG("KVS port mismatch (%u).", rte_be_to_cpu_16(udp_hdr->dst_port));
    return false;
  }

  return true;
}

void Store::process_netcache_query(rte_mbuf *mbuf) {
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
  ip_hdr->hdr_checksum = ~rte_raw_cksum(ip_hdr, sizeof(rte_ipv4_hdr));

  rte_udp_hdr *udp_hdr = (rte_udp_hdr *)pkt_ptr;
  pkt_ptr += sizeof(rte_udp_hdr);

  uint16_t temp_port = udp_hdr->src_port;
  udp_hdr->src_port  = udp_hdr->dst_port;
  udp_hdr->dst_port  = temp_port;

  netcache_hdr_t *nc_hdr = (netcache_hdr_t *)pkt_ptr;

  key_t kv_key;
  std::memcpy(kv_key.data(), nc_hdr->key, KV_KEY_SIZE);

  auto it = kv_map.find(kv_key);

  if (nc_hdr->op == READ_QUERY) {
    LOG_DEBUG("Processing read query...");
    if (it == kv_map.end()) {
      nc_hdr->status = KVS_FAILURE;
    } else {
      std::memcpy(nc_hdr->val, it->second.data(), KV_VAL_SIZE);
      nc_hdr->status = KVS_SUCCESS;
    }
  } else if (nc_hdr->op == WRITE_QUERY) {
    LOG_DEBUG("Processing write query...");
    if (it == kv_map.end()) {
      value_t kv_value;
      std::memcpy(kv_value.data(), nc_hdr->val, KV_VAL_SIZE);
      kv_map.emplace(kv_key, std::move(kv_value));
    } else {
      std::memcpy(it->second.data(), nc_hdr->val, KV_VAL_SIZE);
    }
    nc_hdr->status = KVS_SUCCESS;
  }
}

} // namespace netcache
