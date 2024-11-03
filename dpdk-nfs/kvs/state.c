#include "state.h"
#include <stdlib.h>

#include "lib/verified/boilerplate-util.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/verified/double-chain-control.h"
#include "lib/models/verified/ether.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
#include "lib/models/verified/cms-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state(uint32_t capacity, uint32_t sketch_height,
                          uint32_t sketch_width,
                          time_ns_t sketch_cleanup_interval) {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));
  if (ret == NULL)
    return NULL;

  ret->capacity = capacity;

  ret->kvs = NULL;
  if (map_allocate(capacity, sizeof(kv_key_t), &(ret->kvs)) == 0)
    return NULL;

  ret->keys = NULL;
  if (vector_allocate(sizeof(kv_key_t), capacity, &(ret->keys)) == 0)
    return NULL;

  ret->values = NULL;
  if (vector_allocate(sizeof(kv_value_t), capacity, &(ret->values)) == 0)
    return NULL;

  ret->not_cached_counters = NULL;
  if (cms_allocate(sketch_height, sketch_width, sizeof(kv_key_t),
                   sketch_cleanup_interval, &(ret->not_cached_counters)) == 0)
    return NULL;

  ret->cached_counters = NULL;
  if (vector_allocate(sizeof(uint64_t), capacity, &(ret->cached_counters)) == 0)
    return NULL;

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->kvs, kv_key_descrs,
                 sizeof(kv_key_descrs) / sizeof(kv_key_descrs[0]), kv_key_nests,
                 sizeof(kv_key_nests) / sizeof(kv_key_nests[0]), "kv_key_t");
  vector_set_layout(ret->keys, NULL, 0, NULL, 0, "kv_key_t");
  vector_set_layout(ret->values, NULL, 0, NULL, 0, "kv_value_t");
  cms_set_layout(ret->not_cached_counters, kv_key_descrs,
                 sizeof(kv_key_descrs) / sizeof(kv_key_descrs[0]), kv_key_nests,
                 sizeof(kv_key_nests) / sizeof(kv_key_nests[0]), "kv_key_t");
  vector_set_layout(ret->cached_counters, NULL, 0, NULL, 0, "uint64_t");
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->kvs, &allocated_nf_state->keys,
                        &allocated_nf_state->values,
                        &allocated_nf_state->not_cached_counters,
                        &allocated_nf_state->cached_counters,
                        allocated_nf_state->capacity, lcore_id, time);
}

#endif // KLEE_VERIFICATION
