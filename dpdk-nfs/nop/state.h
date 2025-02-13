#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

#include "lib/state/vector.h"

struct State {
  struct Vector *fwd_rules;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
