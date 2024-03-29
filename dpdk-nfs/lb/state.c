#include "state.h"

#include <stdlib.h>

#include "lib/verified/boilerplate-util.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/verified/double-chain-control.h"
#include "lib/models/verified/ether.h"
#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
#include "lib/models/verified/lpm-dir-24-8-control.h"
#endif  // KLEE_VERIFICATION

struct State* allocated_nf_state = NULL;

bool lb_backend_id_condition(void* value, int index, void* state) {
  struct ip_addr* v = value;
  return (0 <= index) AND(index < 32);
}
bool lb_flow_id_condition(void* value, int index, void* state) {
  struct LoadBalancedFlow* v = value;
  return (0 <= index) AND(index < 65536);
}
bool lb_backend_condition(void* value, int index, void* state) {
  struct LoadBalancedBackend* v = value;
  return (0 <= v->nic) AND(v->nic < 3) AND(v->nic != 2);
}
bool lb_flow_id2backend_id_cond(void* value, int index, void* state) {
  uint32_t v = *(uint32_t*)value;
  return (v < 32);
}
struct State* alloc_state(uint32_t backend_capacity, uint32_t flow_capacity,
                          uint32_t cht_height) {
  if (allocated_nf_state != NULL) return allocated_nf_state;
  struct State* ret = malloc(sizeof(struct State));
  if (ret == NULL) return NULL;
  ret->flow_to_flow_id = NULL;
  if (map_allocate(flow_capacity, sizeof(struct LoadBalancedFlow),
                   &(ret->flow_to_flow_id)) == 0)
    return NULL;
  ret->flow_heap = NULL;
  if (vector_allocate(sizeof(struct LoadBalancedFlow), flow_capacity,
                      &(ret->flow_heap)) == 0)
    return NULL;
  ret->flow_chain = NULL;
  if (dchain_allocate(flow_capacity, &(ret->flow_chain)) == 0) return NULL;
  ret->flow_id_to_backend_id = NULL;
  if (vector_allocate(sizeof(uint32_t), flow_capacity,
                      &(ret->flow_id_to_backend_id)) == 0)
    return NULL;
  ret->ip_to_backend_id = NULL;
  if (map_allocate(backend_capacity, sizeof(struct ip_addr),
                   &(ret->ip_to_backend_id)) == 0)
    return NULL;
  ret->backend_ips = NULL;
  if (vector_allocate(sizeof(struct ip_addr), backend_capacity,
                      &(ret->backend_ips)) == 0)
    return NULL;
  ret->backends = NULL;
  if (vector_allocate(sizeof(struct LoadBalancedBackend), backend_capacity,
                      &(ret->backends)) == 0)
    return NULL;
  ret->active_backends = NULL;
  if (dchain_allocate(backend_capacity, &(ret->active_backends)) == 0)
    return NULL;
  ret->cht = NULL;
  if (vector_allocate(sizeof(uint32_t), backend_capacity * cht_height,
                      &(ret->cht)) == 0)
    return NULL;
  if (cht_fill_cht(ret->cht, cht_height, backend_capacity) == 0) return NULL;
  ret->backend_capacity = backend_capacity;
  ret->flow_capacity = flow_capacity;
  ret->cht_height = cht_height;
#ifdef KLEE_VERIFICATION
  map_set_layout(
      ret->flow_to_flow_id, LoadBalancedFlow_descrs,
      sizeof(LoadBalancedFlow_descrs) / sizeof(LoadBalancedFlow_descrs[0]),
      LoadBalancedFlow_nests,
      sizeof(LoadBalancedFlow_nests) / sizeof(LoadBalancedFlow_nests[0]),
      "LoadBalancedFlow");
  map_set_entry_condition(ret->flow_to_flow_id, lb_flow_id_condition, ret);
  vector_set_layout(
      ret->flow_heap, LoadBalancedFlow_descrs,
      sizeof(LoadBalancedFlow_descrs) / sizeof(LoadBalancedFlow_descrs[0]),
      LoadBalancedFlow_nests,
      sizeof(LoadBalancedFlow_nests) / sizeof(LoadBalancedFlow_nests[0]),
      "LoadBalancedFlow");
  vector_set_layout(ret->flow_id_to_backend_id, NULL, 0, NULL, 0, "uint32_t");
  vector_set_entry_condition(ret->flow_id_to_backend_id,
                             lb_flow_id2backend_id_cond, ret);
  map_set_layout(ret->ip_to_backend_id, ip_addr_descrs,
                 sizeof(ip_addr_descrs) / sizeof(ip_addr_descrs[0]),
                 ip_addr_nests,
                 sizeof(ip_addr_nests) / sizeof(ip_addr_nests[0]), "ip_addr");
  map_set_entry_condition(ret->ip_to_backend_id, lb_backend_id_condition, ret);
  vector_set_layout(
      ret->backend_ips, ip_addr_descrs,
      sizeof(ip_addr_descrs) / sizeof(ip_addr_descrs[0]), ip_addr_nests,
      sizeof(ip_addr_nests) / sizeof(ip_addr_nests[0]), "ip_addr");
  vector_set_layout(
      ret->backends, LoadBalancedBackend_descrs,
      sizeof(LoadBalancedBackend_descrs) /
          sizeof(LoadBalancedBackend_descrs[0]),
      LoadBalancedBackend_nests,
      sizeof(LoadBalancedBackend_nests) / sizeof(LoadBalancedBackend_nests[0]),
      "LoadBalancedBackend");
  vector_set_entry_condition(ret->backends, lb_backend_condition, ret);
  vector_set_layout(ret->cht, NULL, 0, NULL, 0, "uint32_t");
#endif  // KLEE_VERIFICATION
  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(
      &allocated_nf_state->flow_to_flow_id, &allocated_nf_state->flow_heap,
      &allocated_nf_state->flow_chain,
      &allocated_nf_state->flow_id_to_backend_id,
      &allocated_nf_state->ip_to_backend_id, &allocated_nf_state->backend_ips,
      &allocated_nf_state->backends, &allocated_nf_state->active_backends,
      &allocated_nf_state->cht, allocated_nf_state->backend_capacity,
      allocated_nf_state->flow_capacity, allocated_nf_state->cht_height,
      lcore_id, time);
}

#endif  // KLEE_VERIFICATION
