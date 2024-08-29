#include <stdlib.h>

#include "entry.h"
#include "kvstore_config.h"
#include "kvstore_hdr.h"
#include "kvstore_manager.h"
#include "nf-log.h"
#include "nf-util.h"
#include "nf.h"

struct nf_config config;

struct State *kvstore_state;

bool nf_init(void) {
  kvstore_state = state_allocate(config.max_entries);
  return kvstore_state != NULL;
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
  NF_DEBUG("It is %" PRId64, now);

  struct rte_ether_hdr *ether_hdr = nf_then_get_ether_header(buffer);
  struct rte_ipv4_hdr *ipv4_hdr = nf_then_get_ipv4_header(ether_hdr, buffer);

  if (ipv4_hdr == NULL) {
    NF_DEBUG("Not IPv4, dropping");
    return DROP;
  }

  struct rte_udp_hdr *udp_hdr = nf_then_get_udp_header(ipv4_hdr, buffer);

  if (udp_hdr == NULL) {
    NF_DEBUG("Not UDP, dropping");
    return DROP;
  }

  struct kvstore_hdr *kvstore_hdr = nf_then_get_kvstore_header(udp_hdr, buffer);

  if (kvstore_hdr == NULL) {
    NF_DEBUG("Not KVS, dropping");
    return DROP;
  }

  if (device == config.internal_device) {
    return config.external_device;
  }

  enum kvstore_op op = (enum kvstore_op)kvstore_hdr->op;

  bool status = true;
  uint16_t dst_device = device;

  switch (kvstore_hdr->op) {
  case KVSTORE_OP_GET: {
    status = kvstore_get(kvstore_state, kvstore_hdr->key, kvstore_hdr->value);
    if (!status) {
      NF_DEBUG("Failed to get key");
    }
  } break;
  case KVSTORE_OP_PUT: {
    status =
        kvstore_put(kvstore_state, kvstore_hdr->key, kvstore_hdr->value, now);
    if (!status) {
      NF_DEBUG("Failed to put key");
    }
    dst_device = config.internal_device;
  } break;
  case KVSTORE_OP_DEL: {
    status = kvstore_delete(kvstore_state, kvstore_hdr->key);
    if (!status) {
      NF_DEBUG("Failed to delete key");
    }
    dst_device = config.internal_device;
  } break;
  default: {
    NF_DEBUG("Unknown operation");
    status = false;
  }
  }

  if (status) {
    kvstore_hdr->status = KVSTORE_STATUS_SUCCESS;
  } else {
    kvstore_hdr->status = KVSTORE_STATUS_FAILED;
  }

  invert_flow(ether_hdr, ipv4_hdr, udp_hdr);

  return dst_device;
}
