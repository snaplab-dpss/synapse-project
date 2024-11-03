#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

struct State {
  struct Map *kvs;
  struct Vector *keys;
  struct Vector *values;
  struct CMS *not_cached_counters;
  struct Vector *cached_counters;

  uint32_t capacity;
  uint32_t sample_size;
  uint64_t hh_threshold;
};

struct State *alloc_state(uint32_t capacity, uint32_t sketch_height,
                          uint32_t sketch_width,
                          time_ns_t sketch_cleanup_interval);

#endif //_STATE_H_INCLUDED_