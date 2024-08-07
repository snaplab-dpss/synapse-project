#ifndef _HEADERS_
#define _HEADERS_

#include "constants.p4"

header cpu_h {
	bit<16> code_path;
	
	@padding port_pad_t pad0;
	port_t in_port;

	@padding port_pad_t pad1;
	port_t out_port;
}

header ethernet_t {
	bit<48> dstAddr;
	bit<48> srcAddr;
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
	bit<32> src_addr;
	bit<32> dst_addr;
}

header tcpudp_t {
	bit<16> src_port;
	bit<16> dst_port;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
	cpu_h cpu;
	ethernet_t ethernet;
	ipv4_t ipv4;
	tcpudp_t tcpudp;
}

#endif /* _HEADERS_ */
