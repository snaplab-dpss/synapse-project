#ifndef _STATE_H_INCLUDED_
#define _STATE_H_INCLUDED_

#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"
#include "lib/state/lpm.h"

struct State {
  // The watch list: domain-name pattern -> domain ID, matched longest-prefix so that a pattern
  // naming fewer labels than the name still matches it. Populated at startup, never written after.
  struct LPM *known_domains;

  // Clients whose traffic is not to be accounted for. A response addressed to one of them is
  // ignored however well its name matches.
  struct LPM *ignored_clients;

  // The DNS Response Table: client-server pair -> index, with the domain ID stored alongside and
  // an allocator providing the timeout eviction.
  struct Map *drt;
  struct Vector *drt_keys;
  struct Vector *drt_domains;
  struct DoubleChain *drt_allocator;

  // Per-domain counters, all indexed by domain ID.
  struct Vector *dns_queried; // DNS responses seen for a watched domain
  struct Vector *dns_missed;  // ... of those, the ones the DRT had no room for
  struct Vector *pkt_counts;  // data packets attributed to the domain
  struct Vector *byte_counts; // bytes attributed to the domain
};

struct State *alloc_state();

#endif //_STATE_H_INCLUDED_
