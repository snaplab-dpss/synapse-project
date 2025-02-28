#ifndef _CONSTANTS_
#define _CONSTANTS_

#if __TARGET_TOFINO__ == 2

#define CPU_PCIE_PORT 0
#define ETH_CPU_PORT_0 2
#define ETH_CPU_PORT_1 3
#define ETH_CPU_PORT_2 4
#define ETH_CPU_PORT_3 5
#define RECIRCULATION_PORT 6

#else

#define CPU_PCIE_PORT 320
#define RECIRCULATION_PORT 68

#endif

#ifndef CACHE_ACTIVATED
#define CACHE_ACTIVATED 1
#endif

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;

typedef bit<8> ip_proto_t;
const ip_proto_t IP_PROTO_ICMP = 1;
const ip_proto_t IP_PROTO_IPV4 = 4;
const ip_proto_t IP_PROTO_TCP = 6;
const ip_proto_t IP_PROTO_UDP = 17;

typedef bit<9> port_t;

typedef bit<8>  pkt_type_t;
const pkt_type_t PKT_TYPE_NORMAL = 1;
const pkt_type_t PKT_TYPE_MIRROR = 2;

typedef bit<4> mirror_type_t;
const mirror_type_t MIRROR_TYPE_E2E = 2;

// NetCache

const bit<16> NC_PORT = 670;

// Size of a key field
#define NC_KEY_WIDTH				128
// Size of a value field
#define NC_VAL_WIDTH				1024
// Size of the vtable index (used to access the vtable register)
#define NC_VTABLE_SIZE_WIDTH		16
// Number of entries in the vtable register
#define NC_ENTRIES					8192
// Size of the key index used in the cache counter
#define NC_KEY_IDX_WIDTH			16
// Number of entries in each of the CM registers
#define SKETCH_ENTRIES				1024
// Hash size (bits) for the CM registers
#define SKETCH_IDX_WIDTH			10
// Number of entries in each of the bloom filter registers
#define BLOOM_ENTRIES				1024
// Hash size (bits) for the bloom filter registers
#define BLOOM_IDX_WIDTH				10
// Heavy hitter threshold
#define HH_THRES					128

typedef bit<NC_KEY_WIDTH>			key_t;
typedef bit<NC_KEY_IDX_WIDTH>		keyIdx_t;
typedef bit<NC_VAL_WIDTH>			val_t;
typedef bit<NC_VTABLE_SIZE_WIDTH>	vtableIdx_t;

const bit<8> READ_QUERY = 0x00;
const bit<8> WRITE_QUERY = 0x01;
const bit<8> DELETE_QUERY = 0x02;

#endif
