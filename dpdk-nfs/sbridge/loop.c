#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "loop.h"
#include "lib/models/verified/vigor-time-control.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
void loop_reset(struct Map** st_map, struct Vector** st_vec,
                uint32_t stat_capacity, uint32_t dev_count,
                unsigned int lcore_id, time_ns_t* time) {
  map_reset(*st_map);
  vector_reset(*st_vec);
  *time = restart_time();
}
void loop_invariant_consume(struct Map** st_map, struct Vector** st_vec,
                            uint32_t stat_capacity, uint32_t dev_count,
                            unsigned int lcore_id, time_ns_t time) {
  klee_trace_ret();
  klee_trace_param_ptr(st_map, sizeof(struct Map*), "st_map");
  klee_trace_param_ptr(st_vec, sizeof(struct Vector*), "st_vec");
  klee_trace_param_u32(stat_capacity, "stat_capacity");
  klee_trace_param_u32(dev_count, "dev_count");
  klee_trace_param_i32(lcore_id, "lcore_id");
  klee_trace_param_i64(time, "time");
}
void loop_invariant_produce(struct Map** st_map, struct Vector** st_vec,
                            uint32_t stat_capacity, uint32_t dev_count,
                            unsigned int* lcore_id, time_ns_t* time) {
  klee_trace_ret();
  klee_trace_param_ptr(st_map, sizeof(struct Map*), "st_map");
  klee_trace_param_ptr(st_vec, sizeof(struct Vector*), "st_vec");
  klee_trace_param_u32(stat_capacity, "stat_capacity");
  klee_trace_param_u32(dev_count, "dev_count");
  klee_trace_param_ptr(lcore_id, sizeof(unsigned int), "lcore_id");
  klee_trace_param_ptr(time, sizeof(time_ns_t), "time");
}
void loop_iteration_border(struct Map** st_map, struct Vector** st_vec,
                           uint32_t stat_capacity, uint32_t dev_count,
                           unsigned int lcore_id, time_ns_t time) {
  loop_invariant_consume(st_map, st_vec, stat_capacity, dev_count, lcore_id,
                         time);
  loop_reset(st_map, st_vec, stat_capacity, dev_count, lcore_id, &time);
  loop_invariant_produce(st_map, st_vec, stat_capacity, dev_count, &lcore_id,
                         &time);
}
#endif  // KLEE_VERIFICATION