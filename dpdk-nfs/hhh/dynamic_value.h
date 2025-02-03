#ifndef _DynamicValue_GEN_H_INCLUDED_
#define _DynamicValue_GEN_H_INCLUDED_

#include <stdbool.h>

#include "lib/util/boilerplate.h"
#include "lib/util/ether.h"
#include "lib/util/time.h"

struct DynamicValue {
  uint64_t bucket_size;
  time_ns_t bucket_time;
} PACKED_FOR_KLEE_VERIFICATION;

#define LOG_DYNAMICVALUE(obj, p)                                                                                                           \
  ;                                                                                                                                        \
  p("{");                                                                                                                                  \
  p("bucket_size: %d", obj->bucket_size);                                                                                                  \
  p("bucket_time: %d", obj->bucket_time);                                                                                                  \
  p("}");

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr DynamicValue_descrs[2];
extern struct nested_field_descr DynamicValue_nests[0];
#endif // KLEE_VERIFICATION

#endif //_DynamicValue_GEN_H_INCLUDED_
