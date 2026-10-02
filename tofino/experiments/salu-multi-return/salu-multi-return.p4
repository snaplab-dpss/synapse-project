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

// A HashPipe stage cell: a {key, count} pair; the carried pair arrives in metadata.
struct pair_t {
	bit<32> lo; // key
	bit<32> hi; // count
}

struct my_ingress_metadata_t {
	bit<32> carried_key;
	bit<32> carried_count;
	bit<32> old_key;
	bit<32> old_count;
	bit<32> out_key;
	bit<32> out_count;
	bit<8> outcome;
	bit<16> predicate;
	bit<10> index;
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
		inout my_ingress_headers_t hdr,
		inout my_ingress_metadata_t meta,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

	Register<pair_t, bit<10>>(1024) stage;

#if VARIANT == 1
	// Three return values: the old pair (always from memory) and an outcome code (always a
	// constant): 0 = counted, 1 = swapped, 2 = kept. The control then updates the carried pair.
	RegisterAction3<pair_t, bit<10>, bit<32>, bit<32>, bit<8>>(stage) inc_or_swap = {
		void apply(inout pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<8> outcome) {
			pair_t in_value = value;
			old_key = in_value.lo;
			old_count = in_value.hi;
			if (in_value.lo == meta.carried_key) {
				value.hi = in_value.hi + meta.carried_count;
				outcome = 0;
			} else if (in_value.hi < meta.carried_count) {
				value.lo = meta.carried_key;
				value.hi = meta.carried_count;
				outcome = 1;
			} else {
				outcome = 2;
			}
		}
	};
#elif VARIANT == 2
	// Two return values whose source differs per branch: zero constants when counted, memory
	// when swapped, the carried (PHV) pair when kept.
	RegisterAction2<pair_t, bit<10>, bit<32>, bit<32>>(stage) inc_or_swap = {
		void apply(inout pair_t value, out bit<32> out_key, out bit<32> out_count) {
			pair_t in_value = value;
			if (in_value.lo == meta.carried_key) {
				value.hi = in_value.hi + meta.carried_count;
				out_key = 0;
				out_count = 0;
			} else if (in_value.hi < meta.carried_count) {
				value.lo = meta.carried_key;
				value.hi = meta.carried_count;
				out_key = in_value.lo;
				out_count = in_value.hi;
			} else {
				out_key = meta.carried_key;
				out_count = meta.carried_count;
			}
		}
	};
#elif VARIANT == 3
	// The first stage: always inserts, one predicate, three return values.
	RegisterAction3<pair_t, bit<10>, bit<32>, bit<32>, bit<8>>(stage) inc_or_swap = {
		void apply(inout pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<8> outcome) {
			pair_t in_value = value;
			old_key = in_value.lo;
			old_count = in_value.hi;
			if (in_value.lo == meta.carried_key) {
				value.hi = in_value.hi + meta.carried_count;
				outcome = 0;
			} else {
				value.lo = meta.carried_key;
				value.hi = meta.carried_count;
				outcome = 1;
			}
		}
	};
#elif VARIANT == 4
	// Two predicates, three return values: the old pair (memory) and the SALU's predicate
	// output, which encodes which comparisons held (bit index = cmp1 << 1 | cmp0).
	RegisterAction3<pair_t, bit<10>, bit<32>, bit<32>, bit<16>>(stage) inc_or_swap = {
		void apply(inout pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<16> outcome) {
			pair_t in_value = value;
			old_key = in_value.lo;
			old_count = in_value.hi;
			bool hit = in_value.lo == meta.carried_key;
			bool lighter = in_value.hi < meta.carried_count;
			outcome = this.predicate<bit<16>>(hit, lighter);
			if (hit) {
				value.hi = in_value.hi + meta.carried_count;
			} else if (lighter) {
				value.lo = meta.carried_key;
				value.hi = meta.carried_count;
			}
		}
	};
#elif VARIANT == 6
	// Two predicates, two return values (the old pair, from memory); the outcome is recomputed
	// outside from the old pair and the carried one.
	RegisterAction2<pair_t, bit<10>, bit<32>, bit<32>>(stage) inc_or_swap = {
		void apply(inout pair_t value, out bit<32> old_key, out bit<32> old_count) {
			pair_t in_value = value;
			old_key = in_value.lo;
			old_count = in_value.hi;
			if (in_value.lo == meta.carried_key) {
				value.hi = in_value.hi + meta.carried_count;
			} else if (in_value.hi < meta.carried_count) {
				value.lo = meta.carried_key;
				value.hi = meta.carried_count;
			}
		}
	};
#endif

	action run_stage() {
#if VARIANT == 2
		meta.out_key = inc_or_swap.execute(meta.index, meta.out_count);
#elif VARIANT == 6
		meta.old_key = inc_or_swap.execute(meta.index, meta.old_count);
#elif VARIANT == 4
		meta.old_key = inc_or_swap.execute(meta.index, meta.old_count, meta.predicate);
#else
		meta.old_key = inc_or_swap.execute(meta.index, meta.old_count, meta.outcome);
#endif
	}

	table stage_tbl {
		actions = { run_stage; }
		default_action = run_stage();
		size = 1;
	}

	apply {
		meta.carried_key = hdr.ethernet.srcAddr[31:0];
		meta.carried_count = 1;
		meta.index = hdr.ethernet.dstAddr[9:0];
		stage_tbl.apply();
#if VARIANT == 2
		meta.carried_key = meta.out_key;
		meta.carried_count = meta.out_count;
#elif VARIANT == 4
		if (meta.predicate == 2 || meta.predicate == 8) {
			meta.carried_key = 0;
			meta.carried_count = 0;
		} else if (meta.predicate == 4) {
			meta.carried_key = meta.old_key;
			meta.carried_count = meta.old_count;
		}
#elif VARIANT == 6
		if (meta.old_key == meta.carried_key) {
			meta.carried_key = 0;
			meta.carried_count = 0;
		} else if (meta.old_count < meta.carried_count) {
			meta.carried_key = meta.old_key;
			meta.carried_count = meta.old_count;
		}
#else
		if (meta.outcome == 0) {
			meta.carried_key = 0;
			meta.carried_count = 0;
		} else if (meta.outcome == 1) {
			meta.carried_key = meta.old_key;
			meta.carried_count = meta.old_count;
		}
#endif
		hdr.ethernet.srcAddr[31:0] = meta.carried_key;
		hdr.ethernet.dstAddr[31:0] = meta.carried_count;
		ig_tm_md.ucast_egress_port = OUT_PORT;
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
