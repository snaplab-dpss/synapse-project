#ifndef _CHT_STUB_CONTROL_H_INCLUDED_
#define _CHT_STUB_CONTROL_H_INCLUDED_

#include "lib/state/cht.h"
#include "lib/models/str-descr.h"

#define PREALLOC_SIZE (256)

void cht_set_layout(struct CHT *cht, struct str_field_descr *key_fields, int key_fields_count, struct nested_field_descr *key_nests,
                    int nested_key_fields_count, char *key_type);

void cht_reset(struct CHT *cht);

#endif