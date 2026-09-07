#ifndef _SMARTCOOKIE_LOOP_H_INCLUDED_
#define _SMARTCOOKIE_LOOP_H_INCLUDED_

#include "lib/state/vector.h"
#include "lib/state/bloom-filter.h"
#include "lib/util/time.h"

void loop_invariant_consume(struct BloomFilter **verified, struct Vector **timedelta, unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct BloomFilter **verified, struct Vector **timedelta, unsigned int *lcore_id, time_ns_t *time);

void loop_iteration_border(struct BloomFilter **verified, struct Vector **timedelta, unsigned int lcore_id, time_ns_t time);

#endif //_SMARTCOOKIE_LOOP_H_INCLUDED_
