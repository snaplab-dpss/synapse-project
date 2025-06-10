#include "state.h"
#include "backend.h"
#include "flow.h"

#include <stdlib.h>

#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/cht-control.h"
#include "lib/models/util/ether.h"
#include "lib/models/state/vector-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

bool lb_backend_id_condition(void *value, int index, void *state) {
  struct ip_addr *v = value;
  return (0 <= index) AND(index < 32);
}

bool lb_flow_id_condition(void *value, int index, void *state) {
  struct FlowId *v = value;
  return (0 <= index) AND(index < 65536);
}

bool lb_flow_id2backend_id_cond(void *value, int index, void *state) {
  uint32_t v = *(uint32_t *)value;
  return (v < 32);
}

struct State *alloc_state(uint32_t max_backends, uint32_t cht_height) {
  if (allocated_nf_state != NULL) {
    return allocated_nf_state;
  }

  struct State *ret = malloc(sizeof(struct State));
  if (ret == NULL)
    return NULL;

  ret->flow_to_backend_id = NULL;
  if (cht_allocate(cht_height, sizeof(struct FlowId), &(ret->flow_to_backend_id)) == 0)
    return NULL;

  ret->backend_to_backend_id = NULL;
  if (map_allocate(max_backends, sizeof(struct Backend), &(ret->backend_to_backend_id)) == 0)
    return NULL;

  ret->backends_heap = NULL;
  if (dchain_allocate(max_backends, &(ret->backends_heap)) == 0)
    return NULL;

  ret->backends = NULL;
  if (vector_allocate(sizeof(struct Backend), max_backends, &(ret->backends)) == 0)
    return NULL;

  ret->max_backends = max_backends;

#ifdef KLEE_VERIFICATION
  cht_set_layout(ret->flow_to_backend_id, FlowId_descrs, sizeof(FlowId_descrs) / sizeof(FlowId_descrs[0]), FlowId_nests,
                 sizeof(FlowId_nests) / sizeof(FlowId_nests[0]), "FlowId");
  map_set_layout(ret->backend_to_backend_id, Backend_descrs, sizeof(Backend_descrs) / sizeof(Backend_descrs[0]), Backend_nests,
                 sizeof(Backend_nests) / sizeof(Backend_nests[0]), "Backend");
  vector_set_layout(ret->backends, Backend_descrs, sizeof(Backend_descrs) / sizeof(Backend_descrs[0]), Backend_nests,
                    sizeof(Backend_nests) / sizeof(Backend_nests[0]), "Backend");
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->flow_to_backend_id, &allocated_nf_state->backend_to_backend_id, &allocated_nf_state->backends_heap,
                        &allocated_nf_state->backends, allocated_nf_state->max_backends, lcore_id, time);
}

#endif // KLEE_VERIFICATION
