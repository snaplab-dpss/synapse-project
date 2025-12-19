#include <core.p4>

#if __TARGET_TOFINO__ == 2
	#include <t2na.p4>
	#define CPU_PCIE_PORT 0
	#define RECIRCULATION_PORT_0 6
	#define RECIRCULATION_PORT_1 128
	#define RECIRCULATION_PORT_2 256
	#define RECIRCULATION_PORT_3 384
#else
	#include <tna.p4>
	#define CPU_PCIE_PORT 192
	#define RECIRCULATION_PORT_0 68
	#define RECIRCULATION_PORT_1 196
#endif

typedef bit<9> port_t;
typedef bit<7> port_pad_t;
typedef bit<32> time_t;

const bit<16> ETHERTYPE_IPV4 = 0x800;
const bit<8>  IP_PROTO_TCP   = 6;
const bit<8>  IP_PROTO_UDP   = 17;

header cpu_h {
	bit<16>	code_path;                   // Written by the data plane
	bit<16>	egress_dev;                  // Written by the control plane
	bit<8>	trigger_dataplane_execution; // Written by the control plane
	bit<16> ingress_dev;
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

header udp_t {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> len;
	bit<16> checksum;
}

struct headers_t {
	cpu_h cpu;
	ethernet_t ethernet;
	ipv4_t ipv4;
	udp_t udp;
}

struct metadata_t {
	bit<16> integer_allocator_index;
	bit<16> integer_allocator_tail;
}

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
		transition select(ig_intr_md.ingress_port) {
			CPU_PCIE_PORT: parse_cpu;
			default: parse_ethernet;
		}
	}

	state parse_cpu {
		pkt.extract(hdr.cpu);
		transition accept;
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

	action send_to_controller(bit<16> code_path) {
		hdr.cpu.setValid();
		hdr.cpu.code_path = code_path;
		forward(CPU_PCIE_PORT);
	}

	table integer_allocator_allocated_indexes {
		key = {
			meta.integer_allocator_index: exact;
		}

		actions = {
			NoAction;
		}

		size = 100000;
		idle_timeout = true;
	}

	Register<bit<16>, _>(1, 0) integer_allocator_head_reg;
	Register<bit<16>, _>(1, 0) integer_allocator_tail_reg;
	Register<bit<16>, _>(65536, 0) integer_allocator_indexes_reg;
	Register<bit<8>, _>(65536, 0) integer_allocator_pending_reg;

	RegisterAction<bit<8>, bit<16>, bool>(integer_allocator_pending_reg) integer_allocator_pending_read = {
		void apply(inout bit<8> is_pending, out bool out_is_pending) {
			if (is_pending == 0) {
				out_is_pending = false;
			} else {
				out_is_pending = true;
			}
		}
	};

	RegisterAction<bit<8>, bit<16>, void>(integer_allocator_pending_reg) integer_allocator_pending_set = {
		void apply(inout bit<8> is_pending) {
			is_pending = 1;
		}
	};

	RegisterAction<bit<16>, bit<16>, bit<16>>(integer_allocator_tail_reg) integer_allocator_tail_read = {
		void apply(inout bit<16> tail, out bit<16> out_tail) {
			out_tail = tail;
		}
	};

	RegisterAction<bit<16>, bit<16>, bit<16>>(integer_allocator_head_reg) integer_allocator_head_inc = {
		void apply(inout bit<16> head, out bit<16> out_head) {
			if (meta.integer_allocator_tail != head) {
				out_head = head;
				head = head + 1;
			}
		}
	};

	RegisterAction<bit<16>, bit<16>, bit<16>>(integer_allocator_indexes_reg) integer_allocator_indexes_read = {
		void apply(inout bit<16> index, out bit<16> out_index) {
			out_index = index;
		}
	};

	apply {
		meta.integer_allocator_index = hdr.udp.dst_port;

		if (ig_intr_md.ingress_port == 0) {
			bool hit = integer_allocator_allocated_indexes.apply().hit;
			if (!hit) {
				bool is_pending = integer_allocator_pending_read.execute(meta.integer_allocator_index);
				if (!is_pending) {
					drop();
				}
			}
		} else {
			meta.integer_allocator_tail = integer_allocator_tail_read.execute(0);
			bit<16> integer_allocator_old_head = integer_allocator_head_inc.execute(0);
			if (integer_allocator_old_head != meta.integer_allocator_tail) {
				meta.integer_allocator_index = integer_allocator_indexes_read.execute(integer_allocator_old_head);
				integer_allocator_pending_set.execute(meta.integer_allocator_index);
				ig_dprsr_md.digest_type = 1;

				hdr.udp.dst_port = meta.integer_allocator_index;
			} else {
				drop();
			}
		}

		forward(ig_intr_md.ingress_port);
	}
}

struct integer_allocator_digest_t {
	bit<16> allocated_index;
}

control IngressDeparser(
	packet_out pkt,
	inout headers_t hdr,
	in metadata_t meta,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
	Digest<integer_allocator_digest_t>() integer_allocator_digest;

	apply {
		if (ig_dprsr_md.digest_type == 1) {
			integer_allocator_digest.pack({
				meta.integer_allocator_index
			});
		}

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