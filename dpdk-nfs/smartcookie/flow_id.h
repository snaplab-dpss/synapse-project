#ifndef _FLOW_ID_H_INCLUDED_
#define _FLOW_ID_H_INCLUDED_

#include <stdint.h>

#include "lib/util/boilerplate.h"

// Client -> server 4-tuple, the bloom filter key for verified connections.
struct FlowId {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
} PACKED_FOR_KLEE_VERIFICATION;

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr flow_id_descrs[4];
extern struct nested_field_descr flow_id_nests[0];
#endif // KLEE_VERIFICATION

#endif //_FLOW_ID_H_INCLUDED_
