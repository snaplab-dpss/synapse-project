#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/vector-control.h"

void loop_reset(struct Vector **estimators, struct Vector **accumulator, struct Vector **nonzero_count, unsigned int lcore_id, time_ns_t *time) {
  vector_reset(*estimators);
  vector_reset(*accumulator);
  vector_reset(*nonzero_count);
  *time = restart_time();
}

void loop_invariant_consume(struct Vector **estimators, struct Vector **accumulator, struct Vector **nonzero_count, unsigned int lcore_id,
                            time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(estimators, sizeof(struct Vector *), "estimators");
  klee_trace_param_ptr(accumulator, sizeof(struct Vector *), "accumulator");
  klee_trace_param_ptr(nonzero_count, sizeof(struct Vector *), "nonzero_count");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Vector **estimators, struct Vector **accumulator, struct Vector **nonzero_count, unsigned int *lcore_id,
                            time_ns_t *time) {
  klee_trace_ret();
  klee_trace_param_ptr(estimators, sizeof(struct Vector *), "estimators");
  klee_trace_param_ptr(accumulator, sizeof(struct Vector *), "accumulator");
  klee_trace_param_ptr(nonzero_count, sizeof(struct Vector *), "nonzero_count");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Vector **estimators, struct Vector **accumulator, struct Vector **nonzero_count, unsigned int lcore_id,
                           time_ns_t time) {
  loop_invariant_consume(estimators, accumulator, nonzero_count, lcore_id, time);
  loop_reset(estimators, accumulator, nonzero_count, lcore_id, &time);
  loop_invariant_produce(estimators, accumulator, nonzero_count, &lcore_id, &time);
}
#endif // KLEE_VERIFICATION
