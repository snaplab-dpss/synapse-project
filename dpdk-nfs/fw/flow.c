#include "flow.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr FlowId_descrs[] = {
    {offsetof(struct FlowId, src_port), sizeof(uint16_t), 0, "src_port"},
    {offsetof(struct FlowId, dst_port), sizeof(uint16_t), 0, "dst_port"},
    {offsetof(struct FlowId, src_ip), sizeof(uint32_t), 0, "src_ip"},
    {offsetof(struct FlowId, dst_ip), sizeof(uint32_t), 0, "dst_ip"},
    {offsetof(struct FlowId, protocol), sizeof(uint8_t), 0, "protocol"},
};
struct nested_field_descr FlowId_nests[] = {};
#endif  // KLEE_VERIFICATION