#include <stdlib.h>

#include "entry.h"
#include "config.h"
#include "kvs_hdr.h"
#include "state.h"
#include "nf-log.h"
#include "nf-util.h"
#include "nf.h"

#include "lib/util/expirator.h"

struct nf_config config;
struct State *kvs_state;

bool nf_init(void) {
  kvs_state = alloc_state();
  return kvs_state != NULL;
}

void invert_flow(struct rte_ether_hdr *ether_hdr, struct rte_ipv4_hdr *ipv4_hdr, struct rte_udp_hdr *udp_hdr) {
  struct rte_ether_addr tmp = ether_hdr->src_addr;
  ether_hdr->src_addr       = ether_hdr->dst_addr;
  ether_hdr->dst_addr       = tmp;

  uint32_t tmp_ip    = ipv4_hdr->src_addr;
  ipv4_hdr->src_addr = ipv4_hdr->dst_addr;
  ipv4_hdr->dst_addr = tmp_ip;

  uint16_t tmp_port = udp_hdr->src_port;
  udp_hdr->src_port = udp_hdr->dst_port;
  udp_hdr->dst_port = tmp_port;
}

void kvs_expire(time_ns_t now) {
  assert(now >= 0); // we don't support the past
  assert(sizeof(time_ns_t) <= sizeof(uint64_t));
  uint64_t time_u     = (uint64_t)now; // OK because of the two asserts
  time_ns_t last_time = time_u - kvs_state->expiration_time;
  expire_items_single_map(kvs_state->heap, kvs_state->keys, kvs_state->kvs, last_time);
}

bool kvs_cache_lookup(struct State *state, time_ns_t now, enum kvs_op op, kv_key_t key, kv_value_t value, int *index) {
  if (map_get(state->kvs, key, index) == 0) {
    return false;
  }

  NF_DEBUG("Cache hit");

  dchain_rejuvenate_index(state->heap, *index, now);

  switch (op) {
  case KVS_OP_GET: {
    void *curr_value;
    vector_borrow(state->values, *index, (void **)&curr_value);
    memcpy(value, curr_value, sizeof(kv_value_t));
    vector_return(state->values, *index, curr_value);
  } break;
  case KVS_OP_PUT: {
    void *curr_value;
    vector_borrow(state->values, *index, (void **)&curr_value);
    memcpy(curr_value, value, sizeof(kv_value_t));
    vector_return(state->values, *index, curr_value);
  } break;
  case KVS_OP_DEL: {
    void *trash;
    map_erase(state->kvs, key, &trash);
    dchain_free_index(state->heap, *index);
  } break;
  }

  return true;
}

bool kvs_on_cache_miss(struct State *state, time_ns_t now, enum kvs_op op, kv_key_t key, kv_value_t value) {
  // Only update the cache on PUT operations.
  if (op != KVS_OP_PUT) {
    return false;
  }

  int index;
  if (!dchain_allocate_new_index(state->heap, &index, now)) {
    // Not enough space in the cache.
    return false;
  }

  void *k;
  vector_borrow(state->keys, index, (void **)&k);
  memcpy(k, key, sizeof(kv_key_t));
  map_put(state->kvs, k, index);
  vector_return(state->keys, index, k);

  void *v;
  vector_borrow(state->values, index, (void **)&v);
  memcpy(v, value, sizeof(kv_value_t));
  vector_return(state->values, index, v);

  return true;
}

int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length, time_ns_t now, struct rte_mbuf *mbuf) {
  kvs_expire(now);

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

  if (device == config.server_dev) {
    return kvs_hdr->client_port;
  }

  int index;
  bool cache_hit = kvs_cache_lookup(kvs_state, now, kvs_hdr->op, kvs_hdr->key, kvs_hdr->value, &index);

  if (cache_hit) {
    invert_flow(ether_hdr, ipv4_hdr, udp_hdr);
    return device;
  }

  bool cache_updated = kvs_on_cache_miss(kvs_state, now, kvs_hdr->op, kvs_hdr->key, kvs_hdr->value);

  if (cache_updated) {
    invert_flow(ether_hdr, ipv4_hdr, udp_hdr);
    return device;
  }

  // Cache miss and not updated, send to storage server.
  kvs_hdr->client_port = device;
  return config.server_dev;
}
