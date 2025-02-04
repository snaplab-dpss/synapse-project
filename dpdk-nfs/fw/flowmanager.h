#ifndef _FLOWMANAGER_H_INCLUDED_
#define _FLOWMANAGER_H_INCLUDED_

#include "flow.h"
#include "lib/util/time.h"

#include <stdbool.h>
#include <stdint.h>

struct FlowManager;

struct FlowManager *flow_manager_allocate();
void flow_manager_allocate_or_refresh_flow(struct FlowManager *manager, struct FlowId *id, uint16_t internal_device, time_ns_t time);
void flow_manager_expire(struct FlowManager *manager, time_ns_t time);
bool flow_manager_get_refresh_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time, uint16_t *internal_device);

#endif //_FLOWMANAGER_H_INCLUDED_
