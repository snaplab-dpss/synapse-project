#include "session.h"

#include <stdint.h>

#ifdef KLEE_VERIFICATION
struct str_field_descr session_descrs[] = {
    {offsetof(struct session, client_ip), sizeof(uint32_t), 0, "client_ip"},
    {offsetof(struct session, server_ip), sizeof(uint32_t), 0, "server_ip"},
};
struct nested_field_descr session_nests[] = {};
#endif // KLEE_VERIFICATION
