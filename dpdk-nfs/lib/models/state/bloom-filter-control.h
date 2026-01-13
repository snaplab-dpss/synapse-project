#ifndef _BLOOM_FILTER_STUB_CONTROL_H_INCLUDED_
#define _BLOOM_FILTER_STUB_CONTROL_H_INCLUDED_

#include "lib/state/bloom-filter.h"
#include "lib/models/str-descr.h"
#include "map-control.h"

#define PREALLOC_SIZE (256)

typedef map_entry_condition bf_entry_condition;

struct BloomFilter {
  struct str_field_descr key_fields[PREALLOC_SIZE];
  struct nested_field_descr key_nests[PREALLOC_SIZE];
  int key_field_count;
  int nested_key_field_count;
  int has_layout;
  int key_size;
  char *key_type;
};

struct hash {
  uint32_t value;
};

struct bf_bucket {
  uint32_t value;
};

void bf_set_layout(struct BloomFilter *bf, struct str_field_descr *key_fields, int key_fields_count, struct nested_field_descr *key_nests,
                   int nested_key_fields_count, char *key_type);

void bf_set_entry_condition(struct BloomFilter *bf, bf_entry_condition *cond, void *state);

void bf_reset(struct BloomFilter *bf);

#endif