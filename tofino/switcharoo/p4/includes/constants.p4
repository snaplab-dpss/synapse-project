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

const bit<16> CUCKOO_PORT = 670;

#define KEY_WIDTH			128
#define VAL_WIDTH			128

typedef bit<KEY_WIDTH>		key_t;
typedef bit<VAL_WIDTH>		val_t;

// Entry Timeout Expiration (ns).
#define ENTRY_TIMEOUT		50000

#define MAX_LOOPS			20

// Cuckoo Table Size.
#define CUCKOO_ENTRIES		8192
#define CUCKOO_IDX_WIDTH	13

// Swap Bloom Table Size.
#define BLOOM_ENTRIES		8192
#define BLOOM_IDX_WIDTH		13

#endif
