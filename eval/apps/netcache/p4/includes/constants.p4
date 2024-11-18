#ifndef _CONSTANTS_
#define _CONSTANTS_

#define MAX_PORTS 255

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;

typedef bit<8> ip_proto_t;
const ip_proto_t IP_PROTO_ICMP = 1;
const ip_proto_t IP_PROTO_IPV4 = 4;
const ip_proto_t IP_PROTO_TCP = 6;
const ip_proto_t IP_PROTO_UDP = 17;

typedef bit<9> port_t;

const port_t CPU_PORT = 255;

// NetCache

const bit<16> NC_PORT = 50000;

// Size of a key field
#define NC_KEY_WIDTH				16
// Size of the key index (used in the cache counter)
#define NC_KEY_IDX_WIDTH			16
// Size of a value field
#define NC_VAL_WIDTH				32
// Size of the vtable index (used to access the vtable register)
#define NC_VTABLE_SIZE_WIDTH		16
// Number of entries in the vtable register
#define NC_ENTRIES					65536
// Number of entries in each of the CM registers
#define SKETCH_ENTRIES				16384
// Number of entries in each of the bloom filter registers
#define BLOOM_ENTRIES				262144
// Heavy hitter threshold
#define HH_THRES					128

typedef bit<NC_KEY_WIDTH>			key_t;
typedef bit<NC_KEY_IDX_WIDTH>		keyIdx_t;
typedef bit<NC_VAL_WIDTH>			val_t;
typedef bit<NC_VTABLE_SIZE_WIDTH>	vtableIdx_t;

const bit<8> READ_QUERY = 0x00;
const bit<8> WRITE_QUERY = 0x01;
const bit<8> DELETE_QUERY = 0x02;
const bit<8> HOT_READ_QUERY = 0x03;

#endif
