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

// BFN-T10-032D
// BFN-T10-032D-024
// BFN-T10-032D-020
// BFN-T10-032D-018
// const port_t CPU_PCIE_PORT = 192;

// BFN-T10-064Q
// BFN-T10-032Q
// PCIe port when using the tofino model
const port_t CPU_PCIE_PORT = 320;

const port_t LAN = 0;
const port_t WAN = 1;

const ether_type_t  ETHERTYPE_IPV4     = 16w0x0800;
const ether_type_t  ETHERTYPE_ARP      = 16w0x0806;
const ether_type_t  ETHERTYPE_IPV6     = 16w0x86dd;
const ether_type_t  ETHERTYPE_VLAN     = 16w0x8100;

const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;

header cpu_h {
	bit<16> code_path;
	bit<7> pad;
	port_t out_port;
}

header ethernet_h {
	bit<112> data;
}

header ipv4_h {
	bit<160> data;
}

header ipv4_options_h {
	bit<32> data;
}

typedef ipv4_options_h[10] ipv4_options_t;

header tcpudp_h {
	bit<32> data;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
	cpu_h          cpu;
	ethernet_h     ethernet;
	ipv4_h         ipv4;
	ipv4_options_t ipv4_options;
	tcpudp_h       tcpudp;
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
			CPU_PCIE_PORT: parse_cpu;
			default: parse_ethernet;
		}
	}

	state parse_cpu {
		pkt.extract(hdr.cpu);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);

		// hdr.ethernet.data[111:96] := ethertype
		transition select(hdr.ethernet.data[111:96]) {
			ETHERTYPE_IPV4: parse_ipv4;
			default: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);

		// hdr.ipv4.data[7:4] := ihl
		transition select (hdr.ipv4.data[7:4]) {
			0x5: parse_ipv4_no_options; 
			0x6: parse_ipv4_options_1;
			0x7: parse_ipv4_options_2;
			0x8: parse_ipv4_options_3;
			0x9: parse_ipv4_options_4;
			0xa: parse_ipv4_options_5;
			0xb: parse_ipv4_options_6;
			0xc: parse_ipv4_options_7;
			0xd: parse_ipv4_options_8;
			0xe: parse_ipv4_options_9;
			0xf: parse_ipv4_options_10;
			/* Omit the default case to drop packets 0..4 */
		}
	}

	state parse_ipv4_options_1 {
		pkt.extract(hdr.ipv4_options[0]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_2 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_3 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_4 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		pkt.extract(hdr.ipv4_options[3]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_5 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		pkt.extract(hdr.ipv4_options[3]);
		pkt.extract(hdr.ipv4_options[4]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_6 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		pkt.extract(hdr.ipv4_options[3]);
		pkt.extract(hdr.ipv4_options[4]);
		pkt.extract(hdr.ipv4_options[5]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_7 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		pkt.extract(hdr.ipv4_options[3]);
		pkt.extract(hdr.ipv4_options[4]);
		pkt.extract(hdr.ipv4_options[5]);
		pkt.extract(hdr.ipv4_options[6]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_8 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		pkt.extract(hdr.ipv4_options[3]);
		pkt.extract(hdr.ipv4_options[4]);
		pkt.extract(hdr.ipv4_options[5]);
		pkt.extract(hdr.ipv4_options[6]);
		pkt.extract(hdr.ipv4_options[7]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_9 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		pkt.extract(hdr.ipv4_options[3]);
		pkt.extract(hdr.ipv4_options[4]);
		pkt.extract(hdr.ipv4_options[5]);
		pkt.extract(hdr.ipv4_options[6]);
		pkt.extract(hdr.ipv4_options[7]);
		pkt.extract(hdr.ipv4_options[8]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_options_10 {
		pkt.extract(hdr.ipv4_options[0]);
		pkt.extract(hdr.ipv4_options[1]);
		pkt.extract(hdr.ipv4_options[2]);
		pkt.extract(hdr.ipv4_options[3]);
		pkt.extract(hdr.ipv4_options[4]);
		pkt.extract(hdr.ipv4_options[5]);
		pkt.extract(hdr.ipv4_options[6]);
		pkt.extract(hdr.ipv4_options[7]);
		pkt.extract(hdr.ipv4_options[8]);
		pkt.extract(hdr.ipv4_options[9]);
		transition parse_ipv4_no_options;
	}

	state parse_ipv4_no_options {
		// hdr.ipv4.data[79:72] := protocol
		transition select (hdr.ipv4.data[79:72]) {
			IP_PROTOCOLS_TCP: parse_tcpudp;
			IP_PROTOCOLS_UDP: parse_tcpudp;
			default: accept;
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
	action fwd(port_t port){
		ig_tm_md.ucast_egress_port = port;
	}

	apply {
		// 1x 8b
		// hdr.ethernet.data[7:0] = 8w01;

		// 1x 16b
		// hdr.ethernet.data[15:0] = 16w0201;

		// 1x 32b
		// hdr.ethernet.data[31:0] = 32w04030201;

		// 1x 64b
		// hdr.ethernet.data[63:0] = 64w0807060504030201;

		// 2x 8b
		hdr.ethernet.data[7:0]   = 8w01;
		hdr.ethernet.data[15:8]  = 8w02;

		// 4x 8b
		// hdr.ethernet.data[7:0]   = 8w01;
		// hdr.ethernet.data[15:8]  = 8w02;
		// hdr.ethernet.data[23:16] = 8w03;
		// hdr.ethernet.data[31:24] = 8w04;

		// 12x 8b
		// hdr.ethernet.data[7:0]   = 8w01;
		// hdr.ethernet.data[15:8]  = 8w02;
		// hdr.ethernet.data[23:16] = 8w03;
		// hdr.ethernet.data[31:24] = 8w04;
		// hdr.ethernet.data[39:32] = 8w05;
		// hdr.ethernet.data[47:40] = 8w06;
		// hdr.ethernet.data[55:48] = 8w07;
		// hdr.ethernet.data[63:56] = 8w08;
		// hdr.ethernet.data[71:64] = 8w09;
		// hdr.ethernet.data[79:72] = 8w10;
		// hdr.ethernet.data[87:80] = 8w11;
		// hdr.ethernet.data[95:88] = 8w12;

		fwd(1);
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
	apply {
		pkt.emit(hdr);
	}
}

parser TofinoEgressParser(
    packet_in pkt,
    out egress_intrinsic_metadata_t eg_intr_md)
{
  state start {
    pkt.extract(eg_intr_md);
    transition accept;
  }
}

parser EgressParser(
    packet_in pkt,
    out empty_header_t hdr,
    out empty_metadata_t eg_md,
    out egress_intrinsic_metadata_t eg_intr_md)
{
  TofinoEgressParser() tofino_parser;

  /* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, eg_intr_md);
    	transition accept;
	}
}

control Egress(
    inout empty_header_t hdr,
    inout empty_metadata_t eg_md,
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
    inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md)
{
  apply {}
}

control EgressDeparser(
    packet_out pkt,
    inout empty_header_t hdr,
    in empty_metadata_t eg_md,
    in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md)
{
  apply {
    pkt.emit(hdr);
  }
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
