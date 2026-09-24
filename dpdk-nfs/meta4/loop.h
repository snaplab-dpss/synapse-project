#ifndef _META4_LOOP_H_INCLUDED_
#define _META4_LOOP_H_INCLUDED_

#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"
#include "lib/util/time.h"

#include "session.h"

void loop_invariant_consume(struct Map **drt, struct Vector **drt_keys, struct Vector **drt_domains,
                            struct DoubleChain **drt_allocator, struct Vector **dns_queried, struct Vector **dns_missed, struct Vector **pkt_counts,
                            struct Vector **byte_counts, uint32_t drt_capacity, unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct Map **drt, struct Vector **drt_keys, struct Vector **drt_domains,
                            struct DoubleChain **drt_allocator, struct Vector **dns_queried, struct Vector **dns_missed, struct Vector **pkt_counts,
                            struct Vector **byte_counts, uint32_t drt_capacity, unsigned int *lcore_id, time_ns_t *time);

void loop_iteration_border(struct Map **drt, struct Vector **drt_keys, struct Vector **drt_domains,
                           struct DoubleChain **drt_allocator, struct Vector **dns_queried, struct Vector **dns_missed, struct Vector **pkt_counts,
                           struct Vector **byte_counts, uint32_t drt_capacity, unsigned int lcore_id, time_ns_t time);

#endif //_META4_LOOP_H_INCLUDED_
