#include "psd_state.h"

#include <stdlib.h>

#include "lib/util/boilerplate.h"
#ifdef KLEE_VERIFICATION
#include "lib/models/state/double-chain-control.h"
#include "lib/models/util/ether.h"
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/lpm-dir-24-8-control.h"

bool counter_condition(void *value, int index, void *state) {
  uint32_t *c     = (uint32_t *)value;
  struct State *s = (struct State *)state;
  return *c <= s->max_ports;
}

#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

struct State *alloc_state(uint32_t capacity, uint64_t max_ports, uint32_t dev_count) {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));

  if (ret == NULL)
    return NULL;

  ret->capacity = capacity;

  ret->srcs = NULL;
  if (map_allocate(capacity, sizeof(struct ip_addr), &(ret->srcs)) == 0) {
    return NULL;
  }

  ret->srcs_key = NULL;
  if (vector_allocate(sizeof(struct ip_addr), capacity, &(ret->srcs_key)) == 0) {
    return NULL;
  }

  ret->touched_ports_counter = NULL;
  if (vector_allocate(sizeof(struct counter), capacity, &(ret->touched_ports_counter)) == 0) {
    return NULL;
  }

  ret->allocator = NULL;
  if (dchain_allocate(capacity, &(ret->allocator)) == 0) {
    return NULL;
  }

  if (map_allocate(capacity * max_ports, sizeof(struct TouchedPort), &(ret->ports)) == 0) {
    return NULL;
  }

  if (vector_allocate(sizeof(struct TouchedPort), capacity * max_ports, &(ret->ports_key)) == 0) {
    return NULL;
  }

#ifdef KLEE_VERIFICATION
  map_set_layout(ret->srcs, ip_addr_descrs, sizeof(ip_addr_descrs) / sizeof(ip_addr_descrs[0]), ip_addr_nests,
                 sizeof(ip_addr_nests) / sizeof(ip_addr_nests[0]), "ip_addr");
  vector_set_layout(ret->srcs_key, ip_addr_descrs, sizeof(ip_addr_descrs) / sizeof(ip_addr_descrs[0]), ip_addr_nests,
                    sizeof(ip_addr_nests) / sizeof(ip_addr_nests[0]), "ip_addr");
  vector_set_layout(ret->touched_ports_counter, counter_descrs, sizeof(counter_descrs) / sizeof(counter_descrs[0]), counter_nests,
                    sizeof(counter_nests) / sizeof(counter_nests[0]), "counter");
  vector_set_entry_condition(ret->touched_ports_counter, counter_condition, ret);
  map_set_layout(ret->ports, touched_port_descrs, sizeof(touched_port_descrs) / sizeof(touched_port_descrs[0]), touched_port_nests,
                 sizeof(touched_port_nests) / sizeof(touched_port_nests[0]), "TouchedPort");
  vector_set_layout(ret->ports_key, touched_port_descrs, sizeof(touched_port_descrs) / sizeof(touched_port_descrs[0]), touched_port_nests,
                    sizeof(touched_port_nests) / sizeof(touched_port_nests[0]), "TouchedPort");
#endif // KLEE_VERIFICATION

  ret->capacity  = capacity;
  ret->max_ports = max_ports;
  ret->dev_count = dev_count;

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->srcs, &allocated_nf_state->srcs_key, &allocated_nf_state->touched_ports_counter,
                        &allocated_nf_state->allocator, &allocated_nf_state->ports, &allocated_nf_state->ports_key,
                        allocated_nf_state->capacity, allocated_nf_state->max_ports, allocated_nf_state->dev_count, lcore_id, time);
}

#endif // KLEE_VERIFICATION
