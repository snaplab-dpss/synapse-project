#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/util/time.h"
#include "lib/state/vector.h"

void loop_invariant_consume(struct Vector **fwd_rules, unsigned int lcore_id, time_ns_t time);
void loop_invariant_produce(struct Vector **fwd_rules, unsigned int *lcore_id, time_ns_t *time);
void loop_iteration_border(struct Vector **fwd_rules, unsigned int lcore_id, time_ns_t time);

#endif //_LOOP_H_INCLUDED_
