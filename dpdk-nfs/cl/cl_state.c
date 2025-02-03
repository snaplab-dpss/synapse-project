#include "cl_state.h"

#include <stdlib.h>
#include <assert.h>

#include "lib/util/boilerplate.h"
#ifdef KLEE_VERIFICATION
#include "lib/models/state/double-chain-control.h"
#include "lib/models/util/ether.h"
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/cms-control.h"
#include "lib/models/state/lpm-dir-24-8-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state(uint32_t max_flows, uint16_t max_clients, uint32_t sketch_height, uint32_t sketch_width,
                          time_ns_t sketch_cleanup_interval, uint32_t dev_count) {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));

  if (ret == NULL)
    return NULL;

  ret->max_flows   = max_flows;
  ret->max_clients = max_clients;
  ret->dev_count   = dev_count;

  ret->flows = NULL;
  if (map_allocate(max_flows, sizeof(struct flow), &(ret->flows)) == 0) {
    return NULL;
  }

  ret->flows_keys = NULL;
  if (vector_allocate(sizeof(struct flow), max_flows, &(ret->flows_keys)) == 0) {
    return NULL;
  }

  ret->flow_allocator = NULL;
  if (dchain_allocate(max_flows, &(ret->flow_allocator)) == 0) {
    return NULL;
  }

  ret->cms = NULL;
  if (cms_allocate(sketch_height, sketch_width, sizeof(struct client), sketch_cleanup_interval, &(ret->cms)) == 0) {
    return NULL;
  }

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->flows, flow_descrs, sizeof(flow_descrs) / sizeof(flow_descrs[0]), flow_nests,
                 sizeof(flow_nests) / sizeof(flow_nests[0]), "flow");
  vector_set_layout(ret->flows_keys, flow_descrs, sizeof(flow_descrs) / sizeof(flow_descrs[0]), flow_nests,
                    sizeof(flow_nests) / sizeof(flow_nests[0]), "flow");
  cms_set_layout(ret->cms, client_descrs, sizeof(client_descrs) / sizeof(client_descrs[0]), client_nests,
                 sizeof(client_nests) / sizeof(client_nests[0]), "client");
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->flows, &allocated_nf_state->flows_keys, &allocated_nf_state->flow_allocator,
                        &allocated_nf_state->cms, allocated_nf_state->max_flows, allocated_nf_state->dev_count, lcore_id, time);
}

#endif // KLEE_VERIFICATION
