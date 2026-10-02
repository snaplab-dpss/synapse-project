#ifndef _hp_slot_GEN_H_INCLUDED_
#define _hp_slot_GEN_H_INCLUDED_

#include <stdbool.h>
#include "lib/util/boilerplate.h"

#include <stdint.h>

// One slot of a HashPipe stage: the prefix it tracks and the packets counted for it.
struct hp_slot {
  uint32_t key;
  uint32_t count;
} PACKED_FOR_KLEE_VERIFICATION;

#define DEFAULT_hp_slot hp_slotc(0, 0)

#define LOG_HP_SLOT(obj, p)                                                                                                                          \
  ;                                                                                                                                                  \
  p("{");                                                                                                                                            \
  p("key: %u, count: %u", obj->key, obj->count);                                                                                                     \
  p("}");

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr hp_slot_descrs[2];
extern struct nested_field_descr hp_slot_nests[0];
#endif // KLEE_VERIFICATION

#endif //_hp_slot_GEN_H_INCLUDED_
