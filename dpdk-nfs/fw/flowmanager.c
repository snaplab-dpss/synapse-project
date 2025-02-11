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
#include "../nf-log.h"

struct FlowManager *flow_manager_allocate() {
  struct FlowManager *manager = (struct FlowManager *)malloc(sizeof(struct FlowManager));
  if (manager == NULL) {
    return NULL;
  }

  manager->state = alloc_state();
  if (manager->state == NULL) {
    return NULL;
  }

  manager->expiration_time = config.expiration_time * 1000;

  return manager;
}

void flow_manager_allocate_or_refresh_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time) {
  int index;
  if (map_get(manager->state->fm, id, &index)) {
    NF_DEBUG("Rejuvenated flow");
    dchain_rejuvenate_index(manager->state->heap, index, time);
    return;
  }
  if (!dchain_allocate_new_index(manager->state->heap, &index, time)) {
    // No luck, the flow table is full, but we can at least let the
    // outgoing traffic out.
    return;
  }

  NF_DEBUG("Allocating new flow");

  struct FlowId *key = 0;
  vector_borrow(manager->state->fv, index, (void **)&key);
  memcpy((void *)key, (void *)id, sizeof(struct FlowId));
  map_put(manager->state->fm, key, index);
  vector_return(manager->state->fv, index, key);
}

void flow_manager_expire(struct FlowManager *manager, time_ns_t time) {
  assert(time >= 0); // we don't support the past
  assert(sizeof(time_ns_t) <= sizeof(uint64_t));
  uint64_t time_u     = (uint64_t)time; // OK because of the two asserts
  time_ns_t last_time = time_u - manager->expiration_time;
  expire_items_single_map(manager->state->heap, manager->state->fv, manager->state->fm, last_time);
}

bool flow_manager_get_refresh_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time) {
  int index;
  if (map_get(manager->state->fm, id, &index) == 0) {
    return false;
  }

  dchain_rejuvenate_index(manager->state->heap, index, time);
  return true;
}
