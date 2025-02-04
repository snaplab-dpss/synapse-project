#include "flowmanager.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h> //for memcpy
#include <rte_ethdev.h>

#include "lib/state/double-chain.h"
#include "lib/state/map.h"
#include "lib/state/vector.h"
#include "lib/util/expirator.h"

#include "state.h"
#include "config.h"

struct FlowManager {
  struct State *state;
  uint32_t expiration_time; /*nanoseconds*/
};

struct FlowManager *flow_manager_allocate() {
  struct FlowManager *manager = (struct FlowManager *)malloc(sizeof(struct FlowManager));
  if (manager == NULL) {
    return NULL;
  }
  manager->state = alloc_state();
  if (manager->state == NULL) {
    return NULL;
  }

  manager->expiration_time = config.expiration_time;

  return manager;
}

bool flow_manager_allocate_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time, uint16_t *external_port) {
  int index;
  if (dchain_allocate_new_index(manager->state->heap, &index, time) == 0) {
    return false;
  }

  *external_port = index;

  struct FlowId *key = 0;
  vector_borrow(manager->state->fv, index, (void **)&key);
  memcpy((void *)key, (void *)id, sizeof(struct FlowId));
  map_put(manager->state->fm, key, index);
  vector_return(manager->state->fv, index, key);
  return true;
}

void flow_manager_expire(struct FlowManager *manager, time_ns_t time) {
  assert(time >= 0); // we don't support the past
  assert(sizeof(time_ns_t) <= sizeof(uint64_t));
  uint64_t time_u                 = (uint64_t)time; // OK because of the two asserts
  time_ns_t vigor_time_expiration = (time_ns_t)manager->expiration_time;
  time_ns_t last_time             = time_u - vigor_time_expiration * 1000; // us to ns
  expire_items_single_map(manager->state->heap, manager->state->fv, manager->state->fm, last_time);
}

bool flow_manager_get_internal(struct FlowManager *manager, struct FlowId *id, time_ns_t time, uint16_t *external_port) {
  int index;
  if (map_get(manager->state->fm, id, &index) == 0) {
    return false;
  }
  *external_port = index;
  dchain_rejuvenate_index(manager->state->heap, index, time);
  return true;
}

bool flow_manager_get_external(struct FlowManager *manager, uint16_t external_port, time_ns_t time, struct FlowId *out_flow) {
  int index = external_port;
  if (dchain_is_index_allocated(manager->state->heap, index) == 0) {
    return false;
  }

  struct FlowId *key = 0;
  vector_borrow(manager->state->fv, index, (void **)&key);
  memcpy((void *)out_flow, (void *)key, sizeof(struct FlowId));
  vector_return(manager->state->fv, index, key);

  dchain_rejuvenate_index(manager->state->heap, index, time);

  return true;
}
