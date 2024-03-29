#include "counter.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr counter_descrs[] = {
    {offsetof(struct counter, value), sizeof(uint32_t), 0, "value"},
};
struct nested_field_descr counter_nests[] = {};
#endif  // KLEE_VERIFICATION