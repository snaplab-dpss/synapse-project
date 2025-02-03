#include "lb_flow.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr LoadBalancedFlow_descrs[] = {
    {offsetof(struct LoadBalancedFlow, src_ip), sizeof(uint32_t), 0, "src_ip"},
    {offsetof(struct LoadBalancedFlow, dst_ip), sizeof(uint32_t), 0, "dst_ip"},
    {offsetof(struct LoadBalancedFlow, src_port), sizeof(uint16_t), 0, "src_port"},
    {offsetof(struct LoadBalancedFlow, dst_port), sizeof(uint16_t), 0, "dst_port"},
    {offsetof(struct LoadBalancedFlow, protocol), sizeof(uint8_t), 0, "protocol"},
};
struct nested_field_descr LoadBalancedFlow_nests[] = {};
#else // KLEE_VERIFICATION

#endif // KLEE_VERIFICATION
