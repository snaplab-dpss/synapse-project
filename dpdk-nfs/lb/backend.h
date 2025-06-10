#ifndef _LB_BACKEND_H_INCLUDED_
#define _LB_BACKEND_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include <rte_ether.h>

#include "lib/util/boilerplate.h"
#include "lib/util/ether.h"

struct Backend {
  struct rte_ether_addr mac;
  uint32_t ip;
};

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr Backend_descrs[2];
extern struct nested_field_descr Backend_nests[1];
#endif // KLEE_VERIFICATION

#endif //_LB_BACKEND_H_INCLUDED_
