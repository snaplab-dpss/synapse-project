#include "state.h"
#include "nf.h"
#include "config.h"
#include "lib/util/boilerplate.h"

#include <stdlib.h>
#include <rte_ethdev.h>

#ifdef KLEE_VERIFICATION
#include "lib/models/state/vector-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

bool port_validity(void *value, int index, void *state) {
  uint16_t dev       = *(uint16_t *)value;
  uint16_t dev_count = rte_eth_dev_count_avail();
  return (dev >= 0) AND((dev < dev_count) OR(dev == DROP));
}

struct State *alloc_state() {
  struct State *ret = malloc(sizeof(struct State));

  ret->fwd_rules = NULL;
  if (vector_allocate(sizeof(uint16_t), rte_eth_dev_count_avail(), &(ret->fwd_rules)) == 0)
    return NULL;

#ifdef KLEE_VERIFICATION
  vector_set_layout(ret->fwd_rules, NULL, 0, NULL, 0, "uint16_t");
  vector_set_entry_condition(ret->fwd_rules, port_validity, ret);
#endif // KLEE_VERIFICATION

  for (size_t dev = 0; dev < rte_eth_dev_count_avail(); dev++) {
    uint16_t *dst_dev;
    vector_borrow(ret->fwd_rules, dev, (void **)&dst_dev);
    *dst_dev = DROP;
    for (size_t i = 0; i < config.fwd_rules.n; i++) {
      if (config.fwd_rules.src_dev[i] == dev) {
        *dst_dev = config.fwd_rules.dst_dev[i];
        break;
      }
    }
    vector_return(ret->fwd_rules, dev, dst_dev);
  }

  allocated_nf_state = ret;

  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) { loop_iteration_border(&allocated_nf_state->fwd_rules, lcore_id, time); }
#endif // KLEE_VERIFICATION
