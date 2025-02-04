#include "lib/state/devices-table.h"

#include "devices-table-control.h"

#include <klee/klee.h>
#include <stdlib.h>
#include <string.h>
#include <rte_ether.h>

#include "../state/map-control.h"
#include "../state/vector-control.h"

#define PREALLOC_SIZE (256)

struct DevicesTable {
  uint32_t capacity;
};

void devtbl_set_layout(struct DevicesTable *devtbl) {}

void devtbl_reset(struct DevicesTable *devtbl) {}

int devtbl_allocate(uint16_t max_devices, struct DevicesTable **devtbl_out) {
  klee_trace_ret();

  klee_trace_param_u32(max_devices, "max_devices");
  klee_trace_param_ptr(devtbl_out, sizeof(struct DevicesTable *), "devtbl_out");

  int allocation_succeeded = klee_int("devtbl_allocation_succeeded");

  if (allocation_succeeded) {
    *devtbl_out = malloc(sizeof(struct DevicesTable));
    klee_make_symbolic((*devtbl_out), sizeof(struct DevicesTable), "devtbl");
    klee_assert((*devtbl_out) != NULL);
    (*devtbl_out)->capacity = max_devices;
    return 1;
  }

  return 0;
}

void devtbl_fill(struct DevicesTable *devtbl, const char *cfg_fname) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)devtbl, "devtbl");
  klee_trace_param_ptr((void *)cfg_fname, strlen(cfg_fname), "cfg_fname");
}

int devtbl_lookup(struct DevicesTable *devtbl, uint16_t dev, struct rte_ether_addr *mac) {
  klee_trace_ret();

  klee_trace_param_u64((uint64_t)devtbl, "devtbl");
  klee_trace_param_i32(dev, "dev");
  klee_trace_param_ptr(mac, sizeof(struct rte_ether_addr), "mac");

  int devices_table_hit = klee_int("devices_table_hit");
  if (devices_table_hit) {
    klee_make_symbolic(mac, sizeof(struct rte_ether_addr), "mac");
    return 1;
  }

  return 0;
}
