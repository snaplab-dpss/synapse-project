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

const bit<16> TYPE_IPV4        = 0x800;
const bit<8>  IP_PROTOCOLS_TCP = 6;
const bit<8>  IP_PROTOCOLS_UDP = 17;

#define CACHE_CAPACITY_EXPONENT_BASE_2 32
#define CACHE_CAPACITY 65535
#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds

#define BURST_EXPONENT_BASE_2 14
#define RATE_EXPONENT_BASE_2 20

#define BURST (1<<BURST_EXPONENT_BASE_2)
#define RATE (1<<RATE_EXPONENT_BASE_2)
#define BURST_TOKENS (1<<(RATE_EXPONENT_BASE_2-BURST_EXPONENT_BASE_2))
#define EXPIRATION_TIME ONE_SECOND

typedef bit<CACHE_CAPACITY_EXPONENT_BASE_2> hash_t;
typedef bit<32> time_t;
typedef bit<8> match_counter_t;
typedef bit<64> tokens_t;

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

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
	cpu_h cpu;
	ethernet_t ethernet;
	ipv4_t ipv4;
	tcpudp_t tcpudp;
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

control Hashing(
	in bit<32> src_addr,
	in bit<32> dst_addr,
	in bit<16> src_port,
	in bit<16> dst_port,
	out hash_t hash
) {
	Hash<hash_t>(HashAlgorithm_t.CRC32) hash_calculator;

	action calc_hash() {
		hash = hash_calculator.get({
			src_addr,
			dst_addr,
			src_port,
			dst_port
		});
	}
	
	apply {
		calc_hash();
	}
}

control Expirator(
	in time_t now,
	in hash_t hash,
	out bool valid
) {
	Register<time_t, _>(CACHE_CAPACITY, 0) reg;

	RegisterAction<time_t, hash_t, bool>(reg) write_action = {
		void apply(inout time_t alarm, out bool alive) {
			if (now < alarm) {
				alive = true;
			} else {
				alive = false;
			}
			
			alarm = now + EXPIRATION_TIME;
		}
	};

	action update_alarm() {
		valid = write_action.execute(hash);
	}

	apply {
		update_alarm();
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
	Hashing() hashing;

	hash_t hash;
	time_t now;

	time_t elapsed;
	tokens_t delta_tokens;
	tokens_t current_tokens;

	Register<time_t, _>(CACHE_CAPACITY, 0) clocks;
	Register<tokens_t, _>(CACHE_CAPACITY, 0) tokens;

	RegisterAction<time_t, hash_t, time_t>(clocks) update_clock_action = {
		void apply(inout time_t last_time, out time_t out_time) {
			out_time = now - last_time;
			last_time = now;
		}
	};

	action update_clock() {
		elapsed = update_clock_action.execute(hash);
	}

	RegisterAction<tokens_t, hash_t, tokens_t>(tokens) update_tokens_action = {
		void apply(inout tokens_t last_tokens, out tokens_t out_tokens) {
			if (elapsed < BURST_TOKENS) {
				last_tokens = last_tokens + (elapsed << RATE_EXPONENT_BASE_2);
			} else {
				last_tokens = BURST_TOKENS;
			}
			out_tokens = last_tokens;
		}
	};

	action calculate_delta_tokens() {
		delta_tokens = elapsed << RATE_EXPONENT_BASE_2;
	}

	action update_tokens() {
		current_tokens = update_tokens_action.execute(hash);
	}

	apply {
		now = ig_intr_md.ingress_mac_tstamp[47:16];

		hashing.apply(
			hdr.ipv4.src_addr,
			hdr.ipv4.dst_addr,
			hdr.tcpudp.src_port,
			hdr.tcpudp.dst_port,
			hash
		);

		update_clock();
		update_tokens();

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
