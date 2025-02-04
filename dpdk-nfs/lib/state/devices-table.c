#include "devices-table.h"
#include "map.h"
#include "vector.h"
#include "../util/compute.h"

struct entry_t {
  struct rte_ether_addr mac;
};

struct key_t {
  uint16_t device;
};

struct DevicesTable {
  struct Map *src_dev_to_entry;
  struct Vector *src_devs;
  struct Vector *entries;
};

int devtbl_allocate(uint16_t max_devices, struct DevicesTable **ft) {
  struct DevicesTable *ret = (struct DevicesTable *)malloc(sizeof(struct DevicesTable));
  if (ret == NULL)
    return 0;

  uint16_t capacity = ensure_power_of_two(max_devices) * 2;

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

void devtbl_fill(struct DevicesTable *ft, const char *cfg_fname) {
  // TODO: implement
}

int devtbl_lookup(struct DevicesTable *ft, uint16_t dev, struct rte_ether_addr *mac) {
  int index;
  if (!map_get(ft->src_dev_to_entry, &dev, &index)) {
    return 0;
  }

  struct entry_t *entry;
  vector_borrow(ft->entries, index, (void **)&entry);
  rte_ether_addr_copy(&entry->mac, mac);
  vector_return(ft->entries, index, entry);

  return 1;
}
