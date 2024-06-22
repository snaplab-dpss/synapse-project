#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#if __TARGET_TOFINO__ == 2
#define CPU_PCIE_PORT 0

#define ETH_CPU_PORT_0 2
#define ETH_CPU_PORT_1 3
#define ETH_CPU_PORT_2 4
#define ETH_CPU_PORT_3 5

// #define RECIRCULATION_PORT 6
#define RECIRCULATION_PORT 128

// hardware
#define IN_PORT 136
#define OUT_PORT 144

// model
// #define IN_PORT 8
// #define OUT_PORT 9
#else
// hardware
#define CPU_PCIE_PORT 192
#define IN_PORT 164
#define OUT_PORT 172

// model
// #define CPU_PCIE_PORT 320
// #define IN_PORT 0
// #define OUT_PORT 1
#endif

#define CAPACITY 16

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

const bit<16> TYPE_IPV4 = 0x800;

const bit<8> IP_PROTOCOLS_TCP = 6;
const bit<8> IP_PROTOCOLS_UDP = 17;


header recirc_h {
	bit<16> times;
}

header ethernet_t {
	bit<48> dstAddr;
	bit<48> srcAddr;
	bit<16> etherType;
}

header ipv4_t {
	bit<8>  version;
	bit<8>  ihl;
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
	recirc_h recirc;
	ethernet_t ethernet;
	ipv4_t ipv4;
	tcpudp_t tcpudp;
}

parser TofinoIngressParser(
	packet_in pkt,
	out ingress_intrinsic_metadata_t ig_intr_md
) {
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
	out ingress_intrinsic_metadata_t  ig_intr_md
) {
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, ig_intr_md);

		transition select(ig_intr_md.ingress_port) {
			RECIRCULATION_PORT: parse_recirc;
			default: parse_ethernet;
		}
	}

	state parse_recirc {
		pkt.extract(hdr.recirc);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			TYPE_IPV4: parse_ipv4;
			default: accept;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select(hdr.ipv4.protocol) {
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
	inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md
) {
	bit<8> rs0_counter_value;
	bit<8> rs1_counter_value;

	bool should_recirculate;

	Register<bit<8>, _>(1, 0) rs0_counter;

	RegisterAction<bit<8>, bit<32>, bit<8>>(rs0_counter) read_and_inc_rs0_counter_action = {
		void apply(inout bit<8> curr_value, out bit<8> out_value) {
			out_value = curr_value;

			if (curr_value == CAPACITY - 1) {
				curr_value = 0;
			} else {
				curr_value = curr_value + 1;
			}
		}
	};

	action read_and_inc_rs0_counter() {
		rs0_counter_value = read_and_inc_rs0_counter_action.execute(0);
	}

	Register<bit<8>, _>(1, 0) rs1_counter;

	RegisterAction<bit<8>, bit<32>, bit<8>>(rs1_counter) read_and_inc_rs1_counter_action = {
		void apply(inout bit<8> curr_value, out bit<8> out_value) {
			out_value = curr_value;

			if (curr_value == CAPACITY - 1) {
				curr_value = 0;
			} else {
				curr_value = curr_value + 1;
			}
		}
	};

	action read_and_inc_rs1_counter() {
		rs1_counter_value = read_and_inc_rs1_counter_action.execute(0);
	}

	action set_recirc_hdr() {
		hdr.recirc.setValid();
		hdr.recirc.times = 1;
	}

	action update_recirc_hdr() {
		hdr.recirc.times = hdr.recirc.times + 1;
	}

	action forward() {
		hdr.recirc.setInvalid();
		ig_tm_md.ucast_egress_port = OUT_PORT;
	}

	action recirculate() {
		ig_tm_md.ucast_egress_port = RECIRCULATION_PORT;
	}

	action recirc() {
		should_recirculate = true;
	}

	action no_recirc() {
		should_recirculate = false;
	}

	table rs0_table {
		key = {
			rs0_counter_value: exact;
		}

		actions = {
			recirc;
			no_recirc;
		}

		size = CAPACITY;
	}

	table rs1_table {
		key = {
			rs1_counter_value: exact;
		}

		actions = {
			recirc;
			no_recirc;
		}

		size = CAPACITY;
	}

	apply {
		if (ig_intr_md.ingress_port != RECIRCULATION_PORT) {
			set_recirc_hdr();
			recirculate();
		} else if (hdr.recirc.times == 1) {
			read_and_inc_rs0_counter();
			rs0_table.apply();
			if (should_recirculate) {
				update_recirc_hdr();
				recirculate();
			} else {
				forward();
			}
		} else if (hdr.recirc.times == 2) {
			read_and_inc_rs1_counter();
			rs1_table.apply();
			if (should_recirculate) {
				update_recirc_hdr();
				recirculate();
			} else {
				forward();
			}
		} else {
			forward();
		}

		ig_tm_md.bypass_egress = 1;
	}
}

control IngressDeparser(
	packet_out pkt,

	/* User */
	inout my_ingress_headers_t  hdr,
	in    my_ingress_metadata_t meta,

	/* Intrinsic */
	in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md
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
    out empty_header_t hdr,
    out empty_metadata_t eg_md,
    out egress_intrinsic_metadata_t eg_intr_md
) {
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
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
  apply {}
}

control EgressDeparser(
    packet_out pkt,
    inout empty_header_t hdr,
    in empty_metadata_t eg_md,
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
