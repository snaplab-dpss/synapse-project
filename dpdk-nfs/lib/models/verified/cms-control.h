#ifndef _CMS_STUB_CONTROL_H_INCLUDED_
#define _CMS_STUB_CONTROL_H_INCLUDED_

#include "lib/verified/cms.h"
#include "lib/models/str-descr.h"
#include "map-control.h"

#define PREALLOC_SIZE (256)

typedef map_entry_condition cms_entry_condition;

struct CMS {
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

struct cms_bucket {
  uint32_t value;
};

void cms_set_layout(struct CMS *cms, struct str_field_descr *key_fields,
                    int key_fields_count, struct nested_field_descr *key_nests,
                    int nested_key_fields_count, char *key_type);

void cms_set_entry_condition(struct CMS *cms, cms_entry_condition *cond,
                             void *state);

void cms_reset(struct CMS *cms);

#endif