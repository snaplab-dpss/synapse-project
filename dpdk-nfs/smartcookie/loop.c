#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/bloom-filter-control.h"

void loop_reset(struct BloomFilter **verified, struct Vector **timedelta, unsigned int lcore_id, time_ns_t *time) {
  bf_reset(*verified);
  vector_reset(*timedelta);

  *time = restart_time();
}

void loop_invariant_consume(struct BloomFilter **verified, struct Vector **timedelta, unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();

  klee_trace_param_ptr(verified, sizeof(struct BloomFilter *), "verified");
  klee_trace_param_ptr(timedelta, sizeof(struct Vector *), "timedelta");

  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct BloomFilter **verified, struct Vector **timedelta, unsigned int *lcore_id, time_ns_t *time) {
  klee_trace_ret();

  klee_trace_param_ptr(verified, sizeof(struct BloomFilter *), "verified");
  klee_trace_param_ptr(timedelta, sizeof(struct Vector *), "timedelta");

  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct BloomFilter **verified, struct Vector **timedelta, unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(verified, timedelta, lcore_id, time);
  loop_reset(verified, timedelta, lcore_id, &time);
  loop_invariant_produce(verified, timedelta, &lcore_id, &time);
}
#endif // KLEE_VERIFICATION
