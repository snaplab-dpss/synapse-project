#ifndef _LOOP_H_INCLUDED_
#define _LOOP_H_INCLUDED_

#include "lib/state/cht.h"
#include "lib/state/map.h"
#include "lib/state/double-chain.h"
#include "lib/state/vector.h"
#include "lib/util/time.h"

#include "ip_addr.h"
#include "backend.h"
#include "flow.h"

void loop_invariant_consume(struct CHT **flow_to_backend_id, struct Map **backend_to_backend_id, struct Dchain **backends_heap,
                            struct Vector **backends, unsigned int lcore_id, time_ns_t time);

void loop_invariant_produce(struct CHT **flow_to_backend_id, struct Map **backend_to_backend_id, struct Dchain **backends_heap,
                            struct Vector **backends, unsigned int *lcore_id, time_ns_t *time);

void loop_iteration_border(struct CHT **flow_to_backend_id, struct Map **backend_to_backend_id, struct Dchain **backends_heap,
                           struct Vector **backends, uint32_t max_backends, unsigned int lcore_id, time_ns_t time);

#endif //_LOOP_H_INCLUDED_