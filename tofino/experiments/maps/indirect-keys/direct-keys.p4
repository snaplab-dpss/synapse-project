/* -* P4_16 -*- */
#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif
typedef bit<9>  port_t;
typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> ether_type_t;
typedef bit<8>  ip_protocol_t;

const port_t RECIRCULATING_PORT = 68;

// Tofino model
const PortId_t CPU_PCIE_PORT = 64;

// BFN-T10-032D
// BFN-T10-032D-024
// BFN-T10-032D-020
// BFN-T10-032D-018
// const PortId_t CPU_PCIE_PORT = 192;

// BFN-T10-064Q
// BFN-T10-032Q
// const PortId_t CPU_PCIE_PORT = 320;

const port_t LAN = 0;
const port_t WAN = 1;

const ether_type_t  ETHERTYPE_IPV4     = 16w0x0800;
const ether_type_t  ETHERTYPE_ARP      = 16w0x0806;
const ether_type_t  ETHERTYPE_IPV6     = 16w0x86dd;
const ether_type_t  ETHERTYPE_VLAN     = 16w0x8100;

const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;

header ethernet_h {
	mac_addr_t   dst_addr;
	mac_addr_t   src_addr;
	ether_type_t ether_type;
}

header ipv4_h {
	bit<4>        version;
	bit<4>        ihl;
	bit<8>        dscp;
	bit<16>       total_len;
	bit<16>       identification;
	bit<3>        flags;
	bit<13>       frag_offset;
	bit<8>        ttl;
	ip_protocol_t protocol;
	bit<16>       hdr_checksum;
	ipv4_addr_t   src_addr;
	ipv4_addr_t   dst_addr;
}

header tcpudp_h {
	bit<16> src_port;
	bit<16> dst_port;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
	ethernet_h ethernet;
	ipv4_h     ipv4;
	tcpudp_h   tcpudp;
}

parser TofinoIngressParser(
		packet_in pkt,
		out ingress_intrinsic_metadata_t ig_intr_md) {
	state start {
		pkt.extract(ig_intr_md);
		transition select(ig_intr_md.resubmit_flag) {
			1 : parse_resubmit;
			0 : parse_port_metadata;
		}
	}

	state parse_resubmit {
		// Parse resubmitted packet here.
		transition reject;
	}

	state parse_port_metadata {
		pkt.advance(PORT_METADATA_SIZE);
		transition accept;
	}
}

parser IngressParser(
	packet_in pkt,

	/* User */    
	out my_ingress_headers_t  hdr,
	out my_ingress_metadata_t meta,

	/* Intrinsic */
	out ingress_intrinsic_metadata_t  ig_intr_md)
{
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, ig_intr_md);

		transition select(ig_intr_md.ingress_port) {
			default: parse_ethernet;
		}
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);

		transition select(hdr.ethernet.ether_type) {
			ETHERTYPE_IPV4: parse_ipv4;
			default: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);

		// We only care about TCP packets
		transition select (hdr.ipv4.protocol) {
			IP_PROTOCOLS_TCP: parse_tcpudp;
			IP_PROTOCOLS_UDP: parse_tcpudp;
			default: reject;
		}
	}

	state parse_tcpudp {
		pkt.extract(hdr.tcpudp);
		transition accept;
	}
}

control Ingress(
	/* User */
	inout my_ingress_headers_t  hdr,
	inout my_ingress_metadata_t meta,

	/* Intrinsic */
	in    ingress_intrinsic_metadata_t               ig_intr_md,
	in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
	inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
	inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
	bit<8> key_byte_0;
	bit<8> key_byte_1;
	bit<8> key_byte_2;
	bit<8> key_byte_3;
	bit<8> key_byte_4;
	bit<8> key_byte_5;
	bit<8> key_byte_6;
	bit<8> key_byte_7;
	bit<8> key_byte_8;
	bit<8> key_byte_9;
	bit<8> key_byte_10;
	bit<8> key_byte_11;

	bit<32> map_value;

	action fwd(port_t port){
		ig_tm_md.ucast_egress_port = port;
	}

	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}

	action send_to_cpu(){
		fwd(CPU_PCIE_PORT);
	}

	table map1 {
		key = {
			hdr.ipv4.src_addr: exact;
			hdr.ipv4.dst_addr: exact;
			hdr.tcpudp.src_port: exact;
			hdr.tcpudp.dst_port: exact;
		}

		actions = {
			NoAction;
		}

		size = 65536;
		idle_timeout = true;
	}

	table map2 {
		key = {
			hdr.ipv4.dst_addr: exact;
			hdr.ipv4.src_addr: exact;
			hdr.tcpudp.dst_port: exact;
			hdr.tcpudp.src_port: exact;
		}

		actions = {
			NoAction;
		}

		size = 65536;
		idle_timeout = true;
	}

	apply {
		if (ig_intr_md.ingress_port != WAN) {
			bool map_hit = map1.apply().hit;

			if (!map_hit) {
				drop();
			} else {
				fwd(2);
			}
		} else {
			bool map_hit = map2.apply().hit;

			if (!map_hit) {
				drop();
			} else {
				fwd(2);
			}
		}
	}
}

control IngressDeparser(
	packet_out pkt,

	/* User */
	inout my_ingress_headers_t  hdr,
	in    my_ingress_metadata_t meta,

	/* Intrinsic */
	in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
	apply {}
}

parser EgressParser(
		packet_in pkt,
		out empty_header_t hdr,
		out empty_metadata_t eg_md,
		out egress_intrinsic_metadata_t eg_intr_md) {
	state start {
		transition accept;
	}
}

control EgressDeparser(
		packet_out pkt,
		inout empty_header_t hdr,
		in empty_metadata_t eg_md,
		in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
	apply {}
}

control Egress(
		inout empty_header_t hdr,
		inout empty_metadata_t eg_md,
		in egress_intrinsic_metadata_t eg_intr_md,
		in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
		inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
		inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
	apply {}
}

Pipeline(
	IngressParser(),
	Ingress(),
	IngressDeparser(),
	EgressParser(),
	Egress(),
	EgressDeparser()
) pipe;

Switch(pipe) main;
