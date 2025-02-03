#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/state/double-chain.h"
#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/cht.h"
#include "lib/util/time.h"
#include "lib/state/fwtbl.h"

#include "flow.h"

void loop_invariant_consume(struct Map **fm, struct Vector **fv, struct DoubleChain **heap, int max_flows, struct ForwardingTable **ft,
                            unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct Map **fm, struct Vector **fv, struct DoubleChain **heap, int max_flows, struct ForwardingTable **ft,
                            unsigned int *lcore_id, time_ns_t *time);

void loop_iteration_border(struct Map **fm, struct Vector **fv, struct DoubleChain **heap, int max_flows, struct ForwardingTable **ft,
                           unsigned int lcore_id, time_ns_t time);

#endif //_LOOP_H_INCLUDED_