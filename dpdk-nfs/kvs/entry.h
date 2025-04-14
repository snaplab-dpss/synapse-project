#ifndef _ENTRY_INCLUDED_
#define _ENTRY_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "lib/util/boilerplate.h"
#include "lib/util/ether.h"

#define KEY_SIZE_BYTES 16
#define MAX_VALUE_SIZE_BYTES 16

typedef uint8_t kv_key_t[KEY_SIZE_BYTES];
typedef uint8_t kv_value_t[MAX_VALUE_SIZE_BYTES];

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/models/str-descr.h"

extern struct str_field_descr kv_key_descrs[1];
extern struct nested_field_descr kv_key_nests[0];
#endif // KLEE_VERIFICATION

#endif //_ENTRY_INCLUDED_