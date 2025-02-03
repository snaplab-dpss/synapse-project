#ifndef _CLIENT_H_INCLUDED_
#define _CLIENT_H_INCLUDED_

#include <stdbool.h>

#include "lib/util/boilerplate.h"
#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"
#include "lib/util/time.h"
#include "lib/state/cms.h"

#include <stdint.h>

struct client {
  uint32_t src_ip;
  uint32_t dst_ip;
} PACKED_FOR_KLEE_VERIFICATION;

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"
extern struct str_field_descr client_descrs[2];
extern struct nested_field_descr client_nests[0];
#endif // KLEE_VERIFICATION

#endif //_CLIENT_H_INCLUDED_
