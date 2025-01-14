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

#define RECIRCULATION_PORT 6

// hardware
// #define IN_PORT 136
// #define OUT_PORT 144

// model
#define IN_PORT 8
#define OUT_PORT 9
#else
// hardware
// #define CPU_PCIE_PORT 192
// #define IN_PORT 164
// #define OUT_PORT 172

// model
#define CPU_PCIE_PORT 320
#define IN_PORT 0
#define OUT_PORT 1
#define RECIRCULATION_PORT 68
#endif

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

const bit<16> TYPE_IPV4 = 0x800;

const bit<8> IP_PROTOCOLS_TCP = 6;
const bit<8> IP_PROTOCOLS_UDP = 17;

header ethernet_t {
	bit<48> dstAddr;
	bit<48> srcAddr;
	bit<16> etherType;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {
}

struct my_ingress_headers_t {
	ethernet_t ethernet;
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
	out ingress_intrinsic_metadata_t  ig_intr_md
) {
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, ig_intr_md);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
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
    action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

    bit<32> field0 = 0;
    bit<32> field1 = 0;
    bit<32> field2 = 0;
    bit<32> field3 = 0;
    bit<32> field4 = 0;
    bit<32> field5 = 0;
    bit<32> field6 = 0;
    bit<32> field7 = 0;
    bit<32> field8 = 0;

    action populate(
        bit<32> _field0,
        bit<32> _field1,
        bit<32> _field2,
        bit<32> _field3,
        bit<32> _field4,
        bit<32> _field5,
        bit<32> _field6,
        bit<32> _field7,
        bit<32> _field8
    ) {
        field0 = _field0;
        field1 = _field1;
        field2 = _field2;
        field3 = _field3;
        field4 = _field4;
        field5 = _field5;
        field6 = _field6;
        field7 = _field7;
        field8 = _field8;
    }

	table t0 {
        key = {
            hdr.ethernet.etherType: exact;
        }

        actions = {
            populate;
        }
    }

    bit<256> super_field = 256w0;

    table t1 {
        key = {
            super_field: exact;
        }

        actions = {
            forward;
        }
    }


	apply {
        t0.apply();

        // Option 0
        // super_field[31:0] = field0;
        // super_field[63:32] = field1;
        // super_field[95:64] = field2;
        // super_field[127:96] = field3;
        // super_field[159:128] = field4;
        // super_field[191:160] = field5;
        // super_field[223:192] = field6;
        // super_field[255:224] = field7;

        // Option 1
        super_field = field7 ++ field6 ++ field5 ++ field4 ++ field3 ++ field2 ++ field1 ++ field0;

        t1.apply();
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
