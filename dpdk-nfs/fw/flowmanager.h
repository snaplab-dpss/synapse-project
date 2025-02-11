#ifndef _FLOWMANAGER_H_INCLUDED_
#define _FLOWMANAGER_H_INCLUDED_

#include "flow.h"
#include "state.h"
#include "lib/util/time.h"

#include <stdbool.h>
#include <stdint.h>

struct FlowManager {
  struct State *state;
  time_ns_t expiration_time;
};

struct FlowManager *flow_manager_allocate();
void flow_manager_allocate_or_refresh_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time);
void flow_manager_expire(struct FlowManager *manager, time_ns_t time);
bool flow_manager_get_refresh_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time);

#endif //_FLOWMANAGER_H_INCLUDED_
