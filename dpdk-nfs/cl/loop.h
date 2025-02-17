#ifndef _CL_LOOP_H_INCLUDED_
#define _CL_LOOP_H_INCLUDED_

#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/state/double-chain.h"
#include "lib/state/cms.h"
#include "lib/util/time.h"

#include "flow.h"
#include "client.h"

void loop_invariant_consume(struct Map **flows, struct Vector **flows_keys, struct DoubleChain **flow_allocator, struct CMS **cms,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t max_flows, unsigned int lcore_id,
                            time_ns_t time);

void loop_invariant_produce(struct Map **flows, struct Vector **flows_keys, struct DoubleChain **flow_allocator, struct CMS **cms,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t max_flows, unsigned int *lcore_id,
                            time_ns_t *time);

void loop_iteration_border(struct Map **flows, struct Vector **flows_keys, struct DoubleChain **flow_allocator, struct CMS **cms,
                           struct Vector **int_devices, struct Vector **fwd_rules, uint32_t max_flows, unsigned int lcore_id,
                           time_ns_t time);

#endif //_PSD_LOOP_H_INCLUDED_