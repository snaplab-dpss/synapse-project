#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "cl_loop.h"

struct State {
  struct Map *flows;
  struct Vector *flows_keys;
  struct DoubleChain *flow_allocator;
  struct CMS *cms;

  uint32_t max_flows;
  uint16_t max_clients;
  uint32_t dev_count;
};

struct State *alloc_state(uint32_t max_flows, uint16_t max_clients, uint32_t sketch_height, uint32_t sketch_width,
                          time_ns_t sketch_cleanup_interval, uint32_t dev_count);

#endif //_STATE_H_INCLUDED_
