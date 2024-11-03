#include <assert.h>
#include <stdlib.h>
#include <string.h> //for memcpy
#include <rte_ethdev.h>

#include "kvs_manager.h"
#include "kvs_hdr.h"
#include "state.h"
#include "../nf-log.h"

bool kvs_cache_lookup(struct State *state, enum kvs_op op, kv_key_t key,
                      kv_value_t value, int *index) {
  if (map_get(state->kvs, key, index) == 0) {
    return false;
  }

  NF_DEBUG("Cache hit");

  switch (op) {
  case KVS_OP_GET: {
    void *curr_value;
    vector_borrow(state->values, *index, (void **)&curr_value);
    memcpy(value, curr_value, sizeof(kv_value_t));
    vector_return(state->values, *index, curr_value);
  } break;
  case KVS_OP_PUT: {
    void *curr_value;
    vector_borrow(state->values, *index, (void **)&curr_value);
    memcpy(curr_value, value, sizeof(kv_value_t));
    vector_return(state->values, *index, curr_value);
  } break;
  case KVS_OP_DEL: {
    void *trash;
    map_erase(state->kvs, key, &trash);
  } break;
  }

  return true;
}

void kvs_update_stats(struct State *state, bool cache_hit, int cache_index,
                      kv_key_t key) {
  if (cache_hit) {
    uint64_t *counter = 0;
    vector_borrow(state->cached_counters, cache_index, (void **)&counter);
    (*counter)++;
    vector_return(state->cached_counters, cache_index, counter);
  } else {
    cms_increment(state->not_cached_counters, key);
  }
}

bool kvs_update_cache(struct State *state, kv_key_t key, kv_value_t value,
                      uint64_t counter) {
  uint32_t cache_size = map_size(state->kvs);
  int index;

  if (cache_size < state->capacity) {
    index = cache_size;
  } else if (!vector_sample_lt(state->cached_counters, state->sample_size,
                               &counter, &index)) {
    return false;
  }

  void *k;
  vector_borrow(state->keys, index, (void **)&k);

  if (cache_size >= state->capacity) {
    void *trash;
    map_erase(state->kvs, k, &trash);
  }

  memcpy(k, key, sizeof(kv_key_t));
  map_put(state->kvs, k, index);
  vector_return(state->keys, index, k);

  void *v;
  vector_borrow(state->values, index, (void **)&v);
  memcpy(v, value, sizeof(kv_value_t));
  vector_return(state->values, index, v);

  uint64_t *c = 0;
  vector_borrow(state->cached_counters, index, (void **)&c);
  *c = counter;
  vector_return(state->cached_counters, index, c);

  return true;
}
