#ifndef _HEADERS_
#define _HEADERS_

#include "constants.p4"

typedef bit<9> dev_port_t;

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;

const bit<16> TYPE_IPV4        = 0x800;
const bit<8>  IP_PROTOCOLS_TCP =   6;
const bit<8>  IP_PROTOCOLS_UDP =  17;

struct empty_header_t {}
struct empty_metadata_t {}

struct ig_meta_t {
    hash_t hash0;
    hash_t hash1;
    hash_t hash2;
    entry_t entry;
    bit<8> found;
}

header cpu_t {
    entry_t entry;
    bit<8> found;
}

header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

struct header_t {
    cpu_t cpu;
    ethernet_t ethernet;
    ipv4_t ipv4;
}

#endif /* _HEADERS_ */
