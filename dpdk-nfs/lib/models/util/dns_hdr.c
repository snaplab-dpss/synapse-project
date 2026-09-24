#include "lib/util/dns_hdr.h"

#include <klee/klee.h>

// Hides the walk over a message's variable-length insides.
//
// Done in the open, that walk decides how many bytes to read from bytes it has just read, so
// symbolic execution forks at every step that could end the name or the record chain -- and since
// paths never rejoin, everything done afterwards is copied into each arm. Measured on meta4: the
// label walk alone emitted the whole session-learning path five times, and the record walk doubled
// that again. Modeled, the walk is one node with a symbolic result, and the NF's own logic appears
// once.

int dns_get_response(const struct dns_hdr *msg, uint16_t length, struct dns_name *name, uint32_t *address) {
  klee_trace_ret();
  klee_trace_param_u64((uint64_t)msg, "msg");
  klee_trace_param_u16(length, "length");
  klee_trace_param_ptr(name, sizeof(struct dns_name), "name");
  klee_trace_param_ptr(address, sizeof(uint32_t), "address");

  klee_make_symbolic(name, sizeof(struct dns_name), "dns_name");
  *address = klee_int("dns_address");

  return klee_int("dns_response_found");
}
