#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

#include "lib/state/lpm-dir-24-8.h"

struct State {
  struct LPM *fwd;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
