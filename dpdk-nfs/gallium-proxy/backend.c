#include "backend.h"

#ifdef KLEE_VERIFICATION
struct str_field_descr backend_descrs[] = {
    {offsetof(struct Backend, ip), sizeof(uint32_t), 0, "src_ip"},
    {offsetof(struct Backend, port), sizeof(uint16_t), 0, "src_port"},
};
struct nested_field_descr backend_nests[] = {};
#endif  // KLEE_VERIFICATION