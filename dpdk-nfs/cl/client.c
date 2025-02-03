#include "client.h"

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr client_descrs[] = {
    {offsetof(struct client, src_ip), sizeof(uint32_t), 0, "src_ip"},
    {offsetof(struct client, dst_ip), sizeof(uint32_t), 0, "dst_ip"},
};
struct nested_field_descr client_nests[] = {};
#endif // KLEE_VERIFICATION