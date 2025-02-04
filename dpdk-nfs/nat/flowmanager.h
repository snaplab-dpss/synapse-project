#ifndef _FLOWMANAGER_H_INCLUDED_
#define _FLOWMANAGER_H_INCLUDED_

#include "config.h"
#include "flow.h"
#include "lib/util/time.h"

#include <stdbool.h>
#include <stdint.h>
#include <rte_ether.h>

struct FlowManager;

struct FlowManager *flow_manager_allocate();
bool flow_manager_allocate_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time, uint16_t *external_port);
void flow_manager_expire(struct FlowManager *manager, time_ns_t time);
bool flow_manager_get_internal(struct FlowManager *manager, struct FlowId *id, time_ns_t time, uint16_t *external_port);
bool flow_manager_get_external(struct FlowManager *manager, uint16_t external_port, time_ns_t time, struct FlowId *out_flow);

#endif //_FLOWMANAGER_H_INCLUDED_
