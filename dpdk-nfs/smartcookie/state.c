#include "state.h"
#include "flow_id.h"
#include "loop.h"
#include "config.h"

#include <stdlib.h>

#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/vector-control.h"
#include "lib/models/state/bloom-filter-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state() {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));

  if (ret == NULL)
    return NULL;

  // The switch agent never clears its bloom filter, so no periodic cleanup is ever requested.
  ret->verified = NULL;
  if (bf_allocate(config.bloom_filter_height, config.bloom_filter_width, sizeof(struct FlowId), 0, &(ret->verified)) == 0) {
    return NULL;
  }

  ret->timedelta = NULL;
  if (vector_allocate(sizeof(uint32_t), 1, &(ret->timedelta)) == 0) {
    return NULL;
  }

#ifdef KLEE_VERIFICATION
  bf_set_layout(ret->verified, flow_id_descrs, sizeof(flow_id_descrs) / sizeof(flow_id_descrs[0]), flow_id_nests,
                sizeof(flow_id_nests) / sizeof(flow_id_nests[0]), "FlowId");
  vector_set_layout(ret->timedelta, NULL, 0, NULL, 0, "uint32_t");
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->verified, &allocated_nf_state->timedelta, lcore_id, time);
}
#endif // KLEE_VERIFICATION
