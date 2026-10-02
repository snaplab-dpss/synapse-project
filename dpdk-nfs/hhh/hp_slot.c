#include "hp_slot.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr hp_slot_descrs[] = {
    {offsetof(struct hp_slot, key), sizeof(uint32_t), 0, "key"},
    {offsetof(struct hp_slot, count), sizeof(uint32_t), 0, "count"},
};
struct nested_field_descr hp_slot_nests[] = {};
#endif // KLEE_VERIFICATION
