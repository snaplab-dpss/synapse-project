#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "lib/verified/boilerplate-util.h"
#include "lib/verified/ether.h"

struct FwdEntry {
  uint16_t device;
  bool is_internal;
  struct rte_ether_addr addr;
} PACKED_FOR_KLEE_VERIFICATION;

struct FwdTable {
  struct Map *src_dev_to_entry;
  struct Vector *src_devs;
  struct Vector *entries;
};

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"
void FwdTable_reset(struct FwdTable *ft);
void FwdTable_set_layout(struct FwdTable *ft);
#endif // KLEE_VERIFICATION

int FwdTable_allocate(struct FwdTable **ft);
void FwdTable_fill(struct FwdTable *ft, const char *devices_cfg_fname);
int FwdTable_lookup(struct FwdTable *ft, uint16_t src_dev, struct FwdEntry *entry);
