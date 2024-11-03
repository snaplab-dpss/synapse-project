#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/verified/map.h"
#include "lib/verified/vector.h"
#include "lib/verified/cms.h"
#include "lib/verified/vigor-time.h"

#include "entry.h"

void loop_invariant_consume(struct Map **kvs, struct Vector **keys,
                            struct Vector **values,
                            struct CMS **not_cached_counters,
                            struct Vector **cached_counters, int max_entries,
                            unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct Map **kvs, struct Vector **keys,
                            struct Vector **values,
                            struct CMS **not_cached_counters,
                            struct Vector **cached_counters, int max_entries,
                            unsigned int *lcore_id, time_ns_t *time);

void loop_iteration_border(struct Map **kvs, struct Vector **keys,
                           struct Vector **values,
                           struct CMS **not_cached_counters,
                           struct Vector **cached_counters, int max_entries,
                           unsigned int lcore_id, time_ns_t time);

#endif //_LOOP_H_INCLUDED_