#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

struct State {
  struct Map *kvs;
  struct Vector *keys;
  struct Vector *values;
  struct DoubleChain *heap;

  uint32_t capacity;
  time_ns_t expiration_time_ns;
};

struct State *alloc_state(uint32_t capacity, uint64_t expiration_time_us);

#endif //_STATE_H_INCLUDED_