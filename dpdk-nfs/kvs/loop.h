#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/verified/double-chain.h"
#include "lib/verified/map.h"
#include "lib/verified/vector.h"
#include "lib/verified/cht.h"
#include "lib/verified/lpm-dir-24-8.h"
#include "lib/verified/vigor-time.h"

#include "entry.h"

void loop_invariant_consume(struct Map** kvstore, struct Vector** keys,
                            struct Vector** values, struct DoubleChain** heap,
                            int max_entries, unsigned int lcore_id,
                            time_ns_t time);

void loop_invariant_produce(struct Map** kvstore, struct Vector** keys,
                            struct Vector** values, struct DoubleChain** heap,
                            int max_entries, unsigned int* lcore_id,
                            time_ns_t* time);

void loop_iteration_border(struct Map** kvstore, struct Vector** keys,
                           struct Vector** values, struct DoubleChain** heap,
                           int max_entries, unsigned int lcore_id,
                           time_ns_t time);

#endif  //_LOOP_H_INCLUDED_