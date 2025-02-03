#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "loop.h"

struct State {
  struct Map *fm;
  struct Vector *fv;
  struct DoubleChain *heap;
  int max_flows;
  struct ForwardingTable *fwd_table;
};

struct State *alloc_state(int max_flows);
#endif //_STATE_H_INCLUDED_
