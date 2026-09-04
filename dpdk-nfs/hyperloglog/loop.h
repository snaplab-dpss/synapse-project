#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/state/vector.h"
#include "lib/util/time.h"

void loop_invariant_consume(struct Vector **estimators, struct Vector **accumulator, struct Vector **nonzero_count, unsigned int lcore_id,
                            time_ns_t time);

void loop_invariant_produce(struct Vector **estimators, struct Vector **accumulator, struct Vector **nonzero_count, unsigned int *lcore_id,
                            time_ns_t *time);

void loop_iteration_border(struct Vector **estimators, struct Vector **accumulator, struct Vector **nonzero_count, unsigned int lcore_id,
                           time_ns_t time);

#endif //_LOOP_H_INCLUDED_
