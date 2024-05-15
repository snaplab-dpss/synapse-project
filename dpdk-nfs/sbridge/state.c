#include "state.h"

#include <stdlib.h>

#include "lib/verified/boilerplate-util.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/verified/ether.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
#include "lib/models/verified/lpm-dir-24-8-control.h"
#endif  // KLEE_VERIFICATION

struct State* allocated_nf_state = NULL;

bool stat_map_condition(void* value, int index, void* state) {
  struct StaticKey* v = value;
  return (-1 <= index) AND(index <= 2);
}

struct State* alloc_state(uint32_t stat_capacity, uint32_t dev_count) {
  if (allocated_nf_state != NULL) return allocated_nf_state;
  struct State* ret = malloc(sizeof(struct State));
  if (ret == NULL) return NULL;
  ret->st_map = NULL;
  if (map_allocate(stat_capacity, sizeof(struct StaticKey), &(ret->st_map)) ==
      0)
    return NULL;
  ret->st_vec = NULL;
  if (vector_allocate(sizeof(struct StaticKey), stat_capacity,
                      &(ret->st_vec)) == 0)
    return NULL;
  ret->stat_capacity = stat_capacity;
  ret->dev_count = dev_count;
#ifdef KLEE_VERIFICATION
  map_set_layout(
      ret->st_map, StaticKey_descrs,
      sizeof(StaticKey_descrs) / sizeof(StaticKey_descrs[0]), StaticKey_nests,
      sizeof(StaticKey_nests) / sizeof(StaticKey_nests[0]), "StaticKey");
  map_set_entry_condition(ret->st_map, stat_map_condition, ret);
  vector_set_layout(
      ret->st_vec, StaticKey_descrs,
      sizeof(StaticKey_descrs) / sizeof(StaticKey_descrs[0]), StaticKey_nests,
      sizeof(StaticKey_nests) / sizeof(StaticKey_nests[0]), "StaticKey");
#endif  // KLEE_VERIFICATION
  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->st_map,
                        &allocated_nf_state->st_vec,
                        allocated_nf_state->stat_capacity,
                        allocated_nf_state->dev_count, lcore_id, time);
}

#endif  // KLEE_VERIFICATION
