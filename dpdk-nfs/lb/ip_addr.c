#include "ip_addr.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr ip_addr_descrs[] = {
    {offsetof(struct ip_addr, addr), sizeof(uint32_t), 0, "addr"},
};
struct nested_field_descr ip_addr_nests[] = {

};
#endif // KLEE_VERIFICATION