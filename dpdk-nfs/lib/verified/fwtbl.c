#include "fwtbl.h"

#include "map.h"
#include "vector.h"

struct entry_t {
  uint16_t device;
  bool is_internal;
  struct rte_ether_addr addr;
};

struct key_t {
  uint16_t device;
};

struct ForwardingTable {
  struct Map *src_dev_to_entry;
  struct Vector *src_devs;
  struct Vector *entries;
};

static inline uint64_t get_next_power_of_two(uint64_t n) {
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  n++;
  return n;
}

int fwtbl_allocate(uint16_t max_devices, struct ForwardingTable **ft) {
  struct ForwardingTable *ret = (struct ForwardingTable *)malloc(sizeof(struct ForwardingTable));
  if (ret == NULL)
    return 0;

  uint16_t capacity = get_next_power_of_two(max_devices) * 2;

  ret->src_dev_to_entry = NULL;
  if (map_allocate(capacity, sizeof(struct key_t), &(ret->src_dev_to_entry)) == 0)
    return 0;

  ret->src_devs = NULL;
  if (vector_allocate(sizeof(struct key_t), capacity, &(ret->src_devs)) == 0)
    return 0;

  ret->entries = NULL;
  if (vector_allocate(sizeof(struct entry_t), capacity, &(ret->entries)) == 0)
    return 0;

  *ft = ret;
  return 1;
}

void fwtbl_fill(struct ForwardingTable *ft, const char *cfg_fname) {
  // TODO: implement
}

int fwtbl_lookup(struct ForwardingTable *ft, uint16_t src_dev, uint16_t *dst_dev, bool *is_internal, struct rte_ether_addr *dst_addr) {
  int index;
  if (!map_get(ft->src_dev_to_entry, &src_dev, &index)) {
    return 0;
  }

  struct entry_t *entry;
  vector_borrow(ft->entries, index, (void **)&entry);
  *dst_dev     = entry->device;
  *is_internal = entry->is_internal;
  rte_ether_addr_copy(&entry->addr, dst_addr);
  vector_return(ft->entries, index, entry);

  return 1;
}