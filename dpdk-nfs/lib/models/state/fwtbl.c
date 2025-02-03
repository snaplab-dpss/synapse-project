#include "lib/state/fwtbl.h"

#include "fwtbl-control.h"

#include <klee/klee.h>
#include <stdlib.h>
#include <string.h>

#include "../state/map-control.h"
#include "../state/vector-control.h"

#define PREALLOC_SIZE (256)

struct ForwardingTable {
  uint32_t capacity;
};

void fwtbl_set_layout(struct ForwardingTable *fwtbl) {}

void fwtbl_reset(struct ForwardingTable *fwtbl) {}

int fwtbl_allocate(uint16_t max_devices, struct ForwardingTable **fwtbl_out) {
  klee_trace_ret();

  klee_trace_param_u32(max_devices, "max_devices");
  klee_trace_param_ptr(fwtbl_out, sizeof(struct ForwardingTable *), "fwtbl_out");

  int allocation_succeeded = klee_int("fwtbl_allocation_succeeded");

  if (allocation_succeeded) {
    *fwtbl_out = malloc(sizeof(struct ForwardingTable));
    klee_make_symbolic((*fwtbl_out), sizeof(struct ForwardingTable), "fwtbl");
    klee_assert((*fwtbl_out) != NULL);
    (*fwtbl_out)->capacity = max_devices;
    return 1;
  }

  return 0;
}

void fwtbl_fill(struct ForwardingTable *fwtbl, const char *cfg_fname) {
  // Do not trace, let's not care about this.
}

int fwtbl_lookup(struct ForwardingTable *ft, uint16_t src_dev, uint16_t *dst_dev, bool *is_internal, struct rte_ether_addr *dst_addr) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)ft, "ft");
  klee_trace_param_i32(src_dev, "src_dev");

  int forwarding_table_hit = klee_int("forwarding_table_hit");
  if (forwarding_table_hit) {
    *dst_dev     = klee_int("dst_dev");
    *is_internal = klee_int("is_internal");
    klee_make_symbolic(dst_addr, sizeof(struct rte_ether_addr), "dst_mac_addr");
  }

  return forwarding_table_hit;
}
