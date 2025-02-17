#ifdef KLEE_VERIFICATION

#include <klee/klee.h>

#include "loop.h"
#include "lib/models/util/time-control.h"
#include "lib/models/state/token-bucket-control.h"
#include "lib/models/state/vector-control.h"

void loop_reset(struct TokenBucket **tb, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t dev_count, unsigned int lcore_id,
                time_ns_t *time) {
  tb_reset(*tb);
  vector_reset(*int_devices);
  vector_reset(*fwd_rules);
  *time = restart_time();
}

void loop_invariant_consume(struct TokenBucket **tb, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t dev_count,
                            unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(tb, sizeof(struct TokenBucket *), "tb");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_u32(dev_count, "dev_count");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct TokenBucket **tb, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t dev_count,
                            unsigned int lcore_id, time_ns_t *time) {
  klee_trace_ret();
  klee_trace_param_ptr(tb, sizeof(struct TokenBucket *), "tb");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_u32(dev_count, "dev_count");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct TokenBucket **tb, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t dev_count,
                           unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(tb, int_devices, fwd_rules, dev_count, lcore_id, time);
  loop_reset(tb, int_devices, fwd_rules, dev_count, lcore_id, &time);
  loop_invariant_produce(tb, int_devices, fwd_rules, dev_count, lcore_id, &time);
}

#endif // KLEE_VERIFICATION