#include "state.h"
#include "ip_addr.h"
#include "counter.h"
#include "touched_port.h"
#include "loop.h"
#include "config.h"

#include <stdlib.h>

#include "lib/util/boilerplate.h"

#ifdef KLEE_VERIFICATION
#include "lib/models/state/map-control.h"
#include "lib/models/state/vector-control.h"
#include "lib/models/state/double-chain-control.h"
#endif // KLEE_VERIFICATION

bool counter_condition(void *value, int index, void *state) {
  uint32_t *c     = (uint32_t *)value;
  struct State *s = (struct State *)state;
  return *c <= config.max_ports;
}

bool port_validity(void *value, int index, void *state) {
  uint16_t dev       = *(uint16_t *)value;
  uint16_t dev_count = rte_eth_dev_count_avail();
  return (dev >= 0) AND((dev < dev_count) OR(dev == DROP));
}

bool bool_invariant(void *value, int index, void *state) { return (*(int *)value == 0) | (*(int *)value == 1); }

struct State *allocated_nf_state = NULL;

struct State *alloc_state() {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));

  if (ret == NULL)
    return NULL;

  ret->srcs = NULL;
  if (map_allocate(config.capacity, sizeof(struct ip_addr), &(ret->srcs)) == 0) {
    return NULL;
  }

  ret->srcs_key = NULL;
  if (vector_allocate(sizeof(struct ip_addr), config.capacity, &(ret->srcs_key)) == 0) {
    return NULL;
  }

  ret->touched_ports_counter = NULL;
  if (vector_allocate(sizeof(struct counter), config.capacity, &(ret->touched_ports_counter)) == 0) {
    return NULL;
  }

  ret->allocator = NULL;
  if (dchain_allocate(config.capacity, &(ret->allocator)) == 0) {
    return NULL;
  }

  ret->ports = NULL;
  if (map_allocate(config.capacity * config.max_ports, sizeof(struct TouchedPort), &(ret->ports)) == 0) {
    return NULL;
  }

  ret->ports_key = NULL;
  if (vector_allocate(sizeof(struct TouchedPort), config.capacity * config.max_ports, &(ret->ports_key)) == 0) {
    return NULL;
  }

  ret->int_devices = NULL;
  if (vector_allocate(sizeof(int), rte_eth_dev_count_avail(), &(ret->int_devices)) == 0)
    return NULL;

  ret->fwd_rules = NULL;
  if (vector_allocate(sizeof(uint16_t), rte_eth_dev_count_avail(), &(ret->fwd_rules)) == 0)
    return NULL;

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
  vector_set_layout(ret->int_devices, NULL, 0, NULL, 0, "int");
  vector_set_entry_condition(ret->int_devices, bool_invariant, ret);
  vector_set_layout(ret->fwd_rules, NULL, 0, NULL, 0, "uint16_t");
  vector_set_entry_condition(ret->fwd_rules, port_validity, ret);
#endif // KLEE_VERIFICATION

  for (size_t dev = 0; dev < rte_eth_dev_count_avail(); dev++) {
    int *is_internal;
    vector_borrow(ret->int_devices, dev, (void **)&is_internal);
    *is_internal = 0;
    for (size_t i = 0; i < config.internal_devs.n; i++) {
      if (config.internal_devs.devices[i] == dev) {
        *is_internal = 1;
        break;
      }
    }
    vector_return(ret->int_devices, dev, is_internal);

    uint16_t *dst_dev;
    vector_borrow(ret->fwd_rules, dev, (void **)&dst_dev);
    *dst_dev = DROP;
    for (size_t i = 0; i < config.fwd_rules.n; i++) {
      if (config.fwd_rules.src_dev[i] == dev) {
        *dst_dev = config.fwd_rules.dst_dev[i];
        break;
      }
    }
    vector_return(ret->fwd_rules, dev, dst_dev);
  }

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->srcs, &allocated_nf_state->srcs_key, &allocated_nf_state->touched_ports_counter,
                        &allocated_nf_state->allocator, &allocated_nf_state->ports, &allocated_nf_state->ports_key,
                        &allocated_nf_state->int_devices, &allocated_nf_state->fwd_rules, config.capacity, lcore_id, time);
}

#endif // KLEE_VERIFICATION
