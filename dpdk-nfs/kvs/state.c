#include "state.h"
#include <stdlib.h>

#include "lib/verified/boilerplate-util.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/verified/double-chain-control.h"
#include "lib/models/verified/ether.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
#include "lib/models/verified/lpm-dir-24-8-control.h"
#endif  // KLEE_VERIFICATION

struct State* allocated_nf_state = NULL;

struct State* alloc_state(int max_entries) {
  if (allocated_nf_state != NULL) return allocated_nf_state;

  struct State* ret = malloc(sizeof(struct State));
  if (ret == NULL) return NULL;

  ret->max_entries = max_entries;

  ret->kvstore = NULL;
  if (map_allocate(max_entries, sizeof(kv_key_t), &(ret->kvstore)) == 0)
    return NULL;

  ret->keys = NULL;
  if (vector_allocate(sizeof(kv_key_t), max_entries, &(ret->keys)) == 0)
    return NULL;

  ret->values = NULL;
  if (vector_allocate(sizeof(kv_value_t), max_entries, &(ret->values)) == 0)
    return NULL;

  ret->heap = NULL;
  if (dchain_allocate(max_entries, &(ret->heap)) == 0) return NULL;

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->kvstore, kv_key_descrs,
                 sizeof(kv_key_descrs) / sizeof(kv_key_descrs[0]), kv_key_nests,
                 sizeof(kv_key_nests) / sizeof(kv_key_nests[0]), "kv_key_t");
  vector_set_layout(ret->keys, NULL, 0, NULL, 0, "kv_key_t");
  vector_set_layout(ret->values, NULL, 0, NULL, 0, "kv_value_t");
#endif  // KLEE_VERIFICATION

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->kvstore, &allocated_nf_state->keys,
                        &allocated_nf_state->values, &allocated_nf_state->heap,
                        allocated_nf_state->max_entries, lcore_id, time);
}

#endif  // KLEE_VERIFICATION
