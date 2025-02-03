#ifndef _HHH_LOOP_H_INCLUDED_
#define _HHH_LOOP_H_INCLUDED_

#include "lib/state/double-chain.h"
#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/cht.h"
#include "lib/state/lpm-dir-24-8.h"
#include "lib/util/time.h"

#include "ip_addr.h"
#include "dynamic_value.h"

void loop_invariant_consume(struct Map ***subnet_indexers, struct DoubleChain ***allocators, struct Vector ***subnet_buckets,
                            struct Vector ***subnets, int n_subnets, uint32_t capacity, uint32_t dev_count, unsigned int lcore_id,
                            time_ns_t time);

void loop_invariant_produce(struct Map ***subnet_indexers, struct DoubleChain ***allocators, struct Vector ***subnet_buckets,
                            struct Vector ***subnets, int n_subnets, uint32_t capacity, uint32_t dev_count, unsigned int *lcore_id,
                            time_ns_t *time);

void loop_iteration_border(struct Map ***subnet_indexers, struct DoubleChain ***allocators, struct Vector ***subnet_buckets,
                           struct Vector ***subnets, int n_subnets, uint32_t capacity, uint32_t dev_count, unsigned int lcore_id,
                           time_ns_t time);

#endif //_HHH_LOOP_H_INCLUDED_