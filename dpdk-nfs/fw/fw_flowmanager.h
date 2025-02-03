#ifndef _FLOWMANAGER_H_INCLUDED_
#define _FLOWMANAGER_H_INCLUDED_

#include "flow.h"
#include "fwd_table.h"
#include "lib/verified/vigor-time.h"

#include <stdbool.h>
#include <stdint.h>

struct FlowManager;

struct FlowManager *flow_manager_allocate(const char *devices_cfg_fname, uint32_t expiration_time, uint64_t max_flows);
void flow_manager_allocate_or_refresh_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time);
void flow_manager_expire(struct FlowManager *manager, time_ns_t time);
bool flow_manager_get_refresh_flow(struct FlowManager *manager, struct FlowId *id, time_ns_t time);
int flow_manager_fwd_table_lookup(struct FlowManager *manager, uint16_t src_dev, struct FwdEntry *entry);

#endif //_FLOWMANAGER_H_INCLUDED_
