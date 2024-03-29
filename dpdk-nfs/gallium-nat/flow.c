#include "flow.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr flow_descrs[] = {
    {offsetof(struct Flow, src_addr), sizeof(uint32_t), 0, "src_addr"},
    {offsetof(struct Flow, dst_addr), sizeof(uint32_t), 0, "dst_addr"},
    {offsetof(struct Flow, src_port), sizeof(uint16_t), 0, "src_port"},
    {offsetof(struct Flow, dst_port), sizeof(uint16_t), 0, "dst_port"},
    {offsetof(struct Flow, protocol), sizeof(uint8_t), 0, "protocol"},
};
struct nested_field_descr flow_nests[] = {};
#endif  // KLEE_VERIFICATION