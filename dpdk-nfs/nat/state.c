#include "state.h"
#include "flow.h"
#include "loop.h"
#include "config.h"

#include <stdlib.h>
#include <rte_ethdev.h>

#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/double-chain-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

bool port_validity(void *value, int index, void *state) {
  uint16_t dev       = *(uint16_t *)value;
  uint16_t dev_count = rte_eth_dev_count_avail();
  return (dev >= 0) AND((dev < dev_count) OR(dev == DROP));
}

bool bool_invariant(void *value, int index, void *state) { return (*(int *)value == 0) | (*(int *)value == 1); }

struct State *alloc_state() {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));
  if (ret == NULL)
    return NULL;

  ret->fm = NULL;
  if (map_allocate(config.max_flows, sizeof(struct FlowId), &(ret->fm)) == 0)
    return NULL;

  ret->fv = NULL;
  if (vector_allocate(sizeof(struct FlowId), config.max_flows, &(ret->fv)) == 0)
    return NULL;

  ret->heap = NULL;
  if (dchain_allocate(config.max_flows, &(ret->heap)) == 0)
    return NULL;

  ret->int_devices = NULL;
  if (vector_allocate(sizeof(int), rte_eth_dev_count_avail(), &(ret->int_devices)) == 0)
    return NULL;

  ret->fwd_rules = NULL;
  if (vector_allocate(sizeof(uint16_t), rte_eth_dev_count_avail(), &(ret->fwd_rules)) == 0)
    return NULL;

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->fm, FlowId_descrs, sizeof(FlowId_descrs) / sizeof(FlowId_descrs[0]), FlowId_nests,
                 sizeof(FlowId_nests) / sizeof(FlowId_nests[0]), "FlowId");
  vector_set_layout(ret->fv, FlowId_descrs, sizeof(FlowId_descrs) / sizeof(FlowId_descrs[0]), FlowId_nests,
                    sizeof(FlowId_nests) / sizeof(FlowId_nests[0]), "FlowId");
  vector_set_layout(ret->int_devices, NULL, 0, NULL, 0, "int");
  vector_set_entry_condition(ret->int_devices, bool_invariant, ret);
  vector_set_layout(ret->fwd_rules, NULL, 0, NULL, 0, "uint16_t");
  vector_set_entry_condition(ret->fwd_rules, port_validity, ret);
#endif // KLEE_VERIFICATION

  for (size_t dev = 0; dev < rte_eth_dev_count_avail(); dev++) {
    int *is_internal;
    vector_borrow(ret->int_devices, dev, (void **)&is_internal);
    *is_internal = 0;
    for (size_t i = 0; i < config.internal_devs.n; i++) {
      if (config.internal_devs.devices[i] == dev) {
        *is_internal = 1;
        break;
      }
    }
    vector_return(ret->int_devices, dev, is_internal);

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
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->fm, &allocated_nf_state->fv, &allocated_nf_state->heap, &allocated_nf_state->int_devices,
                        &allocated_nf_state->fwd_rules, config.max_flows, lcore_id, time);
}

#endif // KLEE_VERIFICATION
