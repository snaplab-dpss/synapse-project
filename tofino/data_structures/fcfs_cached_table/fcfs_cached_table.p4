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

#define bswap32(x) (x[7:0] ++ x[15:8] ++ x[23:16] ++ x[31:24])
#define bswap16(x) (x[7:0] ++ x[15:8])

typedef bit<9> port_t;
typedef bit<7> port_pad_t;
typedef bit<32> time_t;

const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<8>  IP_PROTO_TCP = 6;
const bit<8>  IP_PROTO_UDP = 17;
const bit<16> KVS_PORT = 670;

#define KVS_SERVER_NF_DEV 0

const bit<8> KVS_STATUS_MISS = 0;
const bit<8> KVS_STATUS_HIT = 1;

const bit<8> KVS_OP_GET = 0;
const bit<8> KVS_OP_PUT = 1;

const bit<32> HASH_SALT_1 = 0xfbc31fc7;
const bit<32> HASH_SALT_2 = 0x2681580b;

enum bit<2> fwd_op_t {
	FORWARD_NF_DEV	= 0,
	FORWARD_TO_CPU  = 1,
	RECIRCULATE 	= 2,
	DROP 			= 3,
}

// Entry Timeout Expiration (units of 65536 ns).
#define ENTRY_TIMEOUT 16384 // 1 s
#define CACHE_CAPACITY_LOG2 5
#define FCFS_CT_CACHE_CAPACITY 32

typedef bit<CACHE_CAPACITY_LOG2> fcfs_ct_hash_t;

header cpu_h {
	bit<16>	code_path;                   // Written by the data plane
	bit<16>	egress_dev;                  // Written by the control plane
	bit<8>	trigger_dataplane_execution; // Written by the control plane
}

header recirc_h {
	bit<16>	code_path;
	bit<16> ingress_port;
	bit<32> dev;
}

header ethernet_h {
	bit<48> dstAddr;
	bit<48> srcAddr;
	bit<16> etherType;
}

header ipv4_h {
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

header udp_h {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> len;
	bit<16> checksum;
}

struct headers_t {
	cpu_h cpu;
	recirc_h recirc;
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
}

struct metadata_t {
	bit<16> ingress_port;
	bit<32> dev;
	bit<32> time;
	fcfs_ct_hash_t fcfs_cached_table_hash;
	bit<32> fcfs_cached_table_value;
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
	out headers_t hdr,
	out metadata_t meta,
	out ingress_intrinsic_metadata_t ig_intr_md
) {
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, ig_intr_md);

		meta.ingress_port[8:0] = ig_intr_md.ingress_port;
		meta.dev = 0;
		meta.time = ig_intr_md.ingress_mac_tstamp[47:16];

		transition select(ig_intr_md.ingress_port) {
			CPU_PCIE_PORT: parse_cpu;
			RECIRCULATION_PORT_0: parse_recirc;
			RECIRCULATION_PORT_1: parse_recirc;
			#if __TARGET_TOFINO__ == 2
			RECIRCULATION_PORT_2: parse_recirc;
			RECIRCULATION_PORT_3: parse_recirc;
			#endif
			default: parser_init;
		}
	}

	state parse_cpu {
		pkt.extract(hdr.cpu);
		transition accept;
	}

	state parse_recirc {
		pkt.extract(hdr.recirc);
		transition select(hdr.recirc.code_path) {
			default: parser_init;
		}
	}

	state parser_init {
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

struct fcfs_cached_table_digest_t {
	bit<32> src_addr;
	bit<32> dst_addr;
	bit<16> src_port;
	bit<16> dst_port;
	bit<32> value;
	fcfs_ct_hash_t hash;
}

control FCFSCachedTableCache(
	in time_t now,
	in bit<32> src_addr,
	in bit<32> dst_addr,
	in bit<16> src_port,
	in bit<16> dst_port,
	inout bit<32> value,
	out fcfs_ct_hash_t hash,
	out DigestType_t digest,
	out bool hit
) {
	Hash<fcfs_ct_hash_t>(HashAlgorithm_t.CRC32) hash_calculator;

	action calc_hash() {
		hash = hash_calculator.get({
			src_addr,
			dst_addr,
			src_port,
			dst_port
		});
	}

	Register<time_t, _>(FCFS_CT_CACHE_CAPACITY, 0) reg;

	RegisterAction<time_t, fcfs_ct_hash_t, bool>(reg) write_action = {
		void apply(inout time_t alarm, out bool alive) {
			if (now < alarm) {
				alive = true;
			} else {
				alive = false;
			}
			
			alarm = now + ENTRY_TIMEOUT;
		}
	};

	bool valid = false;
	action update_alarm() { valid = write_action.execute(hash); }
	
	bit<8> key_fields_match = 0;

	Register<bit<32>, _>(FCFS_CT_CACHE_CAPACITY, 0) keys_src_addr;
	Register<bit<32>, _>(FCFS_CT_CACHE_CAPACITY, 0) keys_dst_addr;
	Register<bit<16>, _>(FCFS_CT_CACHE_CAPACITY, 0) keys_src_port;
	Register<bit<16>, _>(FCFS_CT_CACHE_CAPACITY, 0) keys_dst_port;

	Register<bit<32>, _>(FCFS_CT_CACHE_CAPACITY, 0) values;

	RegisterAction<bit<32>, fcfs_ct_hash_t, bit<8>>(keys_src_addr) read_key_src_addr_action = {
		void apply(inout bit<32> curr_key, out bit<8> key_match) {
			if (curr_key == src_addr) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, bit<8>>(keys_dst_addr) read_key_dst_addr_action = {
		void apply(inout bit<32> curr_key, out bit<8> key_match) {
			if (curr_key == dst_addr) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, bit<8>>(keys_src_port) read_key_src_port_action = {
		void apply(inout bit<16> curr_key, out bit<8> key_match) {
			if (curr_key == src_port) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, bit<8>>(keys_dst_port) read_key_dst_port_action = {
		void apply(inout bit<16> curr_key, out bit<8> key_match) {
			if (curr_key == dst_port) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, bit<32>>(values) read_value_action = {
		void apply(inout bit<32> curr_value, out bit<32> out_value) {
			out_value = curr_value;
		}
	};

	action read_key_src_addr() { key_fields_match = key_fields_match + read_key_src_addr_action.execute(hash); }
	action read_key_dst_addr() { key_fields_match = key_fields_match + read_key_dst_addr_action.execute(hash); }
	action read_key_src_port() { key_fields_match = key_fields_match + read_key_src_port_action.execute(hash); }
	action read_key_dst_port() { key_fields_match = key_fields_match + read_key_dst_port_action.execute(hash); }
	action read_value() { value = read_value_action.execute(hash); }

	RegisterAction<bit<32>, fcfs_ct_hash_t, void>(keys_src_addr) write_key_src_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = src_addr;
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, void>(keys_dst_addr) write_key_dst_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = dst_addr;
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, void>(keys_src_port) write_key_src_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = src_port;
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, void>(keys_dst_port) write_key_dst_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = dst_port;
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, void>(values) write_value_action = {
		void apply(inout bit<32> curr_value) {
			curr_value = value;
		}
	};

	action write_key_src_addr() { write_key_src_addr_action.execute(hash); }
	action write_key_dst_addr() { write_key_dst_addr_action.execute(hash); }
	action write_key_src_port() { write_key_src_port_action.execute(hash); }
	action write_key_dst_port() { write_key_dst_port_action.execute(hash); }
	action write_value() { write_value_action.execute(hash); }

	apply {
		hit = false;
		calc_hash();
		update_alarm();
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
			if (key_fields_match == 4) {
				hit = true;
			}
		}
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
	action drop() { ig_dprsr_md.drop_ctl = 1; }
	action fwd(bit<16> port) { ig_tm_md.ucast_egress_port = port[8:0]; }

	action fwd_to_cpu() {
		hdr.recirc.setInvalid();
		fwd(CPU_PCIE_PORT);
	}

	action fwd_nf_dev(bit<16> port) {
		hdr.cpu.setInvalid();
		hdr.recirc.setInvalid();
		fwd(port);
	}

	action swap(inout bit<8> a, inout bit<8> b) {
		bit<8> tmp = a;
		a = b;
		b = tmp;
	}

	action build_cpu_hdr(bit<16> code_path) {
		hdr.cpu.setValid();
		hdr.cpu.code_path = code_path;
	}

	action set_ingress_dev(bit<32> nf_dev) {
		meta.dev = nf_dev;
	}

	action set_ingress_dev_from_recirculation() {
		meta.ingress_port = hdr.recirc.ingress_port;
		meta.dev = hdr.recirc.dev;
	}

	table ingress_port_to_nf_dev {
		key = {
			meta.ingress_port: exact;
		}
		actions = {
			set_ingress_dev;
			set_ingress_dev_from_recirculation;
			drop;
		}

		size = 64;
		const default_action = drop();
	}

	fwd_op_t fwd_op = fwd_op_t.FORWARD_NF_DEV;
	bit<32> nf_dev = 0;
	table forwarding_tbl {
		key = {
			fwd_op: exact;
			nf_dev: ternary;
			meta.ingress_port: ternary;
		}

		actions = {
			fwd;
			fwd_nf_dev;
			fwd_to_cpu;
			drop;
		}

		size = 128;

		const default_action = drop();
	}

	// ============================ FCFS Cached Table =========================================

	bit<32> fcfs_cached_table_value = 0;
	action fcfs_ct_table_get_value(bit<32> value) {
		fcfs_cached_table_value = value;
	}

	table fcfs_ct_table {
		key = {
			hdr.ipv4.src_addr: exact;
			hdr.ipv4.dst_addr: exact;
			hdr.udp.src_port: exact;
			hdr.udp.dst_port: exact;
		}

		actions = { fcfs_ct_table_get_value; }

		size = 65536;
		idle_timeout = true;
	}

	FCFSCachedTableCache() fcfs_cached_table_cache;

	apply {
		ingress_port_to_nf_dev.apply();

		if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
			nf_dev[15:0] = hdr.cpu.egress_dev;
		} else {
			if (fcfs_ct_table.apply().hit) {
				hdr.ipv4.src_addr = fcfs_cached_table_value;
				nf_dev = meta.dev;
			} else {
				bool fcfs_cached_table_cache_hit;
				fcfs_cached_table_cache.apply(
					meta.time,
					hdr.ipv4.src_addr,
					hdr.ipv4.dst_addr,
					hdr.udp.src_port,
					hdr.udp.dst_port,
					meta.fcfs_cached_table_value,
					meta.fcfs_cached_table_hash,
					ig_dprsr_md.digest_type,
					fcfs_cached_table_cache_hit
				);

				if (!fcfs_cached_table_cache_hit) {
					fwd_op = fwd_op_t.FORWARD_TO_CPU;
					build_cpu_hdr(0);
				} else {
					hdr.ipv4.src_addr = meta.fcfs_cached_table_value;
					nf_dev = meta.dev;
				}
			}
		}

		ig_tm_md.bypass_egress = 1;
	}
}

control IngressDeparser(
	packet_out pkt,
	inout headers_t hdr,
	in metadata_t meta,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
	Digest<fcfs_cached_table_digest_t>() fcfs_cached_table_digest;

	apply {
		if (ig_dprsr_md.digest_type == 1) {
			fcfs_cached_table_digest.pack({
				hdr.ipv4.src_addr,
				hdr.ipv4.dst_addr,
				hdr.udp.src_port,
				hdr.udp.dst_port,
				meta.fcfs_cached_table_value,
				meta.fcfs_cached_table_hash
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
