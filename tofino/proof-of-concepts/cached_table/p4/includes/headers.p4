#ifndef _HEADERS_
#define _HEADERS_

#include "constants.p4"

typedef bit<48> mac_addr_t;

struct empty_header_t {}
struct empty_metadata_t {}

struct ig_meta_t {}

header app_t {
    op_t op;
    key_t key;
    hash_t hash;
    value_t value;
    bool_t hit;
}

header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16> etherType;
}

struct header_t {
    ethernet_t ethernet;
    app_t app;
}

#endif /* _HEADERS_ */
