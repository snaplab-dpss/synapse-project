#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/verified/map.h"
#include "lib/verified/vector.h"
#include "lib/verified/lpm-dir-24-8.h"
#include "lib/verified/vigor-time.h"

#include "stat_key.h"

void loop_invariant_consume(struct Map** st_map, struct Vector** st_vec,
                            uint32_t stat_capacity, uint32_t dev_count,
                            unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct Map** st_map, struct Vector** st_vec,
                            uint32_t stat_capacity, uint32_t dev_count,
                            unsigned int* lcore_id, time_ns_t* time);

void loop_iteration_border(struct Map** st_map, struct Vector** st_vec,
                           uint32_t stat_capacity, uint32_t dev_count,
                           unsigned int lcore_id, time_ns_t time);

#endif  //_LOOP_H_INCLUDED_
