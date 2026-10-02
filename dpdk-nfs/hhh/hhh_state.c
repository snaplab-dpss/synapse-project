#include "hhh_state.h"
#include "hhh_config.h"

#include <stdlib.h>
#include <rte_ethdev.h>

extern struct nf_config config;

#include "lib/util/boilerplate.h"
#ifdef KLEE_VERIFICATION
#include "lib/models/state/vector-control.h"
#endif // KLEE_VERIFICATION

struct State *allocated_nf_state = NULL;

bool port_validity(void *value, int index, void *state) {
  uint16_t dev       = *(uint16_t *)value;
  uint16_t dev_count = rte_eth_dev_count_avail();
  return (dev >= 0) AND(dev < dev_count);
}

bool bool_invariant(void *value, int index, void *state) { return (*(int *)value == 0) | (*(int *)value == 1); }

struct State *alloc_state(uint32_t stages, uint32_t width, time_ns_t interval, uint32_t dev_count) {
  if (allocated_nf_state != NULL)
    return allocated_nf_state;

  struct State *ret = malloc(sizeof(struct State));

  if (ret == NULL)
    return NULL;

  ret->stages   = stages;
  ret->width    = width;
  ret->interval = interval;

  ret->counts8 = NULL;
  if (vector_allocate(sizeof(uint32_t), 256, &(ret->counts8)) == 0) {
    return NULL;
  }

  ret->counts16 = NULL;
  if (vector_allocate(sizeof(uint32_t), 65536, &(ret->counts16)) == 0) {
    return NULL;
  }

  ret->tables24 = (struct Vector **)malloc(sizeof(struct Vector *) * stages);
  for (uint32_t s = 0; s < stages; s++) {
    ret->tables24[s] = NULL;
    if (vector_allocate(sizeof(struct hp_slot), width, &(ret->tables24[s])) == 0) {
      return NULL;
    }
  }

  ret->hashers = (struct crc32_hasher **)malloc(sizeof(struct crc32_hasher *) * stages);
  for (uint32_t s = 0; s < stages; s++) {
    ret->hashers[s] = (struct crc32_hasher *)malloc(sizeof(struct crc32_hasher));
    if (ret->hashers[s] == NULL) {
      return NULL;
    }
    crc32_hasher_init(ret->hashers[s], &CRC32_BANK[s]);
  }

  ret->int_devices = NULL;
  if (vector_allocate(sizeof(int), dev_count, &(ret->int_devices)) == 0) {
    return NULL;
  }

  ret->fwd_rules = NULL;
  if (vector_allocate(sizeof(uint16_t), dev_count, &(ret->fwd_rules)) == 0) {
    return NULL;
  }

#ifdef KLEE_VERIFICATION
  vector_set_layout(ret->counts8, NULL, 0, NULL, 0, "uint32_t");
  vector_set_layout(ret->counts16, NULL, 0, NULL, 0, "uint32_t");
  for (uint32_t s = 0; s < stages; s++) {
    vector_set_layout(ret->tables24[s], hp_slot_descrs, sizeof(hp_slot_descrs) / sizeof(hp_slot_descrs[0]), hp_slot_nests,
                      sizeof(hp_slot_nests) / sizeof(hp_slot_nests[0]), "hp_slot");
  }
  vector_set_layout(ret->int_devices, NULL, 0, NULL, 0, "int");
  vector_set_entry_condition(ret->int_devices, bool_invariant, ret);
  vector_set_layout(ret->fwd_rules, NULL, 0, NULL, 0, "uint16_t");
  vector_set_entry_condition(ret->fwd_rules, port_validity, ret);
#endif // KLEE_VERIFICATION

  for (size_t dev = 0; dev < dev_count; dev++) {
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

  ret->dev_count = dev_count;

  allocated_nf_state = ret;
  return ret;
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time) {
  loop_iteration_border(&allocated_nf_state->counts8, &allocated_nf_state->counts16, &allocated_nf_state->tables24, &allocated_nf_state->hashers,
                        &allocated_nf_state->int_devices, &allocated_nf_state->fwd_rules, allocated_nf_state->stages, allocated_nf_state->dev_count,
                        lcore_id, time);
}

#endif // KLEE_VERIFICATION
