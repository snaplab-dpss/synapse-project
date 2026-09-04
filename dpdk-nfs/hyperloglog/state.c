#include "state.h"
#include "loop.h"
#include "config.h"

#include <stdlib.h>

#include "nf.h"
#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/vector-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state() {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));

  if (ret == NULL)
    return NULL;

  ret->estimators = NULL;
  if (vector_allocate(sizeof(uint32_t), config.num_estimators, &(ret->estimators)) == 0) {
    return NULL;
  }

  ret->accumulator = NULL;
  if (vector_allocate(sizeof(uint32_t), 1, &(ret->accumulator)) == 0) {
    return NULL;
  }

  ret->nonzero_count = NULL;
  if (vector_allocate(sizeof(uint32_t), 1, &(ret->nonzero_count)) == 0) {
    return NULL;
  }

#ifdef KLEE_VERIFICATION
  vector_set_layout(ret->estimators, NULL, 0, NULL, 0, "uint32_t");
  vector_set_layout(ret->accumulator, NULL, 0, NULL, 0, "uint32_t");
  vector_set_layout(ret->nonzero_count, NULL, 0, NULL, 0, "uint32_t");
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->estimators, &allocated_nf_state->accumulator, &allocated_nf_state->nonzero_count, lcore_id, time);
}
#endif // KLEE_VERIFICATION
