#include "constants.p4"

#ifndef _HEADERS_
#define _HEADERS_

header ethernet_t {
	bit<48> dst_addr;
	bit<48> src_addr;
	bit<16> ether_type;
}

header ipv4_t {
	bit<4>	version;
	bit<4>	ihl;
	bit<8>	diffserv;
	bit<16> len;
	bit<16> identification;
	bit<3>	flags;
	bit<13> frag_offset;
	bit<8>	ttl;
	bit<8>	protocol;
	bit<16> hdr_checksum;
	bit<32> src_addr;
	bit<32> dst_addr;
}

header tcp_t {
	bit<16> src_port;
	bit<16> dst_port;
	bit<32> seq_no;
	bit<32> ack_no;
	bit<4>	data_offset;
	bit<3>	res;
	bit<3>	ecn;
	bit<6>	ctrl;
	bit<16> window;
	bit<16> checksum;
	bit<16> urgent_ptr;
}

header udp_t {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> length_;
	bit<16> checksum;
}

header netcache_t {
	bit<8>	op;
	key_t	key;
	val_t	val;
	bit<8>	status;
	bit<16>	port;
}

struct ingress_metadata_t {
	bit<16>	original_nc_port;
	bit<8> is_client_packet;
	bit<16> key_idx;
	bit<8> cache_hit;
}

struct egress_metadata_t {}

struct header_t {
	ethernet_t	ethernet;
	ipv4_t		ipv4;
	tcp_t		tcp;
	udp_t		udp;
	netcache_t	netcache;
}

#endif
