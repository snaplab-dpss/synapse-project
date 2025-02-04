#include "state.h"
#include "flow.h"
#include "config.h"
#include "loop.h"

#include <stdlib.h>
#include <rte_ethdev.h>

#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/double-chain-control.h"
#include "lib/models/state/devices-table-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

bool port_validity(void *value, int index, void *state) {
  uint16_t *internal_device = value;
  uint16_t dev_count        = rte_eth_dev_count_avail();
  return (*internal_device < dev_count) AND(*internal_device != config.wan_device) AND(*internal_device != DROP)
      AND(*internal_device != FLOOD);
}

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

  ret->int_devices = NULL;
  if (vector_allocate(sizeof(uint16_t), config.max_flows, &(ret->int_devices)) == 0)
    return NULL;

  ret->heap = NULL;
  if (dchain_allocate(config.max_flows, &(ret->heap)) == 0)
    return NULL;

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->fm, FlowId_descrs, sizeof(FlowId_descrs) / sizeof(FlowId_descrs[0]), FlowId_nests,
                 sizeof(FlowId_nests) / sizeof(FlowId_nests[0]), "FlowId");
  vector_set_layout(ret->fv, FlowId_descrs, sizeof(FlowId_descrs) / sizeof(FlowId_descrs[0]), FlowId_nests,
                    sizeof(FlowId_nests) / sizeof(FlowId_nests[0]), "FlowId");
  vector_set_layout(ret->int_devices, NULL, 0, NULL, 0, "uint16_t");
  vector_set_entry_condition(ret->int_devices, port_validity, ret);
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->fm, &allocated_nf_state->fv, &allocated_nf_state->int_devices, &allocated_nf_state->heap,
                        config.max_flows, lcore_id, time);
}

#endif // KLEE_VERIFICATION
