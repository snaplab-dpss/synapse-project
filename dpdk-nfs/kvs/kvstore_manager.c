#include <assert.h>
#include <stdlib.h>
#include <string.h>  //for memcpy
#include <rte_ethdev.h>

#include "lib/verified/double-chain.h"
#include "lib/verified/map.h"
#include "lib/verified/vector.h"
#include "lib/verified/expirator.h"

#include "kvstore_manager.h"
#include "state.h"
#include "../nf-log.h"

struct State *state;

struct State *state_allocate(uint64_t max_entries) {
  state = alloc_state(max_entries);
  if (state == NULL) {
    return NULL;
  }

  return state;
}

bool kvstore_get(struct State *state, kv_key_t key, kv_value_t out_value) {
  int index;
  if (map_get(state->kvstore, key, &index) == 0) {
    return false;
  }

  void *value;
  vector_borrow(state->values, index, (void **)&value);
  memcpy(out_value, value, sizeof(kv_value_t));
  vector_return(state->values, index, value);

  return true;
}

bool kvstore_put(struct State *state, kv_key_t key, kv_value_t new_value,
                 time_ns_t time) {
  int index;
  if (map_get(state->kvstore, key, &index)) {
    NF_DEBUG("Updating value");

    void *value;
    vector_borrow(state->values, index, (void **)&value);
    memcpy(value, new_value, sizeof(kv_value_t));
    vector_return(state->values, index, value);

    return true;
  }

  if (!dchain_allocate_new_index(state->heap, &index, time)) {
    // No luck, the table is full, but we can at least let the
    // outgoing traffic out.
    return false;
  }

  NF_DEBUG("Allocating new entry");

  void *k;
  vector_borrow(state->keys, index, (void **)&k);
  memcpy(k, key, sizeof(kv_key_t));
  map_put(state->kvstore, k, index);
  vector_return(state->keys, index, k);

  void *v;
  vector_borrow(state->values, index, (void **)&v);
  memcpy(v, new_value, sizeof(kv_value_t));
  vector_return(state->values, index, v);

  return true;
}

bool kvstore_delete(struct State *state, kv_key_t key) {
  int index;
  if (map_get(state->kvstore, key, &index) == 0) {
    NF_DEBUG("Tried to delete a non-existent entry");
    return false;
  }

  NF_DEBUG("Deleting entry");
  if (dchain_free_index(state->heap, index) == 0) {
    // This should never happen, as we should always be able to free an index
    // if we got it from the dchain.
    return false;
  }

  void *trash;
  map_erase(state->kvstore, key, &trash);

  return true;
}