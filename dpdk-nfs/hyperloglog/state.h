#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/vector.h"

struct State {
  struct Vector *estimators;
  struct Vector *accumulator;
  struct Vector *nonzero_count;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
