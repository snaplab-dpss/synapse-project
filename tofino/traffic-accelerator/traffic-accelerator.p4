#include <core.p4>

#if __TARGET_TOFINO__ == 2
  #include <t2na.p4>
  #define CPU_PCIE_PORT 0
  #define RECIRCULATION_PORT 6
#else
  #include <tna.p4>
  #define CPU_PCIE_PORT 192
  #define RECIRCULATION_PORT 68
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

header bridge_metadata_t {
	bit<8> has_recirc_hdr;
	bit<8> bypass_egress;
}

header recirc_t {
	bit<16> max_recirculations;
	bit<16> recirculation_count;
}

struct headers_t {
	bridge_metadata_t bridge_metadata;
	recirc_t recirc;
	ethernet_t ethernet;
	ipv4_t ipv4;
	udp_t udp;
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
		transition select (ig_intr_md.ingress_port) {
			RECIRCULATION_PORT: parse_recirculation_hdr;
			default: parse_ethernet;
		}
	}

	state parse_recirculation_hdr {
		pkt.extract(hdr.recirc);
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

	action flood() {
		ig_tm_md.mcast_grp_a = 0x1;
	}
	
	action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action recirculate() {
		hdr.bridge_metadata.has_recirc_hdr = 1;
		forward(RECIRCULATION_PORT);
	}

	action bypass_egress() {
		// ig_tm_md.bypass_egress = 1;
		hdr.bridge_metadata.bypass_egress = 1;
	}

	Register<bit<16>, _>(1, 1) rate_multiplier_register;
	RegisterAction<bit<16>, bit<16>, bit<16>>(rate_multiplier_register) read_rate_multiplier_action = {
		void apply(inout bit<16> value, out bit<16> out_value) {
			out_value = value;
		}
	};

	action swap32(inout bit<32> a, inout bit<32> b) {
		bit<32> tmp = a;
		a = b;
		b = tmp;
	}

	action swap16(inout bit<16> a, inout bit<16> b) {
		bit<16> tmp = a;
		a = b;
		b = tmp;
	}

	bool is_tg_traffic = false;
	action route_tg() {
		// flood();
		is_tg_traffic = true;
	}

	action route_symmetric_response() {
		swap32(hdr.ipv4.src_addr, hdr.ipv4.dst_addr);
		swap16(hdr.udp.src_port, hdr.udp.dst_port);
		forward(ig_intr_md.ingress_port);
		bypass_egress();
	}

	table router_tbl {
		key = {
			ig_intr_md.ingress_port: exact;
		}

		actions = {
			route_tg;
			route_symmetric_response;
			forward;
			drop;
		}

		size = 32;
	}

	Counter<bit<64>, bit<9>>(1024, CounterType_t.PACKETS_AND_BYTES) in_counter;

	apply {
		hdr.bridge_metadata.setValid();
		hdr.bridge_metadata.has_recirc_hdr = 0;
		hdr.bridge_metadata.bypass_egress = 0;
		
		router_tbl.apply();
		in_counter.count(ig_intr_md.ingress_port);
		
		if (is_tg_traffic) {
			bit<16> max_recirculations = read_rate_multiplier_action.execute(0);

			hdr.recirc.setValid();
			hdr.recirc.recirculation_count = 0;
			hdr.recirc.max_recirculations = max_recirculations;
			
			recirculate();
			bypass_egress();
		} else if (hdr.recirc.isValid()) {
			hdr.recirc.recirculation_count = hdr.recirc.recirculation_count + 1;

			if (hdr.recirc.recirculation_count != hdr.recirc.max_recirculations) {
				recirculate();
				flood();
			} else {
				hdr.recirc.setInvalid();
				flood();
			}
		}
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
		transition parse_bridge_metadata;
	}

	state parse_bridge_metadata {
		pkt.extract(hdr.bridge_metadata);
		transition select(hdr.bridge_metadata.has_recirc_hdr) {
			0: parse_ethernet;
			1: parse_recirculation_hdr;
		}
	}

	state parse_recirculation_hdr {
		pkt.extract(hdr.recirc);
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
	Counter<bit<64>, bit<9>>(1024, CounterType_t.PACKETS_AND_BYTES) out_counter;

	apply {
		out_counter.count(eg_intr_md.egress_port);
		hdr.bridge_metadata.setInvalid();
	}
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
