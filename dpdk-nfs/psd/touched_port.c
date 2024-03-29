#include "touched_port.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr touched_port_descrs[] = {
    {offsetof(struct TouchedPort, src), sizeof(uint32_t), 0, "src"},
    {offsetof(struct TouchedPort, port), sizeof(uint16_t), 0, "port"},
};
struct nested_field_descr touched_port_nests[] = {};
#endif  // KLEE_VERIFICATION