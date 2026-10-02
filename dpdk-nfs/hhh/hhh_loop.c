#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "hhh_loop.h"

#include "lib/models/util/time-control.h"
#include "lib/models/state/vector-control.h"

void loop_reset(struct Vector **counts8, struct Vector **counts16, struct Vector ***tables24, struct crc32_hasher ***hashers,
                struct Vector **int_devices, struct Vector **fwd_rules, uint32_t stages, uint32_t dev_count, unsigned int lcore_id, time_ns_t *time) {
  vector_reset(*counts8);
  vector_reset(*counts16);
  for (uint32_t s = 0; s < stages; s++) {
    vector_reset((*tables24)[s]);
  }
  vector_reset(*int_devices);
  vector_reset(*fwd_rules);

  *time = restart_time();
}

void loop_invariant_consume(struct Vector **counts8, struct Vector **counts16, struct Vector ***tables24, struct crc32_hasher ***hashers,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t stages, uint32_t dev_count, unsigned int lcore_id,
                            time_ns_t time) {
  klee_trace_ret();

  klee_trace_param_ptr(counts8, sizeof(struct Vector *), "counts8");
  klee_trace_param_ptr(counts16, sizeof(struct Vector *), "counts16");
  for (uint32_t s = 0; s < stages; s++) {
    klee_trace_param_ptr(&(*tables24)[s], sizeof(struct Vector *), "tables24");
  }
  for (uint32_t s = 0; s < stages; s++) {
    klee_trace_param_ptr(&(*hashers)[s], sizeof(struct crc32_hasher *), "hashers");
  }
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_u32(stages, "stages");
  klee_trace_param_u32(dev_count, "dev_count");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Vector **counts8, struct Vector **counts16, struct Vector ***tables24, struct crc32_hasher ***hashers,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t stages, uint32_t dev_count, unsigned int *lcore_id,
                            time_ns_t *time) {
  klee_trace_ret();

  klee_trace_param_ptr(counts8, sizeof(struct Vector *), "counts8");
  klee_trace_param_ptr(counts16, sizeof(struct Vector *), "counts16");
  for (uint32_t s = 0; s < stages; s++) {
    klee_trace_param_ptr(&(*tables24)[s], sizeof(struct Vector *), "tables24");
  }
  for (uint32_t s = 0; s < stages; s++) {
    klee_trace_param_ptr(&(*hashers)[s], sizeof(struct crc32_hasher *), "hashers");
  }
  klee_trace_param_ptr(int_devices, sizeof(struct Vector *), "int_devices");
  klee_trace_param_ptr(fwd_rules, sizeof(struct Vector *), "fwd_rules");
  klee_trace_param_u32(stages, "stages");
  klee_trace_param_u32(dev_count, "dev_count");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Vector **counts8, struct Vector **counts16, struct Vector ***tables24, struct crc32_hasher ***hashers,
                           struct Vector **int_devices, struct Vector **fwd_rules, uint32_t stages, uint32_t dev_count, unsigned int lcore_id,
                           time_ns_t time) {
  loop_invariant_consume(counts8, counts16, tables24, hashers, int_devices, fwd_rules, stages, dev_count, lcore_id, time);
  loop_reset(counts8, counts16, tables24, hashers, int_devices, fwd_rules, stages, dev_count, lcore_id, &time);
  loop_invariant_produce(counts8, counts16, tables24, hashers, int_devices, fwd_rules, stages, dev_count, &lcore_id, &time);
}
#endif // KLEE_VERIFICATION
