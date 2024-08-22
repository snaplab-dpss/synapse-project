#ifndef __KVSTORE_HDR_H_INCLUDED_
#define __KVSTORE_HDR_H_INCLUDED_

#define KVSTORE_PORT 670

#include "entry.h"
#include "../nf-util.h"

enum kvstore_op {
  KVSTORE_OP_GET = 0,
  KVSTORE_OP_PUT = 1,
  KVSTORE_OP_DEL = 2,
};

enum kvstore_status {
  KVSTORE_STATUS_SUCCESS = 0,
  KVSTORE_STATUS_FAILED = 1,
};

struct kvstore_hdr {
  uint8_t op;
  kv_key_t key;
  kv_value_t value;
  uint8_t status;
} __attribute__((__packed__));

#ifdef KLEE_VERIFICATION
static struct str_field_descr kvstore_hdr_fields[] = {
    {offsetof(struct kvstore_hdr, op), sizeof(uint8_t), 0, "op"},
    {offsetof(struct kvstore_hdr, key), sizeof(uint8_t), KEY_SIZE_BYTES, "key"},
    {offsetof(struct kvstore_hdr, value), sizeof(uint8_t), MAX_VALUE_SIZE_BYTES,
     "value"},
    {offsetof(struct kvstore_hdr, status), sizeof(uint8_t), 0, "status"},
};
#endif  // KLEE_VERIFICATION

static inline struct kvstore_hdr *nf_then_get_kvstore_header(void *udp_hdr_,
                                                             uint8_t **p) {
  struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)udp_hdr_;

  uint16_t unread_len = packet_get_unread_length(p);

  if ((udp_hdr->dst_port != rte_be_to_cpu_16(KVSTORE_PORT)) |
      (unread_len < sizeof(struct kvstore_hdr))) {
    return NULL;
  }

  CHUNK_LAYOUT(p, kvstore_hdr, kvstore_hdr_fields);
  struct kvstore_hdr *hdr =
      (struct kvstore_hdr *)nf_borrow_next_chunk(p, sizeof(struct kvstore_hdr));

  return hdr;
}

#endif  // __KVSTORE_HDR_H_INCLUDED_