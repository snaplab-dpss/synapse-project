#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_
#include "lib/util/time.h"

void loop_invariant_consume(unsigned int lcore_id, time_ns_t time);
void loop_invariant_produce(unsigned int *lcore_id, time_ns_t *time);
void loop_iteration_border(unsigned int lcore_id, time_ns_t time);

#endif //_LOOP_H_INCLUDED_
