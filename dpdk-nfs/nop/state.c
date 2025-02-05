#include "state.h"
#include "nf.h"
#include "lib/util/boilerplate.h"

#include <stdlib.h>
#include <rte_ethdev.h>

#ifdef KLEE_VERIFICATION
#include "lib/models/state/lpm-dir-24-8-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

bool port_validity(uint32_t prefix, int value) {
  uint16_t dev_count = rte_eth_dev_count_avail();
  return (value >= 0) AND(value < dev_count) AND(value != DROP) AND(value != FLOOD);
}

struct State *alloc_state() {
  struct State *ret = malloc(sizeof(struct State));

  if (!lpm_allocate(&ret->fwd)) {
    return NULL;
  }

#ifdef KLEE_VERIFICATION
  lpm_set_entry_condition(ret->fwd, port_validity);
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;

  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) { loop_iteration_border(lcore_id, time); }
#endif // KLEE_VERIFICATION
