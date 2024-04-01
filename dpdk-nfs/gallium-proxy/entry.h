#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/verified/boilerplate-util.h"

struct Entry {
  uint16_t port;
} PACKED_FOR_KLEE_VERIFICATION;

#ifdef KLEE_VERIFICATION
#include "lib/models/str-descr.h"
#include <klee/klee.h>

extern struct str_field_descr entry_descrs[1];
extern struct nested_field_descr entry_nests[0];
#endif  // KLEE_VERIFICATION