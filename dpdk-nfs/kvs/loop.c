#ifdef KLEE_VERIFICATION
#include <klee/klee.h>

#include "loop.h"

#include "lib/models/verified/vigor-time-control.h"
#include "lib/models/verified/double-chain-control.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
#include "lib/models/verified/cms-control.h"

void loop_reset(struct Map **kvs, struct Vector **keys, struct Vector **values,
                struct CMS **not_cached_counters,
                struct Vector **cached_counters, int max_entries,
                unsigned int lcore_id, time_ns_t *time) {
  map_reset(*kvs);
  vector_reset(*keys);
  vector_reset(*values);
  cms_reset(*not_cached_counters);
  vector_reset(*cached_counters);
  *time = restart_time();
}

void loop_invariant_consume(struct Map **kvs, struct Vector **keys,
                            struct Vector **values,
                            struct CMS **not_cached_counters,
                            struct Vector **cached_counters, int max_entries,
                            unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(kvs, sizeof(struct Map *), "kvs");
  klee_trace_param_ptr(keys, sizeof(struct Vector *), "keys");
  klee_trace_param_ptr(values, sizeof(struct Vector *), "values");
  klee_trace_param_ptr(not_cached_counters, sizeof(struct CMS *),
                       "not_cached_counters");
  klee_trace_param_ptr(cached_counters, sizeof(struct Vector *),
                       "cached_counters");
  klee_trace_param_i32(max_entries, "max_entries");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}

void loop_invariant_produce(struct Map **kvs, struct Vector **keys,
                            struct Vector **values,
                            struct CMS **not_cached_counters,
                            struct Vector **cached_counters, int max_entries,
                            unsigned int *lcore_id, time_ns_t *time) {
  klee_trace_ret();
  klee_trace_param_ptr(kvs, sizeof(struct Map *), "kvs");
  klee_trace_param_ptr(keys, sizeof(struct Vector *), "keys");
  klee_trace_param_ptr(values, sizeof(struct Vector *), "values");
  klee_trace_param_ptr(not_cached_counters, sizeof(struct CMS *),
                       "not_cached_counters");
  klee_trace_param_ptr(cached_counters, sizeof(struct Vector *),
                       "cached_counters");
  klee_trace_param_i32(max_entries, "max_entries");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}

void loop_iteration_border(struct Map **kvs, struct Vector **keys,
                           struct Vector **values,
                           struct CMS **not_cached_counters,
                           struct Vector **cached_counters, int max_entries,
                           unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(kvs, keys, values, not_cached_counters,
                         cached_counters, max_entries, lcore_id, time);
  loop_reset(kvs, keys, values, not_cached_counters, cached_counters,
             max_entries, lcore_id, &time);
  loop_invariant_produce(kvs, keys, values, not_cached_counters,
                         cached_counters, max_entries, &lcore_id, &time);
}
#endif // KLEE_VERIFICATION