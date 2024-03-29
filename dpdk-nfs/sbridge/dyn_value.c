#include "dyn_value.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr DynamicValue_descrs[] = {
    {offsetof(struct DynamicValue, device), sizeof(uint16_t), 0, "device"},
};
struct nested_field_descr DynamicValue_nests[] = {};
#endif  // KLEE_VERIFICATION