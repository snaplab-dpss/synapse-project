#ifndef _KVSTORE_HDR_H_INCLUDED_
#define _KVSTORE_HDR_H_INCLUDED_

#define KVSTORE_PORT 670

#include "entry.h"
#include "../nf-util.h"

enum kvs_op {
  KVS_OP_GET = 0,
  KVS_OP_PUT = 1,
  KVS_OP_DEL = 2,
};

enum kvs_status {
  KVS_STATUS_CACHE_HIT  = 0,
  KVS_STATUS_CACHE_MISS = 1,
};

struct kvs_hdr {
  uint8_t op;
  kv_key_t key;
  kv_value_t value;
  uint8_t status;
} __attribute__((__packed__));

#ifdef KLEE_VERIFICATION
static struct str_field_descr kvs_hdr_fields[] = {
    {offsetof(struct kvs_hdr, op), sizeof(uint8_t), 0, "op"},
    {offsetof(struct kvs_hdr, key), sizeof(uint8_t), KEY_SIZE_BYTES, "key"},
    {offsetof(struct kvs_hdr, value), sizeof(uint8_t), MAX_VALUE_SIZE_BYTES, "value"},
    {offsetof(struct kvs_hdr, status), sizeof(uint8_t), 0, "status"},
};
#endif // KLEE_VERIFICATION

static inline struct kvs_hdr *nf_then_get_kvs_header(void *udp_hdr_, uint8_t **p) {
  struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)udp_hdr_;

  uint16_t unread_len = packet_get_unread_length(p);

  if ((udp_hdr->dst_port != rte_be_to_cpu_16(KVSTORE_PORT)) | (unread_len < sizeof(struct kvs_hdr))) {
    return NULL;
  }

  CHUNK_LAYOUT(p, kvs_hdr, kvs_hdr_fields);
  struct kvs_hdr *hdr = (struct kvs_hdr *)nf_borrow_next_chunk(p, sizeof(struct kvs_hdr));

  return hdr;
}

#endif // __KVSTORE_HDR_H_INCLUDED_