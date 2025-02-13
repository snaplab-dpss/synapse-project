#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/vector-control.h"

void loop_reset(struct Vector **fwd_rules, unsigned int lcore_id, time_ns_t *time) {
  vector_reset(*fwd_rules);
  *time = restart_time();
}

void loop_invariant_consume(struct Vector **fwd_rules, unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Vector **fwd_rules, unsigned int *lcore_id, time_ns_t *time) {
  klee_trace_ret();
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Vector **fwd_rules, unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(fwd_rules, lcore_id, time);
  loop_reset(fwd_rules, lcore_id, &time);
  loop_invariant_produce(fwd_rules, &lcore_id, &time);
}

#endif // KLEE_VERIFICATION