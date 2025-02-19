#include "state.h"
#include "nf.h"
#include "config.h"
#include "lib/util/boilerplate.h"

#include <stdlib.h>

#ifdef KLEE_VERIFICATION
#include "lib/models/state/vector-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state() {
  struct State *ret = malloc(sizeof(struct State));

  allocated_nf_state = ret;

  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) { loop_iteration_border(lcore_id, time); }
#endif // KLEE_VERIFICATION
