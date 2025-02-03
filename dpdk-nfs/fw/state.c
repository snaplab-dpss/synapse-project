#include "state.h"
#include <stdlib.h>

#include "lib/verified/boilerplate-util.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/verified/double-chain-control.h"
#include "lib/models/verified/ether.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
#include "lib/models/verified/lpm-dir-24-8-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state(int max_flows) {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));
  if (ret == NULL)
    return NULL;

  ret->fm = NULL;
  if (map_allocate(max_flows, sizeof(struct FlowId), &(ret->fm)) == 0)
    return NULL;

  ret->fv = NULL;
  if (vector_allocate(sizeof(struct FlowId), max_flows, &(ret->fv)) == 0)
    return NULL;

  ret->heap = NULL;
  if (dchain_allocate(max_flows, &(ret->heap)) == 0)
    return NULL;

  ret->max_flows = max_flows;

  ret->fwd_table = NULL;
  if (FwdTable_allocate(&(ret->fwd_table)) == 0)
    return NULL;

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->fm, FlowId_descrs, sizeof(FlowId_descrs) / sizeof(FlowId_descrs[0]), FlowId_nests,
                 sizeof(FlowId_nests) / sizeof(FlowId_nests[0]), "FlowId");
  vector_set_layout(ret->fv, FlowId_descrs, sizeof(FlowId_descrs) / sizeof(FlowId_descrs[0]), FlowId_nests,
                    sizeof(FlowId_nests) / sizeof(FlowId_nests[0]), "FlowId");
  FwdTable_set_layout(ret->fwd_table);
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->fm, &allocated_nf_state->fv, &allocated_nf_state->heap, allocated_nf_state->max_flows,
                        &allocated_nf_state->fwd_table, lcore_id, time);
}

#endif // KLEE_VERIFICATION
