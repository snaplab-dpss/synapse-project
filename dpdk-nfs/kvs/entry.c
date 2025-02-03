#include "entry.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr kv_key_descrs[] = {
    {0, sizeof(uint8_t) * KEY_SIZE_BYTES, 0, "data"},
};
struct nested_field_descr kv_key_nests[] = {};
#endif // KLEE_VERIFICATION