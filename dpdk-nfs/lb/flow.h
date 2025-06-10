#ifndef _LB_FLOW_H_INCLUDED_
#define _LB_FLOW_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "lib/util/boilerplate.h"
#include "lib/util/ether.h"

#define DEFAULT_LOADBALANCEDFLOW FlowIdc(0, 0, 0, 0, 0)

struct FlowId {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t protocol;
} PACKED_FOR_KLEE_VERIFICATION;

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr FlowId_descrs[5];
extern struct nested_field_descr FlowId_nests[0];
#endif // KLEE_VERIFICATION

#endif //_LB_FLOW_H_INCLUDED_
