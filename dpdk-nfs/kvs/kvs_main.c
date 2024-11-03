#include <stdlib.h>

#include "entry.h"
#include "kvs_config.h"
#include "kvs_hdr.h"
#include "kvs_manager.h"
#include "nf-log.h"
#include "nf-util.h"
#include "nf.h"

struct nf_config config;

struct State *kvs_state;

bool nf_init(void) {
  kvs_state = alloc_state(config.capacity, config.sketch_height,
                          config.sketch_width, config.sketch_cleanup_interval);
  return kvs_state != NULL;
}

void invert_flow(struct rte_ether_hdr *ether_hdr, struct rte_ipv4_hdr *ipv4_hdr,
                 struct rte_udp_hdr *udp_hdr) {
  struct rte_ether_addr tmp = ether_hdr->s_addr;
  ether_hdr->s_addr = ether_hdr->d_addr;
  ether_hdr->d_addr = tmp;

  uint32_t tmp_ip = ipv4_hdr->src_addr;
  ipv4_hdr->src_addr = ipv4_hdr->dst_addr;
  ipv4_hdr->dst_addr = tmp_ip;

  uint16_t tmp_port = udp_hdr->src_port;
  udp_hdr->src_port = udp_hdr->dst_port;
  udp_hdr->dst_port = tmp_port;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length,
               time_ns_t now, struct rte_mbuf *mbuf) {
  cms_cleanup(kvs_state->not_cached_counters, now);

  struct rte_ether_hdr *ether_hdr = nf_then_get_ether_header(buffer);

  struct rte_ipv4_hdr *ipv4_hdr = nf_then_get_ipv4_header(ether_hdr, buffer);
  if (ipv4_hdr == NULL) {
    return DROP;
  }

  struct rte_udp_hdr *udp_hdr = nf_then_get_udp_header(ipv4_hdr, buffer);
  if (udp_hdr == NULL) {
    return DROP;
  }

  struct kvs_hdr *kvs_hdr = nf_then_get_kvs_header(udp_hdr, buffer);
  if (kvs_hdr == NULL) {
    return DROP;
  }

  if (device == config.internal_device) {
    return config.external_device;
  }

  int cache_index;
  bool cache_hit = kvs_cache_lookup(kvs_state, kvs_hdr->op, kvs_hdr->key,
                                    kvs_hdr->value, &cache_index);
  kvs_update_stats(kvs_state, cache_hit, cache_index, kvs_hdr->key);

  if (cache_hit) {
    kvs_hdr->status = KVS_STATUS_CACHE_HIT;
    invert_flow(ether_hdr, ipv4_hdr, udp_hdr);
    return config.external_device;
  }

  if (kvs_hdr->op == KVS_OP_PUT) {
    uint64_t min_estimate =
        cms_count_min(kvs_state->not_cached_counters, kvs_hdr->key);

    if (min_estimate > kvs_state->hh_threshold) {
      kvs_update_cache(kvs_state, kvs_hdr->key, kvs_hdr->value, min_estimate);
    }
  }

  // Cache misses should send the packet to the storage servers (internal
  // device), but we simplify for evaluation purposes.
  kvs_hdr->status = KVS_STATUS_CACHE_MISS;
  invert_flow(ether_hdr, ipv4_hdr, udp_hdr);
  return config.external_device;
}
