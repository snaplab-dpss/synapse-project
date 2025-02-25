#include "state.h"
#include "entry.h"
#include "lib/util/boilerplate.h"
#include "config.h"
#include "loop.h"

#include <stdlib.h>
#include <rte_ethdev.h>

#ifdef KLEE_VERIFICATION
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/double-chain-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state() {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));
  if (ret == NULL)
    return NULL;

  ret->capacity        = config.capacity;
  ret->expiration_time = config.expiration_time_us * 1000;

  ret->kvs = NULL;
  if (map_allocate(config.capacity, sizeof(kv_key_t), &(ret->kvs)) == 0)
    return NULL;

  ret->keys = NULL;
  if (vector_allocate(sizeof(kv_key_t), config.capacity, &(ret->keys)) == 0)
    return NULL;

  ret->values = NULL;
  if (vector_allocate(sizeof(kv_value_t), config.capacity, &(ret->values)) == 0)
    return NULL;

  ret->heap = NULL;
  if (dchain_allocate(config.capacity, &(ret->heap)) == 0)
    return NULL;

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->kvs, kv_key_descrs, sizeof(kv_key_descrs) / sizeof(kv_key_descrs[0]), kv_key_nests,
                 sizeof(kv_key_nests) / sizeof(kv_key_nests[0]), "kv_key_t");
  vector_set_layout(ret->keys, NULL, 0, NULL, 0, "kv_key_t");
  vector_set_layout(ret->values, NULL, 0, NULL, 0, "kv_value_t");
#endif // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->kvs, &allocated_nf_state->keys, &allocated_nf_state->values, &allocated_nf_state->heap,
                        allocated_nf_state->capacity, lcore_id, time);
}

#endif // KLEE_VERIFICATION
