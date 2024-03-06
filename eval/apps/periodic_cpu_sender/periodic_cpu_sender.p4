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
#endif

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

const bit<16> TYPE_IPV4 = 0x800;

const bit<8> IP_PROTOCOLS_TCP = 6;
const bit<8> IP_PROTOCOLS_UDP = 17;
const bit<8> IP_PROTOCOLS_WARMUP = 146;

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

struct my_ingress_metadata_t {
	bool is_warmup_pkt;
}

struct my_ingress_headers_t {
	cpu_h cpu;
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

		meta.is_warmup_pkt = false;

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
			IP_PROTOCOLS_WARMUP: parse_warmup;
			default: accept;
		}
	}

	state parse_warmup {
		meta.is_warmup_pkt = true;
		transition parse_tcpudp;
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
	bit<16> out_port;
	bit<32> alarm;
	bool send_to_cpu;

	Counter<bit<32>, port_t>(1, CounterType_t.PACKETS_AND_BYTES) pkt_counter;
	Counter<bit<32>, port_t>(1, CounterType_t.PACKETS_AND_BYTES) cpu_counter;

	action set_alarm(bit<32> _alarm) {
		alarm = _alarm;
	}

	table alarm_table {
		key = {}

		actions = {
			set_alarm;
		}

		size = 1;
	}

	Register<bit<32>, _>(1, 1) timer;

	RegisterAction<bit<32>, bit<32>, bool>(timer) update_timer_action = {
		void apply(inout bit<32> curr_time, out bool should_send_to_cpu) {
			if (curr_time == alarm) {
				curr_time = 1;
				should_send_to_cpu = true;
			} else {
				curr_time = curr_time + 1;
				should_send_to_cpu = false;
			}
		}
	};

	action update_timer() {
		send_to_cpu = update_timer_action.execute(0);
	}

	action populate(bit<16> _out_port) {
		out_port = _out_port;
	}

	table forwarder {
		key = {}

		actions = {
			populate;
		}

		size = 1;
	}

	action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action send_to_controller() {
		hdr.cpu.setValid();
		hdr.cpu.code_path = 0;
		hdr.cpu.in_port = ig_intr_md.ingress_port;
		forward(CPU_PCIE_PORT);
	}

	apply {
		if (ig_intr_md.ingress_port == CPU_PCIE_PORT) {
			hdr.cpu.setInvalid();
			forward(hdr.cpu.out_port[8:0]);
		} else {
			if (!meta.is_warmup_pkt) {
				pkt_counter.count(0);
			}

			forwarder.apply();

			alarm_table.apply();
			update_timer();

			if (send_to_cpu) {
				send_to_controller();

				if (!meta.is_warmup_pkt) {
					cpu_counter.count(0);
				}
			} else {
				forward(out_port[8:0]);
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
