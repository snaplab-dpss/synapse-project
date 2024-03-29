#include "stat_key.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr StaticKey_descrs[] = {
    {offsetof(struct StaticKey, addr), sizeof(struct rte_ether_addr), 0,
     "addr"},
    {offsetof(struct StaticKey, device), sizeof(uint16_t), 0, "device"},
};
struct nested_field_descr StaticKey_nests[] = {
    {offsetof(struct StaticKey, addr),
     offsetof(struct rte_ether_addr, addr_bytes), sizeof(uint8_t), 6,
     "addr_bytes"},
};
#endif  // KLEE_VERIFICATION