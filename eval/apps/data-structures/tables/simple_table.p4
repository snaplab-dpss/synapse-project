#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

typedef bit<9>  port_t;

// hardware
const port_t CPU_PCIE_PORT = 192;

// model
// const port_t CPU_PCIE_PORT = 320;

const port_t IN_PORT = 0;
const port_t OUT_PORT = 1;

const port_t IN_DEV_PORT = 164;
const port_t OUT_DEV_PORT = 172;

header hdr0_h {
  bit<8> b0;
  bit<8> b1;
  bit<8> b2;
  bit<8> b3;
  bit<8> b4;
  bit<8> b5;
  bit<8> b6;
  bit<8> b7;
  bit<8> b8;
  bit<8> b9;
  bit<8> b10;
  bit<8> b11;
  bit<8> b12;
  bit<8> b13;
}

header hdr1_h {
  bit<8> b0;
  bit<8> b1;
  bit<8> b2;
  bit<8> b3;
  bit<8> b4;
  bit<8> b5;
  bit<8> b6;
  bit<8> b7;
  bit<8> b8;
  bit<8> b9;
  bit<8> b10;
  bit<8> b11;
  bit<32> b12_b15;
  bit<32> b16_b19;
}

header hdr2_h {
	bit<32> b0;
}

header cpu_h {
	bit<16> code_path;
	bit<7> pad0;
	port_t in_port;
	bit<7> pad1;
	port_t out_port;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
	cpu_h cpu;
	hdr0_h hdr0;
	hdr1_h hdr1;
	hdr2_h hdr2;
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
			default: parse_init;
		}
	}

	state parse_cpu {
		pkt.extract(hdr.cpu);
		transition parse_init;
	}

	state parse_init{
		transition parse_0;
	}

	state parse_0 {
		pkt.extract(hdr.hdr0);
		transition parse_1;
	}

	state parse_1{
		transition select(hdr.hdr0.b12++hdr.hdr0.b13) {
			16w2048: parse_2;
			default: reject;
		}
	}

	state parse_2 {
		pkt.extract(hdr.hdr1);
		transition parse_3;
	}

	state parse_3{
		transition select(hdr.hdr1.b9) {
			8w6: parse_4;
			8w17: parse_4;
			default: reject;
		}
	}

	state parse_4 {
		pkt.extract(hdr.hdr2);
		transition accept;
	}
}

#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds
#define EXPIRATION_TIME (5 * ONE_SECOND)

// Pass the value of REGISTER_INDEX_WIDTH during compilation. E.g.:
// $ p4_build.sh cache.p4 -DREGISTER_INDEX_WIDTH=12
#define REGISTER_CAPACITY (1 << REGISTER_INDEX_WIDTH)

struct cache_control_t {
	bit<32> last_time;
	bit<32> valid;
};

struct pair32_t {
	bit<32> first;
	bit<32> second;	
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
	// ============================ TABLE ================================

	action populate() {}

	table table_with_timeout {
		key = {
			hdr.hdr2.b0: exact;
			hdr.hdr1.b12_b15: exact;
			hdr.hdr1.b16_b19: exact;
		}

		actions = {
			populate;
		}

		size = 65536;
		idle_timeout = true;
	}

	// ============================ END TABLE ================================

	action fwd(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}

	action send_to_cpu(bit<16> code_path) {
		hdr.cpu.setValid();
		hdr.cpu.code_path = code_path;
		hdr.cpu.in_port = ig_intr_md.ingress_port;
		fwd(CPU_PCIE_PORT);
	}

	apply {
		if (hdr.cpu.isValid()) {
			fwd(OUT_DEV_PORT);
			hdr.cpu.setInvalid();
		} else {
			if (table_with_timeout.apply().hit) {
				fwd(OUT_DEV_PORT);
			} else {
				send_to_cpu(0);
			}
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
