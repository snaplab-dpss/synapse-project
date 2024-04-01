#ifndef _COUNTER_GEN_H_INCLUDED_
#define _COUNTER_GEN_H_INCLUDED_

#include <stdbool.h>
#include "lib/verified/boilerplate-util.h"

#include "lib/verified/ether.h"

#include <stdint.h>

struct counter {
  uint32_t value;
} PACKED_FOR_KLEE_VERIFICATION;

#define LOG_counter(obj, p)   \
  ;                           \
  p("{");                     \
  p("value: %d", obj->value); \
  p("}");

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr counter_descrs[1];
extern struct nested_field_descr counter_nests[0];
#endif  // KLEE_VERIFICATION

#endif  //_COUNTER_GEN_H_INCLUDED_
