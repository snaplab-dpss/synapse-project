#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

struct State {
  struct TokenBucket *tb;
  uint32_t capacity;
  uint64_t rate;
  uint64_t burst;
  uint32_t dev_count;
};

struct State *alloc_state(uint32_t capacity, uint64_t rate, uint64_t burst, uint32_t dev_count);

#endif //_STATE_H_INCLUDED_
