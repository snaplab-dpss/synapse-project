#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"
#include "lib/state/cms.h"

struct State {
  struct Map *flows;
  struct Vector *flows_keys;
  struct DoubleChain *flow_allocator;
  struct CMS *cms;
  struct Vector *int_devices;
  struct Vector *fwd_rules;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
