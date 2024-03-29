#include "flow.h"

#ifdef KLEE_VERIFICATION
struct str_field_descr flow_descrs[] = {
    {offsetof(struct Flow, src_addr), sizeof(uint32_t), 0, "src_addr"},
    {offsetof(struct Flow, dst_addr), sizeof(uint32_t), 0, "dst_addr"},
    {offsetof(struct Flow, src_port), sizeof(uint16_t), 0, "src_port"},
    {offsetof(struct Flow, dst_port), sizeof(uint16_t), 0, "dst_port"},
    {offsetof(struct Flow, device), sizeof(uint16_t), 0, "device"},
    {offsetof(struct Flow, proto), sizeof(uint8_t), 0, "proto"},
};
struct nested_field_descr flow_nests[] = {};
#endif  // KLEE_VERIFICATION