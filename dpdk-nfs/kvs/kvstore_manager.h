#ifndef _KVSTORE_MANAGER_H_INCLUDED_
#define _KVSTORE_MANAGER_H_INCLUDED_

#include "entry.h"
#include "state.h"

#include "lib/verified/vigor-time.h"

#include <stdbool.h>
#include <stdint.h>

struct State;

struct State *state_allocate(uint64_t max_entries);

bool kvstore_get(struct State *state, kv_key_t key, kv_value_t out_value);
bool kvstore_put(struct State *state, kv_key_t key, kv_value_t value,
                 time_ns_t time);
bool kvstore_delete(struct State *state, kv_key_t key);

#endif  //_KVSTORE_MANAGER_H_INCLUDED_
