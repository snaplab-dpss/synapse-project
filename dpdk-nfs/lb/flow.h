#ifndef _LB_FLOW_H_INCLUDED_
#define _LB_FLOW_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "lib/util/boilerplate.h"
#include "lib/util/ether.h"

#define DEFAULT_LOADBALANCEDFLOW LoadBalancedFlowc(0, 0, 0, 0, 0)

struct LoadBalancedFlow {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t protocol;
} PACKED_FOR_KLEE_VERIFICATION;

#define LOG_LOADBALANCEDFLOW(obj, p)                                                                                                       \
  ;                                                                                                                                        \
  p("{");                                                                                                                                  \
  p("src_ip: %d", (obj)->src_ip);                                                                                                          \
  p("dst_ip: %d", (obj)->dst_ip);                                                                                                          \
  p("src_port: %d", (obj)->src_port);                                                                                                      \
  p("dst_port: %d", (obj)->dst_port);                                                                                                      \
  p("protocol: %d", (obj)->protocol);                                                                                                      \
  p("}");

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr LoadBalancedFlow_descrs[5];
extern struct nested_field_descr LoadBalancedFlow_nests[0];
#endif // KLEE_VERIFICATION

#endif //_LB_FLOW_H_INCLUDED_
