#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/verified/boilerplate-util.h"

struct Backend {
  uint32_t ip;
};

#ifdef KLEE_VERIFICATION
#include "lib/models/str-descr.h"
#include <klee/klee.h>

extern struct str_field_descr backend_descrs[1];
extern struct nested_field_descr backend_nests[0];
#endif  // KLEE_VERIFICATION