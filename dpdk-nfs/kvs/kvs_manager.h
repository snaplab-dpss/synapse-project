#ifndef _KVSTORE_MANAGER_H_INCLUDED_
#define _KVSTORE_MANAGER_H_INCLUDED_

#include "entry.h"
#include "kvs_hdr.h"
#include "state.h"

#include <stdbool.h>
#include <stdint.h>

bool kvs_cache_lookup(struct State *state, enum kvs_op op, kv_key_t key,
                      kv_value_t value, int *index);
void kvs_update_stats(struct State *state, bool cache_hit, int cache_index,
                      kv_key_t key);
bool kvs_update_cache(struct State *state, kv_key_t key, kv_value_t value,
                      uint64_t counter);

// bool kvs_get(struct State *state, kv_key_t key, kv_value_t out_value);
// bool kvs_put(struct State *state, kv_key_t key, kv_value_t value,
//              time_ns_t time);
// bool kvs_delete(struct State *state, kv_key_t key);
// bool kvs_insert(struct State *state, kv_key_t key, kv_value_t value);

#endif //_KVSTORE_MANAGER_H_INCLUDED_
