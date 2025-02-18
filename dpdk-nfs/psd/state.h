#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"

struct State {
  struct Map *srcs;
  struct Vector *srcs_key;
  struct Vector *touched_ports_counter;
  struct DoubleChain *allocator;
  struct Map *ports;
  struct Vector *ports_key;
  struct Vector *int_devices;
  struct Vector *fwd_rules;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
