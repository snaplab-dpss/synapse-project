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

// Pass the value of CACHE_CAPACITY_EXPONENT_BASE_2 during compilation. E.g.:
// $ p4_build.sh cached_table_map.p4 -DCACHE_CAPACITY_EXPONENT_BASE_2=12
#define CACHE_CAPACITY (1 << CACHE_CAPACITY_EXPONENT_BASE_2)
#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds

typedef bit<CACHE_CAPACITY_EXPONENT_BASE_2> hash_t;
typedef bit<32> time_t;
typedef bit<8> match_counter_t;

// Pass the value of EXPIRATION_TIME_SEC during compilation. E.g.:
// $ p4_build.sh cached_table_map.p4 -DEXPIRATION_TIME_SEC=1
const time_t EXPIRATION_TIME = EXPIRATION_TIME_SEC * ONE_SECOND;

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

struct digest_map_t {
	bit<32> src_addr;
	bit<32> dst_addr;
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

control Cache(
	in time_t now,
	in bit<32> src_addr,
	in bit<32> dst_addr,
	in bit<16> src_port,
	in bit<16> dst_port,
	in hash_t hash,
	inout bit<16> port,
	inout DigestType_t digest,
	out bool hit
) {
	Expirator() expirator;
	
	bool valid;
	match_counter_t key_fields_match = 0;

	Register<bit<32>, _>(CACHE_CAPACITY, 0) keys_src_addr;
	Register<bit<32>, _>(CACHE_CAPACITY, 0) keys_dst_addr;
	Register<bit<16>, _>(CACHE_CAPACITY, 0) keys_src_port;
	Register<bit<16>, _>(CACHE_CAPACITY, 0) keys_dst_port;

	Register<bit<16>, _>(CACHE_CAPACITY, 0) values;

	RegisterAction<bit<32>, hash_t, match_counter_t>(keys_src_addr) read_key_src_addr_action = {
		void apply(inout bit<32> curr_key, out match_counter_t key_match) {
			if (curr_key == src_addr) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<32>, hash_t, match_counter_t>(keys_dst_addr) read_key_dst_addr_action = {
		void apply(inout bit<32> curr_key, out match_counter_t key_match) {
			if (curr_key == dst_addr) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<16>, hash_t, match_counter_t>(keys_src_port) read_key_src_port_action = {
		void apply(inout bit<16> curr_key, out match_counter_t key_match) {
			if (curr_key == src_port) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<16>, hash_t, match_counter_t>(keys_dst_port) read_key_dst_port_action = {
		void apply(inout bit<16> curr_key, out match_counter_t key_match) {
			if (curr_key == dst_port) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<16>, hash_t, bit<16>>(values) read_value_action = {
		void apply(inout bit<16> curr_value, out bit<16> out_value) {
			out_value = curr_value;
		}
	};

	action read_key_src_addr() {
		key_fields_match = key_fields_match + read_key_src_addr_action.execute(hash);
	}

	action read_key_dst_addr() {
		key_fields_match = key_fields_match + read_key_dst_addr_action.execute(hash);
	}

	action read_key_src_port() {
		key_fields_match = key_fields_match + read_key_src_port_action.execute(hash);
	}

	action read_key_dst_port() {
		key_fields_match = key_fields_match + read_key_dst_port_action.execute(hash);
	}

	action read_value() {
		port = read_value_action.execute(hash);
	}

	RegisterAction<bit<32>, hash_t, void>(keys_src_addr) write_key_src_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = src_addr;
		}
	};

	RegisterAction<bit<32>, hash_t, void>(keys_dst_addr) write_key_dst_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = dst_addr;
		}
	};

	RegisterAction<bit<16>, hash_t, void>(keys_src_port) write_key_src_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = src_port;
		}
	};

	RegisterAction<bit<16>, hash_t, void>(keys_dst_port) write_key_dst_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = dst_port;
		}
	};

	RegisterAction<bit<16>, hash_t, void>(values) write_value_action = {
		void apply(inout bit<16> curr_value) {
			curr_value = port;
		}
	};

	action write_key_src_addr() {
		write_key_src_addr_action.execute(hash);
	}

	action write_key_dst_addr() {
		write_key_dst_addr_action.execute(hash);
	}

	action write_key_src_port() {
		write_key_src_port_action.execute(hash);
	}

	action write_key_dst_port() {
		write_key_dst_port_action.execute(hash);
	}

	action write_value() {
		write_value_action.execute(hash);
	}

	apply {
		hit = false;
		expirator.apply(now, hash, valid);

		if (valid) {
			read_key_src_addr();
			read_key_dst_addr();
			read_key_src_port();
			read_key_dst_port();
			read_value();
		} else {
			write_key_src_addr();
			write_key_dst_addr();
			write_key_src_port();
			write_key_dst_port();
			write_value();

			digest = 1;
		}

		if (!valid || key_fields_match == 4) {
			hit = true;
		}
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
	Hashing() hashing;
	Cache() cache;

	bit<16> port;
	hash_t hash;
	bool hit;

	action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action send_to_controller() {
		hdr.cpu.setValid();
		hdr.cpu.code_path = 1234;
		hdr.cpu.in_port = ig_intr_md.ingress_port;
		forward(CPU_PCIE_PORT);
	}

	action populate(bit<16> out_port) {
		port = out_port;
	}

	table map {
		key = {
			hdr.ipv4.src_addr: exact;
			hdr.ipv4.dst_addr: exact;
			hdr.tcpudp.src_port: exact;
			hdr.tcpudp.dst_port: exact;
		}

		actions = {
			populate;
		}

		size = 65536;
		idle_timeout = true;
	}

	table forwarder {
		key = {}

		actions = {
			populate;
		}

		size = 1;
	}

	apply {
		if (hdr.cpu.isValid()) {
			forward(hdr.cpu.out_port);
			hdr.cpu.setInvalid();
		} else {
			hashing.apply(
				hdr.ipv4.src_addr,
				hdr.ipv4.dst_addr,
				hdr.tcpudp.src_port,
				hdr.tcpudp.dst_port,
				hash
			);

			forwarder.apply();

			if (map.apply().hit) {
				forward(port[8:0]);
			} else {
				cache.apply(
					ig_intr_md.ingress_mac_tstamp[47:16],
					hdr.ipv4.src_addr,
					hdr.ipv4.dst_addr,
					hdr.tcpudp.src_port,
					hdr.tcpudp.dst_port,
					hash,
					port,
					ig_dprsr_md.digest_type,
					hit
				);

				if (!hit) {
					send_to_controller();
				} else {
					forward(port[8:0]);
				}
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
	Digest<digest_map_t>() digest_map;

	apply {
		if (ig_dprsr_md.digest_type == 1) {
			digest_map.pack({
				hdr.ipv4.src_addr,
				hdr.ipv4.dst_addr,
				hdr.tcpudp.src_port,
				hdr.tcpudp.dst_port
			});
		}

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
