#ifndef _CONSTANTS_
#define _CONSTANTS_

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;

enum bit<8> ip_proto_t {
	TCP = 6,
	UDP = 17
}

enum bit<8> kv_ops_t {
	GET	= 0x00,
	PUT	= 0x01,
}

enum bit<8> cuckoo_ops_t {
	LOOKUP	= 0x00,
	INSERT	= 0x01,
	SWAP	= 0X02,
	SWAPPED = 0x03,
	NOP		= 0x04
}

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;

struct l4_lookup_t {
	bit<16> src_port;
	bit<16> dst_port;
}

// Port configuration.

const PortId_t RECIRC_PORT_0 = 6;
const PortId_t RECIRC_PORT_1 = 128;
const PortId_t RECIRC_PORT_2 = 256;
const PortId_t RECIRC_PORT_3 = 384;

// Front-panel port 1
const PortId_t KVS_SERVER_PORT = 136;

const bit<8> KVS_STATUS_HIT = 1;
const bit<8> KVS_STATUS_MISS = 0;
const bit<8> KVS_STATUS_CUCKOO = 2; // hack

const bit<16> CUCKOO_PORT = 670;

#define KEY_WIDTH			128
#define VAL_WIDTH			128

typedef bit<KEY_WIDTH>		key_t;
typedef bit<VAL_WIDTH>		val_t;

// Entry Timeout Expiration (units of 65536 ns).
// #define ENTRY_TIMEOUT		1525 // 100ms
#define ENTRY_TIMEOUT		15250000 // 1000 seconds

#define MAX_LOOPS			50
// #define MAX_LOOPS			3

// Cuckoo Table Size.
// #define CUCKOO_ENTRIES	8192
// #define CUCKOO_IDX_WIDTH	13
#define CUCKOO_ENTRIES		2
#define CUCKOO_IDX_WIDTH	1

// Swap Bloom Table Size.
#define BLOOM_ENTRIES		8192
#define BLOOM_IDX_WIDTH		13

#endif
