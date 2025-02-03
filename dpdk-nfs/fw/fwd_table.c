#include "fwd_table.h"

#include "lib/verified/map.h"
#include "lib/verified/vector.h"
#include "../nf-util.h"

#include <rte_ethdev.h>
#include <stdlib.h>

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

#include "lib/models/verified/map-control.h"
#include "lib/models/verified/vector-control.h"
#endif

struct FwdKey {
  uint16_t device;
} PACKED_FOR_KLEE_VERIFICATION;

int FwdTable_allocate(struct FwdTable **ft) {
  struct FwdTable *ret = (struct FwdTable *)malloc(sizeof(struct FwdTable));
  if (ret == NULL)
    return 0;

  uint16_t dev_count = rte_eth_dev_count_avail();
  uint16_t capacity  = get_next_power_of_two(dev_count) * 2;

  ret->src_dev_to_entry = NULL;
  if (map_allocate(capacity, sizeof(struct FwdKey), &(ret->src_dev_to_entry)) == 0)
    return 0;

  ret->src_devs = NULL;
  if (vector_allocate(sizeof(struct FwdKey), capacity, &(ret->src_devs)) == 0)
    return 0;

  ret->entries = NULL;
  if (vector_allocate(sizeof(struct FwdEntry), capacity, &(ret->entries)) == 0)
    return 0;

  *ft = ret;
  return 1;
}

#ifdef KLEE_VERIFICATION
// File parsing, is not really the kind of code we want to verify.
void FwdTable_fill(struct FwdTable *ft, const char *devices_cfg_fname) {}

struct str_field_descr device_descrs[] = {
    {offsetof(struct FwdKey, device), sizeof(struct FwdKey), 0, "device"},
};
struct nested_field_descr device_nests[] = {};

struct str_field_descr FwdEntry_descrs[] = {
    {offsetof(struct FwdEntry, device), sizeof(uint16_t), 0, "device"},
    {offsetof(struct FwdEntry, is_internal), sizeof(bool), 0, "is_internal"},
    {offsetof(struct FwdEntry, addr), sizeof(struct rte_ether_addr), 0, "addr"},
};
struct nested_field_descr FwdEntry_nests[] = {};

void FwdTable_set_layout(struct FwdTable *ft) {
  map_set_layout(ft->src_dev_to_entry, device_descrs, sizeof(device_descrs) / sizeof(device_descrs[0]), device_nests,
                 sizeof(device_nests) / sizeof(device_nests[0]), "device");
  vector_set_layout(ft->src_devs, NULL, 0, NULL, 0, "uint16_t");
  vector_set_layout(ft->entries, FwdEntry_descrs, sizeof(FwdEntry_descrs) / sizeof(FwdEntry_descrs[0]), FwdEntry_nests,
                    sizeof(FwdEntry_nests) / sizeof(FwdEntry_nests[0]), "FwdEntry");
}

void FwdTable_reset(struct FwdTable *ft) {
  map_reset(ft->src_dev_to_entry);
  vector_reset(ft->src_devs);
  vector_reset(ft->entries);
}
#else
void FwdTable_fill(struct FwdTable *ft, const char *devices_cfg_fname) { rte_exit(EXIT_FAILURE, "TODO: implement FwdTable_fill"); }
#endif

int FwdTable_lookup(struct FwdTable *ft, uint16_t src_dev, struct FwdEntry *out_entry) {
  int index;
  if (!map_get(ft->src_dev_to_entry, &src_dev, &index)) {
    return 0;
  }

  struct FwdEntry *entry;
  vector_borrow(ft->entries, index, (void **)&entry);
  memcpy(out_entry, entry, sizeof(struct FwdEntry));
  vector_return(ft->entries, index, entry);

  return 1;
}
