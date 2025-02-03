#include "dynamic_value.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr DynamicValue_descrs[] = {
    {offsetof(struct DynamicValue, bucket_size), sizeof(uint64_t), 0, "bucket_size"},
    {offsetof(struct DynamicValue, bucket_time), sizeof(int64_t), 0, "bucket_time"},
};
struct nested_field_descr DynamicValue_nests[] = {};
#endif // KLEE_VERIFICATION