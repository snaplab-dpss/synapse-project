#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

struct State {
  struct Map* st_map;
  struct Vector* st_vec;
  uint32_t stat_capacity;
  uint32_t dev_count;
};

struct State* alloc_state(uint32_t stat_capacity, uint32_t dev_count);
#endif  //_STATE_H_INCLUDED_
