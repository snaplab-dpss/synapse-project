#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/vector.h"
#include "lib/state/bloom-filter.h"

struct State {
  // Connections whose setup the server agent has confirmed (keyed by client -> server 4-tuple).
  struct BloomFilter *verified;
  // Single entry: this device's clock minus the server's, in 2^16 ns ticks.
  struct Vector *timedelta;
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
