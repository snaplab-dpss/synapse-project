#ifndef _FLOW_H_INCLUDED_
#define _FLOW_H_INCLUDED_

#include <stdint.h>

#include "lib/util/boilerplate.h"

struct FlowId {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
} PACKED_FOR_KLEE_VERIFICATION;

#define LOG_FLOWID(obj, p)                                                                                                                           \
  ;                                                                                                                                                  \
  p("{");                                                                                                                                            \
  p("src_ip: %d", (obj)->src_ip);                                                                                                                    \
  p("dst_ip: %d", (obj)->dst_ip);                                                                                                                    \
  p("src_port: %d", (obj)->src_port);                                                                                                                \
  p("dst_port: %d", (obj)->dst_port);                                                                                                                \
  p("}");

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr FlowId_descrs[4];
extern struct nested_field_descr FlowId_nests[0];
#endif // KLEE_VERIFICATION

#endif // _FLOW_H_INCLUDED_