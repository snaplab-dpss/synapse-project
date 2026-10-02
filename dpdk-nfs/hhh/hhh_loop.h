#ifndef _HHH_LOOP_H_INCLUDED_
#define _HHH_LOOP_H_INCLUDED_

#include "lib/state/vector.h"
#include "lib/util/time.h"
#include "lib/util/math.h"

#include "hp_slot.h"

void loop_invariant_consume(struct Vector **counts8, struct Vector **counts16, struct Vector ***tables24, struct crc32_hasher ***hashers,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t stages, uint32_t dev_count, unsigned int lcore_id,
                            time_ns_t time);

void loop_invariant_produce(struct Vector **counts8, struct Vector **counts16, struct Vector ***tables24, struct crc32_hasher ***hashers,
                            struct Vector **int_devices, struct Vector **fwd_rules, uint32_t stages, uint32_t dev_count, unsigned int *lcore_id,
                            time_ns_t *time);

void loop_iteration_border(struct Vector **counts8, struct Vector **counts16, struct Vector ***tables24, struct crc32_hasher ***hashers,
                           struct Vector **int_devices, struct Vector **fwd_rules, uint32_t stages, uint32_t dev_count, unsigned int lcore_id,
                           time_ns_t time);

#endif //_HHH_LOOP_H_INCLUDED_
