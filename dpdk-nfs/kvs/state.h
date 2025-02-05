#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"
#include "lib/state/lpm-dir-24-8.h"

struct State {
  struct Map *kvs;
  struct Vector *keys;
  struct Vector *values;
  struct DoubleChain *heap;
  struct LPM *fwd;

  uint32_t capacity;
  time_ns_t expiration_time;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_