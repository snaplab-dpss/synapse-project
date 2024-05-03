#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

struct State {
  struct Map* kvstore;
  struct Vector* keys;
  struct Vector* values;
  struct DoubleChain* heap;
  uint32_t max_entries;
};

struct State* alloc_state(int max_flows);
#endif  //_STATE_H_INCLUDED_
