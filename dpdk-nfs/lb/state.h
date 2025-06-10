#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/cht.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"
#include "lib/state/map.h"

struct State {
  struct CHT *flow_to_backend_id;
  struct Map *backend_to_backend_id;
  struct Dchain *backends_heap;
  struct Vector *backends;

  uint32_t max_backends;
};

struct State *alloc_state(uint32_t max_backends, uint32_t cht_height);

#endif //_STATE_H_INCLUDED_
