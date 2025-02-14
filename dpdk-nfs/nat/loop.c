#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/double-chain-control.h"

void loop_reset(struct Map **fm, struct Vector **fv, struct DoubleChain **heap, struct Vector **int_devices, struct Vector **fwd_rules,
                int max_flows, unsigned int lcore_id, time_ns_t *time) {
  map_reset(*fm);
  vector_reset(*fv);
  dchain_reset(*heap, max_flows);
  vector_reset(*int_devices);
  vector_reset(*fwd_rules);
  *time = restart_time();
}
void loop_invariant_consume(struct Map **fm, struct Vector **fv, struct DoubleChain **heap, struct Vector **int_devices,
                            struct Vector **fwd_rules, int max_flows, unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(fm, sizeof(struct Map *), "fm");
  klee_trace_param_ptr(fv, sizeof(struct Vector *), "fv");
  klee_trace_param_ptr(heap, sizeof(struct DoubleChain *), "heap");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_i32(max_flows, "max_flows");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}
void loop_invariant_produce(struct Map **fm, struct Vector **fv, struct DoubleChain **heap, struct Vector **int_devices,
                            struct Vector **fwd_rules, int max_flows, unsigned int *lcore_id, time_ns_t *time) {
  klee_trace_ret();
  klee_trace_param_ptr(fm, sizeof(struct Map *), "fm");
  klee_trace_param_ptr(fv, sizeof(struct Vector *), "fv");
  klee_trace_param_ptr(heap, sizeof(struct DoubleChain *), "heap");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_i32(max_flows, "max_flows");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}
void loop_iteration_border(struct Map **fm, struct Vector **fv, struct DoubleChain **heap, struct Vector **int_devices,
                           struct Vector **fwd_rules, int max_flows, unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(fm, fv, heap, int_devices, fwd_rules, max_flows, lcore_id, time);
  loop_reset(fm, fv, heap, int_devices, fwd_rules, max_flows, lcore_id, &time);
  loop_invariant_produce(fm, fv, heap, int_devices, fwd_rules, max_flows, &lcore_id, &time);
}
#endif // KLEE_VERIFICATION