#ifndef _PSD_LOOP_H_INCLUDED_
#define _PSD_LOOP_H_INCLUDED_

#include "lib/state/double-chain.h"
#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/cht.h"
#include "lib/state/lpm-dir-24-8.h"
#include "lib/util/time.h"

#include "ip_addr.h"
#include "counter.h"
#include "touched_port.h"

void loop_invariant_consume(struct Map **srcs, struct Vector **srcs_keys, struct Vector **touched_ports_counter,
                            struct DoubleChain **allocator, struct Map **ports, struct Vector **ports_key, uint32_t capacity,
                            uint32_t max_ports, uint32_t dev_count, unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct Map **srcs, struct Vector **srcs_keys, struct Vector **touched_ports_counter,
                            struct DoubleChain **allocator, struct Map **ports, struct Vector **ports_key, uint32_t capacity,
                            uint32_t max_ports, uint32_t dev_count, unsigned int *lcore_id, time_ns_t *time);

void loop_iteration_border(struct Map **srcs, struct Vector **srcs_keys, struct Vector **touched_ports_counter,
                           struct DoubleChain **allocator, struct Map **ports, struct Vector **ports_key, uint32_t capacity,
                           uint32_t max_ports, uint32_t dev_count, unsigned int lcore_id, time_ns_t time);

#endif //_PSD_LOOP_H_INCLUDED_
