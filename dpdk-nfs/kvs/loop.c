#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/verified/vigor-time-control.h"
#include "lib/models/verified/double-chain-control.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"

void loop_reset(struct Map** kvstore, struct Vector** keys,
                struct Vector** values, struct DoubleChain** heap,
                int max_entries, unsigned int lcore_id, time_ns_t* time) {
  map_reset(*kvstore);
  vector_reset(*keys);
  vector_reset(*values);
  dchain_reset(*heap, max_entries);
  *time = restart_time();
}

void loop_invariant_consume(struct Map** kvstore, struct Vector** keys,
                            struct Vector** values, struct DoubleChain** heap,
                            int max_entries, unsigned int lcore_id,
                            time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(kvstore, sizeof(struct Map*), "kvstore");
  klee_trace_param_ptr(keys, sizeof(struct Vector*), "keys");
  klee_trace_param_ptr(values, sizeof(struct Vector*), "values");
  klee_trace_param_ptr(heap, sizeof(struct DoubleChain*), "heap");
  klee_trace_param_i32(max_entries, "max_entries");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Map** kvstore, struct Vector** keys,
                            struct Vector** values, struct DoubleChain** heap,
                            int max_entries, unsigned int* lcore_id,
                            time_ns_t* time) {
  klee_trace_ret();
  klee_trace_param_ptr(kvstore, sizeof(struct Map*), "kvstore");
  klee_trace_param_ptr(keys, sizeof(struct Vector*), "keys");
  klee_trace_param_ptr(values, sizeof(struct Vector*), "values");
  klee_trace_param_ptr(heap, sizeof(struct DoubleChain*), "heap");
  klee_trace_param_i32(max_entries, "max_entries");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Map** kvstore, struct Vector** keys,
                           struct Vector** values, struct DoubleChain** heap,
                           int max_entries, unsigned int lcore_id,
                           time_ns_t time) {
  loop_invariant_consume(kvstore, keys, values, heap, max_entries, lcore_id,
                         time);
  loop_reset(kvstore, keys, values, heap, max_entries, lcore_id, &time);
  loop_invariant_produce(kvstore, keys, values, heap, max_entries, &lcore_id,
                         &time);
}
#endif  // KLEE_VERIFICATION