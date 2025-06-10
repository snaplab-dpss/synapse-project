#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/state/cht-control.h"
#include "lib/models/state/map-control.h"
#include "lib/models/state/double-chain-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/util/time-control.h"

void loop_reset(struct CHT **flow_to_backend_id, struct Map **backend_to_backend_id, struct Dchain **backends_heap, struct Vector **backends,
                uint32_t max_backends, unsigned int lcore_id, time_ns_t *time) {
  cht_reset(*flow_to_backend_id);
  map_reset(*backend_to_backend_id);
  dchain_reset(*backends_heap, max_backends);
  vector_reset(*backends);
  *time = restart_time();
}

void loop_invariant_consume(struct CHT **flow_to_backend_id, struct Map **backend_to_backend_id, struct Dchain **backends_heap,
                            struct Vector **backends, unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(flow_to_backend_id, sizeof(struct CHT *), "flow_to_backend_id");
  klee_trace_param_ptr(backend_to_backend_id, sizeof(struct Map *), "backend_to_backend_id");
  klee_trace_param_ptr(backends_heap, sizeof(struct DoubleChain *), "backends_heap");
  klee_trace_param_ptr(backends, sizeof(struct Vector *), "backends");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct CHT **flow_to_backend_id, struct Map **backend_to_backend_id, struct Dchain **backends_heap,
                            struct Vector **backends, unsigned int *lcore_id, time_ns_t *time) {
  klee_trace_ret();
  klee_trace_param_ptr(flow_to_backend_id, sizeof(struct CHT *), "flow_to_backend_id");
  klee_trace_param_ptr(backend_to_backend_id, sizeof(struct Map *), "backend_to_backend_id");
  klee_trace_param_ptr(backends_heap, sizeof(struct DoubleChain *), "backends_heap");
  klee_trace_param_ptr(backends, sizeof(struct Vector *), "backends");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct CHT **flow_to_backend_id, struct Map **backend_to_backend_id, struct Dchain **backends_heap,
                           struct Vector **backends, uint32_t max_backends, unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(flow_to_backend_id, backend_to_backend_id, backends_heap, backends, lcore_id, time);
  loop_reset(flow_to_backend_id, backend_to_backend_id, backends_heap, backends, max_backends, lcore_id, &time);
  loop_invariant_produce(flow_to_backend_id, backend_to_backend_id, backends_heap, backends, &lcore_id, &time);
}

#endif // KLEE_VERIFICATION