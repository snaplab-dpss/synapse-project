#include "backend.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr LoadBalancedBackend_descrs[] = {
    {offsetof(struct LoadBalancedBackend, nic), sizeof(uint16_t), 0, "nic"},
    {offsetof(struct LoadBalancedBackend, mac), sizeof(struct rte_ether_addr), 0, "mac"},
    {offsetof(struct LoadBalancedBackend, ip), sizeof(uint32_t), 0, "ip"},
};
struct nested_field_descr LoadBalancedBackend_nests[] = {
    {offsetof(struct LoadBalancedBackend, mac), offsetof(struct rte_ether_addr, addr_bytes), sizeof(uint8_t), 6, "addr_bytes"},
};
#else  // KLEE_VERIFICATION
#endif // KLEE_VERIFICATION