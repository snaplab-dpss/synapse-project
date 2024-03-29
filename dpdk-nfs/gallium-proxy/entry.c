#include "entry.h"

#ifdef KLEE_VERIFICATION
struct str_field_descr entry_descrs[] = {
    {offsetof(struct Entry, port), sizeof(uint16_t), 0, "port"},
};
struct nested_field_descr entry_nests[] = {};
#endif  // KLEE_VERIFICATION