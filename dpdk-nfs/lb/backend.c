#include "backend.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION

struct str_field_descr Backend_descrs[] = {
    {offsetof(struct Backend, mac), sizeof(struct rte_ether_addr), 0, "mac"},
    {offsetof(struct Backend, ip), sizeof(uint32_t), 0, "ip"},
};

struct nested_field_descr Backend_nests[] = {
    {offsetof(struct Backend, mac), offsetof(struct rte_ether_addr, addr_bytes), sizeof(uint8_t), 6, "addr_bytes"},
};

#endif // KLEE_VERIFICATION