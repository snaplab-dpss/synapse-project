#include "state.h"
#include "flow.h"
#include "client.h"
#include "loop.h"
#include "config.h"

#include <stdlib.h>
#include <assert.h>

#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/double-chain-control.h"
#include "lib/models/state/cms-control.h"
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

  ret->flows = NULL;
  if (map_allocate(config.max_flows, sizeof(struct flow), &(ret->flows)) == 0) {
    return NULL;
  }

  ret->flows_keys = NULL;
  if (vector_allocate(sizeof(struct flow), config.max_flows, &(ret->flows_keys)) == 0) {
    return NULL;
  }

  ret->flow_allocator = NULL;
  if (dchain_allocate(config.max_flows, &(ret->flow_allocator)) == 0) {
    return NULL;
  }

  ret->cms = NULL;
  if (cms_allocate(config.sketch_height, config.sketch_width, sizeof(struct client), config.sketch_cleanup_interval, &(ret->cms)) == 0) {
    return NULL;
  }

  ret->int_devices = NULL;
  if (vector_allocate(sizeof(int), rte_eth_dev_count_avail(), &(ret->int_devices)) == 0)
    return NULL;

  ret->fwd_rules = NULL;
  if (vector_allocate(sizeof(uint16_t), rte_eth_dev_count_avail(), &(ret->fwd_rules)) == 0)
    return NULL;

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->flows, flow_descrs, sizeof(flow_descrs) / sizeof(flow_descrs[0]), flow_nests,
                 sizeof(flow_nests) / sizeof(flow_nests[0]), "flow");
  vector_set_layout(ret->flows_keys, flow_descrs, sizeof(flow_descrs) / sizeof(flow_descrs[0]), flow_nests,
                    sizeof(flow_nests) / sizeof(flow_nests[0]), "flow");
  cms_set_layout(ret->cms, client_descrs, sizeof(client_descrs) / sizeof(client_descrs[0]), client_nests,
                 sizeof(client_nests) / sizeof(client_nests[0]), "client");
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
  loop_iteration_border(&allocated_nf_state->flows, &allocated_nf_state->flows_keys, &allocated_nf_state->flow_allocator,
                        &allocated_nf_state->cms, &allocated_nf_state->int_devices, &allocated_nf_state->fwd_rules, config.max_flows,
                        lcore_id, time);
}

#endif // KLEE_VERIFICATION
