#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/double-chain-control.h"
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/cms-control.h"

void loop_reset(struct Map **flows, struct Vector **flows_keys, struct DoubleChain **flow_allocator, struct CMS **cms,
                struct Vector **int_devices, struct Vector **fwd_rules, uint32_t max_flows, unsigned int lcore_id, time_ns_t *time) {
  map_reset(*flows);
  vector_reset(*flows_keys);
  dchain_reset(*flow_allocator, max_flows);
  cms_reset((*cms));
  vector_reset(*int_devices);
  vector_reset(*fwd_rules);
  *time = restart_time();
}

void loop_invariant_consume(struct Map **flows, struct Vector **flows_keys, struct DoubleChain **flow_allocator, struct CMS **cms,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t max_flows, unsigned int lcore_id,
                            time_ns_t time) {
  klee_trace_ret();

  klee_trace_param_ptr(flows, sizeof(struct Map *), "flows");
  klee_trace_param_ptr(flows_keys, sizeof(struct Vector *), "flows_keys");
  klee_trace_param_ptr(flow_allocator, sizeof(struct DoubleChain *), "flow_allocator");
  klee_trace_param_ptr(cms, sizeof(struct CMS *), "cms");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");

  klee_trace_param_u32(max_flows, "max_flows");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Map **flows, struct Vector **flows_keys, struct DoubleChain **flow_allocator, struct CMS **cms,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t max_flows, unsigned int *lcore_id,
                            time_ns_t *time) {
  klee_trace_ret();

  klee_trace_param_ptr(flows, sizeof(struct Map *), "flows");
  klee_trace_param_ptr(flows_keys, sizeof(struct Vector *), "flows_keys");
  klee_trace_param_ptr(flow_allocator, sizeof(struct DoubleChain *), "flow_allocator");
  klee_trace_param_ptr(cms, sizeof(struct CMS *), "cms");
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");

  klee_trace_param_u32(max_flows, "max_flows");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Map **flows, struct Vector **flows_keys, struct DoubleChain **flow_allocator, struct CMS **cms,
                           struct Vector **int_devices, struct Vector **fwd_rules, uint32_t max_flows, unsigned int lcore_id,
                           time_ns_t time) {
  loop_invariant_consume(flows, flows_keys, flow_allocator, cms, int_devices, fwd_rules, max_flows, lcore_id, time);
  loop_reset(flows, flows_keys, flow_allocator, cms, int_devices, fwd_rules, max_flows, lcore_id, &time);
  loop_invariant_produce(flows, flows_keys, flow_allocator, cms, int_devices, fwd_rules, max_flows, &lcore_id, &time);
}
#endif // KLEE_VERIFICATION