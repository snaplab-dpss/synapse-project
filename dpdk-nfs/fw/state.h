#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"

struct State {
  struct Map *fm;
  struct Vector *fv;
  struct DoubleChain *heap;
  struct Vector *int_devices;
  struct Vector *fwd_rules;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
