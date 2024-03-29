#include "flow.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr flow_descrs[] = {
    {offsetof(struct flow, src_port), sizeof(uint16_t), 0, "src_port"},
    {offsetof(struct flow, dst_port), sizeof(uint16_t), 0, "dst_port"},
    {offsetof(struct flow, src_ip), sizeof(uint32_t), 0, "src_ip"},
    {offsetof(struct flow, dst_ip), sizeof(uint32_t), 0, "dst_ip"},
    {offsetof(struct flow, protocol), sizeof(uint8_t), 0, "protocol"},
};
struct nested_field_descr flow_nests[] = {};
#endif  // KLEE_VERIFICATION