#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/token-bucket.h"
#include "lib/state/vector.h"

struct State {
  struct TokenBucket *tb;
  struct Vector *int_devices;
  struct Vector *fwd_rules;
  uint32_t capacity;
  uint64_t rate;
  uint64_t burst;
  uint32_t dev_count;
};

struct State *alloc_state(uint32_t capacity, uint64_t rate, uint64_t burst, uint32_t dev_count);

#endif //_STATE_H_INCLUDED_
