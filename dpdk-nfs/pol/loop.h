#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/state/token-bucket.h"
#include "lib/state/vector.h"
#include "lib/util/time.h"

#include "ip_addr.h"

void loop_invariant_consume(struct TokenBucket **tb, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t dev_count,
                            unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct TokenBucket **tb, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t dev_count,
                            unsigned int lcore_id, time_ns_t *time);

void loop_iteration_border(struct TokenBucket **tb, struct Vector **int_devices, struct Vector **fwd_rules, uint32_t dev_count,
                           unsigned int lcore_id, time_ns_t time);

#endif //_LOOP_H_INCLUDED_