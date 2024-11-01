#ifndef _TB_STUB_CONTROL_H_INCLUDED_
#define _TB_STUB_CONTROL_H_INCLUDED_

#include "lib/verified/token-bucket.h"
#include "lib/models/str-descr.h"
#include "../verified/map-control.h"
#include "../verified/vigor-time-control.h"

#define PREALLOC_SIZE (256)

void tb_set_layout(struct TokenBucket *tb, struct str_field_descr *key_fields,
                   int key_fields_count, struct nested_field_descr *key_nests,
                   int nested_key_fields_count, char *key_type);

void tb_reset(struct TokenBucket *tb);

#endif