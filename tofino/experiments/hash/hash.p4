#include <core.p4>

#if __TARGET_TOFINO__ == 2
  #include <t2na.p4>
  #define CPU_PCIE_PORT 0
  #define RECIRCULATION_PORT 6
#else
  #include <tna.p4>
  #define CPU_PCIE_PORT 192
  #define RECIRCULATION_PORT 320
#endif

typedef bit<9> port_t;
typedef bit<7> port_pad_t;
typedef bit<32> time_t;

const bit<16> ETHERTYPE_IPV4 = 0x800;
const bit<8>  IP_PROTO_TCP   = 6;
const bit<8>  IP_PROTO_UDP   = 17;

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

header udp_t {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> len;
	bit<16> checksum;
}

header app_t {
    bit<32> hash;
    bit<32> hash0;
    bit<32> hash1;
    bit<32> hash2;
}

struct headers_t {
	ethernet_t ethernet;
	ipv4_t ipv4;
	udp_t udp;
    app_t app;
}

struct metadata_t {}

parser TofinoIngressParser(
	packet_in pkt,
	out headers_t hdr,
	out ingress_intrinsic_metadata_t ig_intr_md
) {
	state start {
		pkt.extract(ig_intr_md);
		transition select(ig_intr_md.resubmit_flag) {
			0: parse_port_metadata;
			1: parse_resubmit;
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
	out headers_t hdr,
	out metadata_t meta,
	out ingress_intrinsic_metadata_t ig_intr_md
) {
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, hdr, ig_intr_md);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			ETHERTYPE_IPV4: parse_ipv4;
			default: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select (hdr.ipv4.protocol) {
			IP_PROTO_UDP: parse_udp;
			default: reject;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition accept;
	}
}

control Ingress(
	inout headers_t hdr,
	inout metadata_t meta,
	in ingress_intrinsic_metadata_t ig_intr_md,
	in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
	inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
	inout ingress_intrinsic_metadata_for_tm_t ig_tm_md
) {
	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}
	
	action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

    Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_calculator;
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash0_calculator;
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash1_calculator;
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash2_calculator;


	const bit<32> HASH_SALT_0 = 0xfbc31fc7;
	const bit<32> HASH_SALT_1 = 0x2681580b;
	const bit<32> HASH_SALT_2 = 0x486d7e2f;

	action calc_hash() {
		hdr.app.hash = hash_calculator.get({
			hdr.ipv4.src_addr,
			hdr.ipv4.dst_addr,
			hdr.udp.src_port,
			hdr.udp.dst_port
		});
	}

	action calc_hash0() {
		hdr.app.hash0 = hash0_calculator.get({
			hdr.ipv4.src_addr,
			hdr.ipv4.dst_addr,
			hdr.udp.src_port,
			hdr.udp.dst_port,
			HASH_SALT_0
		});
	}

	action calc_hash1() {
		hdr.app.hash1 = hash1_calculator.get({
			hdr.ipv4.src_addr,
			hdr.ipv4.dst_addr,
			hdr.udp.src_port,
			hdr.udp.dst_port,
			HASH_SALT_1
		});
	}

	action calc_hash2() {
		hdr.app.hash2 = hash2_calculator.get({
			hdr.ipv4.src_addr,
			hdr.ipv4.dst_addr,
			hdr.udp.src_port,
			hdr.udp.dst_port,
			HASH_SALT_2
		});
	}

	apply {
        hdr.app.setValid();
        
		calc_hash();
		calc_hash0();
		calc_hash1();
		calc_hash2();

        forward(CPU_PCIE_PORT);
	}
}

control IngressDeparser(
	packet_out pkt,
	inout headers_t hdr,
	in metadata_t meta,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
	apply {
		pkt.emit(hdr);
	}
}

parser TofinoEgressParser(
	packet_in pkt,
	out egress_intrinsic_metadata_t eg_intr_md
) {
	state start {
		pkt.extract(eg_intr_md);
		transition accept;
	}
}

parser EgressParser(
	packet_in pkt,
	out headers_t hdr,
	out metadata_t meta,
	out egress_intrinsic_metadata_t eg_intr_md
) {
	TofinoEgressParser() tofino_parser;

	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, eg_intr_md);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			ETHERTYPE_IPV4: parse_ipv4;
			default: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select (hdr.ipv4.protocol) {
			IP_PROTO_UDP: parse_udp;
			default: reject;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition accept;
	}
}

control Egress(
	inout headers_t hdr,
	inout metadata_t meta,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
	inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
	apply {}
}

control EgressDeparser(
	packet_out pkt,
	inout headers_t hdr,
	in metadata_t meta,
	in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md
) {
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