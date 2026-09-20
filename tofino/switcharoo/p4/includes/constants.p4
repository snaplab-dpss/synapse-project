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
	DONE	= 0x04
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

const bit<8> KVS_STATUS_HIT		= 1;
const bit<8> KVS_STATUS_MISS	= 0;
const bit<8> KVS_STATUS_CUCKOO	= 2; // hack

const bit<16> CUCKOO_PORT = 670;

// Table 2's CRC-32 (entry 1 of the polynomial bank in dpdk-nfs/lib/util/crc32.h; a primitive
// polynomial, coprime with table 1's built-in CRC-32). The two tables MUST hash with different
// polynomials: a CRC is affine over GF(2), so one CRC with two salts only yields h2 = h1 ^ const,
// which makes the two tables two ways of one bucket instead of two independent homes, and a key
// displaced from table 1 can never be placed anywhere its lookup would not already have probed.
#define TABLE_2_CRC_POLY	coeff = 0x7B17A39F, reversed = true, msb = false, extended = false, init = 0xFFFFFFFF, xor = 0xFFFFFFFF

#define KEY_WIDTH			32
#define VAL_WIDTH			32

typedef bit<KEY_WIDTH>		key_t;
typedef bit<VAL_WIDTH>		val_t;

// Entry Timeout Expiration (units of 65536 ns).
#define ENTRY_TIMEOUT		16384 // 1 s

#define MAX_LOOPS			4

// Cuckoo Table Size.
#define CUCKOO_ENTRIES		4096
#define CUCKOO_IDX_WIDTH	12

// Swap Bloom Table Size.
#define BLOOM_ENTRIES		65536
#define BLOOM_IDX_WIDTH		16

#endif
