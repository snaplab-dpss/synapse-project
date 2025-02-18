#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/double-chain-control.h"
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"

void loop_reset(struct Map **srcs, struct Vector **srcs_keys, struct Vector **touched_ports_counter, struct DoubleChain **allocator,
                struct Map **ports, struct Vector **ports_key, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t capacity,
                unsigned int lcore_id, time_ns_t *time) {
  map_reset(*srcs);
  vector_reset(*srcs_keys);
  vector_reset(*touched_ports_counter);
  dchain_reset(*allocator, capacity);
  map_reset(*ports);
  vector_reset(*ports_key);
  vector_reset(*int_devices);
  vector_reset(*fwd_rules);

  *time = restart_time();
}

void loop_invariant_consume(struct Map **srcs, struct Vector **srcs_keys, struct Vector **touched_ports_counter,
                            struct DoubleChain **allocator, struct Map **ports, struct Vector **ports_key, struct Vector **int_devices,
                            struct Vector **fwd_rules, uint32_t capacity, unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();

  klee_trace_param_ptr(srcs, sizeof(struct Map *), "srcs");
  klee_trace_param_ptr(srcs_keys, sizeof(struct Vector *), "srcs_keys");
  klee_trace_param_ptr(touched_ports_counter, sizeof(struct Vector *), "touched_ports_counter");
  klee_trace_param_ptr(allocator, sizeof(struct DoubleChain *), "allocator");
  klee_trace_param_ptr(ports, sizeof(struct Vector *), "ports");
  klee_trace_param_ptr(ports_key, sizeof(struct Vector *), "ports_key");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");

  klee_trace_param_u32(capacity, "capacity");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Map **srcs, struct Vector **srcs_keys, struct Vector **touched_ports_counter,
                            struct DoubleChain **allocator, struct Map **ports, struct Vector **ports_key, struct Vector **int_devices,
                            struct Vector **fwd_rules, uint32_t capacity, unsigned int *lcore_id, time_ns_t *time) {
  klee_trace_ret();

  klee_trace_param_ptr(srcs, sizeof(struct Map *), "srcs");
  klee_trace_param_ptr(srcs_keys, sizeof(struct Vector *), "srcs_keys");
  klee_trace_param_ptr(touched_ports_counter, sizeof(struct Vector *), "touched_ports_counter");
  klee_trace_param_ptr(allocator, sizeof(struct DoubleChain *), "allocator");
  klee_trace_param_ptr(ports, sizeof(struct Vector *), "ports");
  klee_trace_param_ptr(ports_key, sizeof(struct Vector *), "ports_key");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");

  klee_trace_param_u32(capacity, "capacity");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Map **srcs, struct Vector **srcs_keys, struct Vector **touched_ports_counter,
                           struct DoubleChain **allocator, struct Map **ports, struct Vector **ports_key, struct Vector **int_devices,
                           struct Vector **fwd_rules, uint32_t capacity, unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(srcs, srcs_keys, touched_ports_counter, allocator, ports, ports_key, int_devices, fwd_rules, capacity, lcore_id,
                         time);
  loop_reset(srcs, srcs_keys, touched_ports_counter, allocator, ports, ports_key, int_devices, fwd_rules, capacity, lcore_id, &time);
  loop_invariant_produce(srcs, srcs_keys, touched_ports_counter, allocator, ports, ports_key, int_devices, fwd_rules, capacity, &lcore_id,
                         &time);
}
#endif // KLEE_VERIFICATION