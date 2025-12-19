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

#define FCFS_CT_CAPACITY 65536
#define FCFS_CT_CACHE_CAPACITY 65536

typedef bit<16> fcfs_ct_hash_t;

header cpu_h {
	bit<16>	code_path;                   // Written by the data plane
	bit<16>	egress_dev;                  // Written by the control plane
	bit<8>	trigger_dataplane_execution; // Written by the control plane
	bit<16> ingress_dev;
	bit<32> new_index;
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
	bit<32> fcfs_ct_key_0;
	bit<32> fcfs_ct_key_1;
	bit<16> fcfs_ct_key_2;
	bit<16> fcfs_ct_key_3;
	bit<32> fcfs_ct_value;
	bool fcfs_ct_is_alive;
	bit<8> fcfs_ct_key_fields_match;
	bit<32> fcfs_ct_integer_allocator_tail;
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

struct fcfs_ct_digest_t {
	bit<32> src_addr;
	bit<32> dst_addr;
	bit<16> src_port;
	bit<16> dst_port;
	bit<32> value;
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
		}

		size = 64;
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

	action fcfs_ct_table_0_get_value(bit<32> value) {
		meta.fcfs_ct_value = value;
	}

	table fcfs_ct_table_0 {
		key = {
			meta.fcfs_ct_key_0: exact;
			meta.fcfs_ct_key_1: exact;
			meta.fcfs_ct_key_2: exact;
			meta.fcfs_ct_key_3: exact;
		}

		actions = { fcfs_ct_table_0_get_value; }

		size = FCFS_CT_CAPACITY * 2;
		idle_timeout = true;
	}

	action fcfs_ct_table_1_get_value(bit<32> value) {
		meta.fcfs_ct_value = value;
	}

	table fcfs_ct_table_1 {
		key = {
			meta.fcfs_ct_key_0: exact;
			meta.fcfs_ct_key_1: exact;
			meta.fcfs_ct_key_2: exact;
			meta.fcfs_ct_key_3: exact;
		}

		actions = { fcfs_ct_table_1_get_value; }

		size = FCFS_CT_CAPACITY * 2;
		idle_timeout = true;
	}

	Register<bit<32>, _>(1, 0) fcfs_ct_integer_allocator_head_reg;
	Register<bit<32>, _>(1, 0) fcfs_ct_integer_allocator_tail_reg;
	Register<bit<32>, _>(FCFS_CT_CAPACITY, 0) fcfs_ct_integer_allocator_indexes_reg;
	Register<bit<8>, _>(FCFS_CT_CAPACITY, 0) fcfs_ct_integer_allocator_pending_reg;

	RegisterAction<bit<8>, bit<32>, bool>(fcfs_ct_integer_allocator_pending_reg) fcfs_ct_integer_allocator_pending_read = {
		void apply(inout bit<8> is_pending, out bool out_is_pending) {
			if (is_pending == 0) {
				out_is_pending = false;
			} else {
				out_is_pending = true;
			}
		}
	};

	RegisterAction<bit<8>, bit<32>, void>(fcfs_ct_integer_allocator_pending_reg) fcfs_ct_integer_allocator_pending_set = {
		void apply(inout bit<8> is_pending) {
			is_pending = 1;
		}
	};

	RegisterAction<bit<32>, bit<32>, bit<32>>(fcfs_ct_integer_allocator_tail_reg) fcfs_ct_integer_allocator_tail_read = {
		void apply(inout bit<32> tail, out bit<32> out_tail) {
			out_tail = tail;
		}
	};

	RegisterAction<bit<32>, bit<32>, bit<32>>(fcfs_ct_integer_allocator_head_reg) fcfs_ct_integer_allocator_head_inc = {
		void apply(inout bit<32> head, out bit<32> out_head) {
			if (meta.fcfs_ct_integer_allocator_tail != head) {
				out_head = head;
				if (head == FCFS_CT_CAPACITY - 1) {
					head = 0;
				} else {
					head = head + 1;
				}
			}
		}
	};

	RegisterAction<bit<32>, bit<32>, bit<32>>(fcfs_ct_integer_allocator_indexes_reg) fcfs_ct_integer_allocator_indexes_read = {
		void apply(inout bit<32> index, out bit<32> out_index) {
			out_index = index;
		}
	};

	Hash<fcfs_ct_hash_t>(HashAlgorithm_t.CRC32) fcfs_ct_hash_calculator;

	fcfs_ct_hash_t fcfs_ct_hash;
	action fcfs_ct_calc_hash() {
		fcfs_ct_hash = fcfs_ct_hash_calculator.get({
			meta.fcfs_ct_key_0,
			meta.fcfs_ct_key_1,
			meta.fcfs_ct_key_2,
			meta.fcfs_ct_key_3
		});
	}

	Register<bit<8>, _>(FCFS_CT_CACHE_CAPACITY, 0) fcfs_ct_liveness_reg;

	RegisterAction<bit<8>, fcfs_ct_hash_t, bool>(fcfs_ct_liveness_reg) fcfs_ct_liveness_check = {
		void apply(inout bit<8> is_alive, out bool out_is_alive) {
			if (is_alive == 0) {
				out_is_alive = false;
			} else {
				out_is_alive = true;
			}
		}
	};

	RegisterAction<bit<8>, fcfs_ct_hash_t, bool>(fcfs_ct_liveness_reg) fcfs_ct_liveness_check_and_set = {
		void apply(inout bit<8> is_alive, out bool out_is_alive) {
			if (is_alive == 0) {
				out_is_alive = false;
			} else {
				out_is_alive = true;
			}
			is_alive = 1;
		}
	};

	action fcfs_ct_liveness_check_execute() { meta.fcfs_ct_is_alive = fcfs_ct_liveness_check.execute(fcfs_ct_hash); }
	action fcfs_ct_liveness_check_and_set_execute() { meta.fcfs_ct_is_alive = fcfs_ct_liveness_check_and_set.execute(fcfs_ct_hash); }

	Register<bit<32>, _>(FCFS_CT_CACHE_CAPACITY, 0) fcfs_ct_keys_0;
	Register<bit<32>, _>(FCFS_CT_CACHE_CAPACITY, 0) fcfs_ct_keys_1;
	Register<bit<16>, _>(FCFS_CT_CACHE_CAPACITY, 0) fcfs_ct_keys_2;
	Register<bit<16>, _>(FCFS_CT_CACHE_CAPACITY, 0) fcfs_ct_keys_3;
	Register<bit<32>, _>(FCFS_CT_CACHE_CAPACITY, 0) fcfs_ct_values;

	RegisterAction<bit<32>, fcfs_ct_hash_t, bit<8>>(fcfs_ct_keys_0) fcfs_ct_read_key_0 = {
		void apply(inout bit<32> curr_key, out bit<8> key_match) {
			if (curr_key == meta.fcfs_ct_key_0) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, bit<8>>(fcfs_ct_keys_1) fcfs_ct_read_key_1 = {
		void apply(inout bit<32> curr_key, out bit<8> key_match) {
			if (curr_key == meta.fcfs_ct_key_1) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, bit<8>>(fcfs_ct_keys_2) fcfs_ct_read_key_2 = {
		void apply(inout bit<16> curr_key, out bit<8> key_match) {
			if (curr_key == meta.fcfs_ct_key_2) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, bit<8>>(fcfs_ct_keys_3) fcfs_ct_read_key_3 = {
		void apply(inout bit<16> curr_key, out bit<8> key_match) {
			if (curr_key == meta.fcfs_ct_key_3) {
				key_match = 1;
			} else {
				key_match = 0;
			}
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, bit<32>>(fcfs_ct_values) fcfs_ct_read_value = {
		void apply(inout bit<32> curr_value, out bit<32> out_value) {
			out_value = curr_value;
		}
	};

	action fcfs_ct_read_key_0_execute() { meta.fcfs_ct_key_fields_match = meta.fcfs_ct_key_fields_match + fcfs_ct_read_key_0.execute(fcfs_ct_hash); }
	action fcfs_ct_read_key_1_execute() { meta.fcfs_ct_key_fields_match = meta.fcfs_ct_key_fields_match + fcfs_ct_read_key_1.execute(fcfs_ct_hash); }
	action fcfs_ct_read_key_2_execute() { meta.fcfs_ct_key_fields_match = meta.fcfs_ct_key_fields_match + fcfs_ct_read_key_2.execute(fcfs_ct_hash); }
	action fcfs_ct_read_key_3_execute() { meta.fcfs_ct_key_fields_match = meta.fcfs_ct_key_fields_match + fcfs_ct_read_key_3.execute(fcfs_ct_hash); }
	action fcfs_ct_read_value_execute() { meta.fcfs_ct_value = fcfs_ct_read_value.execute(fcfs_ct_hash); }

	RegisterAction<bit<32>, fcfs_ct_hash_t, void>(fcfs_ct_keys_0) fcfs_ct_write_key_0 = {
		void apply(inout bit<32> curr_key) {
			curr_key = meta.fcfs_ct_key_0;
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, void>(fcfs_ct_keys_1) fcfs_ct_write_key_1 = {
		void apply(inout bit<32> curr_key) {
			curr_key = meta.fcfs_ct_key_1;
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, void>(fcfs_ct_keys_2) fcfs_ct_write_key_2 = {
		void apply(inout bit<16> curr_key) {
			curr_key = meta.fcfs_ct_key_2;
		}
	};

	RegisterAction<bit<16>, fcfs_ct_hash_t, void>(fcfs_ct_keys_3) fcfs_ct_write_key_3 = {
		void apply(inout bit<16> curr_key) {
			curr_key = meta.fcfs_ct_key_3;
		}
	};

	RegisterAction<bit<32>, fcfs_ct_hash_t, void>(fcfs_ct_values) fcfs_ct_write_value = {
		void apply(inout bit<32> curr_value) {
			curr_value = meta.fcfs_ct_value;
		}
	};

	action fcfs_ct_write_key_0_execute() { fcfs_ct_write_key_0.execute(fcfs_ct_hash); }
	action fcfs_ct_write_key_1_execute() { fcfs_ct_write_key_1.execute(fcfs_ct_hash); }
	action fcfs_ct_write_key_2_execute() { fcfs_ct_write_key_2.execute(fcfs_ct_hash); }
	action fcfs_ct_write_key_3_execute() { fcfs_ct_write_key_3.execute(fcfs_ct_hash); }
	action fcfs_ct_write_value_execute() { fcfs_ct_write_value.execute(fcfs_ct_hash); }

	apply {
		ingress_port_to_nf_dev.apply();

		if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
			nf_dev[15:0] = hdr.cpu.egress_dev;
		} else {
			if (ig_intr_md.ingress_port == 0) {
				// Read FCFS CT Operation
				meta.fcfs_ct_key_0 = hdr.ipv4.src_addr;
				meta.fcfs_ct_key_1 = hdr.ipv4.dst_addr;
				meta.fcfs_ct_key_2 = hdr.udp.src_port;
				meta.fcfs_ct_key_3 = hdr.udp.dst_port;
				bool found = fcfs_ct_table_0.apply().hit;
				if (!found && meta.fcfs_ct_is_alive) {
					fcfs_ct_calc_hash();
					fcfs_ct_liveness_check_execute();
					meta.fcfs_ct_key_fields_match = 0;
					fcfs_ct_read_key_0_execute();
					fcfs_ct_read_key_1_execute();
					fcfs_ct_read_key_2_execute();
					fcfs_ct_read_key_3_execute();
					if (meta.fcfs_ct_key_fields_match == 4) {
						found = true;
						fcfs_ct_read_value_execute();
					}
				}

				if (found) {
					hdr.udp.dst_port = meta.fcfs_ct_value[15:0];
					nf_dev = meta.dev;
				} else {
					fwd_op = fwd_op_t.DROP;
				}
			} else {
				// Read/Write FCFS CT Operation
				meta.fcfs_ct_key_0 = hdr.ipv4.src_addr;
				meta.fcfs_ct_key_1 = hdr.ipv4.dst_addr;
				meta.fcfs_ct_key_2 = hdr.udp.src_port;
				meta.fcfs_ct_key_3 = hdr.udp.dst_port;
				bool found = fcfs_ct_table_1.apply().hit;
				bool index_allocation_failed = false;
				bool collision_detected = false;
				if (!found) {
					fcfs_ct_calc_hash();
					fcfs_ct_liveness_check_and_set_execute();
					if (meta.fcfs_ct_is_alive) {
						meta.fcfs_ct_key_fields_match = 0;
						fcfs_ct_read_key_0_execute();
						fcfs_ct_read_key_1_execute();
						fcfs_ct_read_key_2_execute();
						fcfs_ct_read_key_3_execute();
						if (meta.fcfs_ct_key_fields_match != 4) {
							collision_detected = true;
						} else {
							found = true;
						}
					} else {
						fcfs_ct_write_key_0_execute();
						fcfs_ct_write_key_1_execute();
						fcfs_ct_write_key_2_execute();
						fcfs_ct_write_key_3_execute();
					}

					if (!meta.fcfs_ct_is_alive || collision_detected) {
						meta.fcfs_ct_integer_allocator_tail = fcfs_ct_integer_allocator_tail_read.execute(0);
						bit<32> fcfs_ct_integer_allocator_old_head = fcfs_ct_integer_allocator_head_inc.execute(0);
						if (fcfs_ct_integer_allocator_old_head != meta.fcfs_ct_integer_allocator_tail) {
							meta.fcfs_ct_value = fcfs_ct_integer_allocator_indexes_read.execute(fcfs_ct_integer_allocator_old_head);
							fcfs_ct_integer_allocator_pending_set.execute(meta.fcfs_ct_value);
						} else {
							index_allocation_failed = true;
						}
					}

					if (found) {
						fcfs_ct_read_value_execute();
					} else if (!collision_detected && !index_allocation_failed) {
						fcfs_ct_write_value_execute();
						ig_dprsr_md.digest_type = 1;
					}
				}

				// Exported symbols:
				//   index_allocation_failed: failed to allocate a new index
				//   collision_detected: write operation failed due to hash collision, send to the controller

				if (index_allocation_failed) {
					// Failed to allocate new index
					fwd_op = fwd_op_t.DROP;
				} else if (collision_detected) {
					fwd_op = fwd_op_t.FORWARD_TO_CPU;
					build_cpu_hdr(0);
					hdr.cpu.ingress_dev = meta.dev[15:0];
					hdr.cpu.new_index = meta.fcfs_ct_value;
				}
			}
		}

		forwarding_tbl.apply();
		ig_tm_md.bypass_egress = 1;
	}
}

control IngressDeparser(
	packet_out pkt,
	inout headers_t hdr,
	in metadata_t meta,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
	Digest<fcfs_ct_digest_t>() fcfs_ct_digest;

	apply {
		if (ig_dprsr_md.digest_type == 1) {
			fcfs_ct_digest.pack({
				meta.fcfs_ct_key_0,
				meta.fcfs_ct_key_1,
				meta.fcfs_ct_key_2,
				meta.fcfs_ct_key_3,
				meta.fcfs_ct_value
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
