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

enum bit<8> cuckoo_ops_t {
	LOOKUP	= 0x00,
	UPDATE  = 0x01,
	INSERT	= 0x02,
	SWAP	= 0x03,
	DONE	= 0x04,
}

enum bit<2> fwd_op_t {
	FORWARD_NF_DEV	= 0,
	FORWARD_TO_CPU  = 1,
	RECIRCULATE 	= 2,
	DROP 			= 3,
}

typedef bit<32> key_t;
typedef bit<32> val_t;

// Entry Timeout Expiration (units of 65536 ns).
#define ENTRY_TIMEOUT 16384 // 1 s
#define MAX_LOOPS 4
#define CUCKOO_ENTRIES 4096
#define CUCKOO_IDX_WIDTH 12
#define BLOOM_ENTRIES 65536
#define BLOOM_IDX_WIDTH 16

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

header cuckoo_h {
	bit<8>  op;
	bit<8>  recirc_cntr;
	bit<32> ts;
	key_t   key;
	val_t   val;
	bit<8>  old_op;
	key_t   old_key;
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

header kvs_h {
	bit<8>	op;
	key_t	key;
	val_t	val;
	bit<8>	status;
	bit<16>	client_port;
}

struct headers_t {
	cpu_h cpu;
	recirc_h recirc;
	cuckoo_h cuckoo;
	ethernet_h ethernet;
	ipv4_h ipv4;
	udp_h udp;
	kvs_h kvs;
}

struct metadata_t {
	bit<16> ingress_port;
	bit<32> dev;
	bit<32> time;
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
			0xca: parse_cuckoo; // <- Special cuckoo parsing!
			0xfe: parse_cuckoo; // <- Special cuckoo parsing!
			default: parser_init;
		}
	}

	state parse_cuckoo {
		pkt.extract(hdr.cuckoo);
		transition parser_init;
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
		transition select(hdr.udp.src_port, hdr.udp.dst_port) {
			(KVS_PORT, _) : parse_kvs;
			(_, KVS_PORT) : parse_kvs;
			default: reject;
		}
	}

	state parse_kvs {
		pkt.extract(hdr.kvs);
		transition accept;
	}
}

control CuckooHashTable(in bit<32> now, inout cuckoo_h cuckoo, out bool success) {
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_1;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2_r;

	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_1 = 0;
	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_2 = 0;
	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_2_r = 0;

	action calc_cuckoo_hash_1() { cuckoo_hash_1	= cuckoo_hash_func_1.get({HASH_SALT_1, cuckoo.key}); }
	action calc_cuckoo_hash_2() { cuckoo_hash_2	= cuckoo_hash_func_2.get({HASH_SALT_2, cuckoo.key}); }
	action calc_cuckoo_hash_2_r() { cuckoo_hash_2_r = cuckoo_hash_func_2_r.get({HASH_SALT_2, cuckoo.key}); }

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_k_1;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_k_2;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_v_1;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_v_2;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_ts_1;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_ts_2;

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bool>(reg_k_1) k_1_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bool>(reg_k_2) k_2_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_k_1) k_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_k_2) k_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_v_1) v_1_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_v_2) v_2_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_v_1) v_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_v_2) v_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bool>(reg_ts_1) ts_1_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > ENTRY_TIMEOUT) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bool>(reg_ts_2) ts_2_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > ENTRY_TIMEOUT) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_ts_1) ts_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_ts_2) ts_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	action ts_diff(in bit<32> ts, out bit<32> diff) {
		diff = now - ts;
	}

	apply {
		cuckoo.old_op = cuckoo.op;
		cuckoo.old_key = cuckoo.key;

		calc_cuckoo_hash_1();
		calc_cuckoo_hash_2();
		calc_cuckoo_hash_2_r();

		success = false;
		if (cuckoo.op == cuckoo_ops_t.LOOKUP || cuckoo.op == cuckoo_ops_t.UPDATE) {
			if (k_1_read.execute(cuckoo_hash_1)) {
				if (ts_1_query_and_refresh.execute(cuckoo_hash_1)) {
					cuckoo.val = v_1_read_or_update.execute(cuckoo_hash_1);
					cuckoo.op = cuckoo_ops_t.DONE;
					success = true;
				}
			}
			if (!success) {
				if (k_2_read.execute(cuckoo_hash_2)) {
					if (ts_2_query_and_refresh.execute(cuckoo_hash_2)) {
						cuckoo.val = v_2_read_or_update.execute(cuckoo_hash_2);
						cuckoo.op = cuckoo_ops_t.DONE;
						success = true;
					}
				}
			}
		} else {
			cuckoo.key = k_1_swap.execute(cuckoo_hash_1);
			cuckoo.ts = ts_1_swap.execute(cuckoo_hash_1);
			cuckoo.val = v_1_swap.execute(cuckoo_hash_1);

			bit<32> ts_1_diff;
			ts_diff(cuckoo.ts, ts_1_diff);

			if (ts_1_diff < ENTRY_TIMEOUT) {
				cuckoo.key = k_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.ts = ts_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.val = v_2_swap.execute(cuckoo_hash_2_r);

				bit<32> ts_2_diff;
				ts_diff(cuckoo.ts, ts_2_diff);

				if (ts_2_diff < ENTRY_TIMEOUT) {
					cuckoo.op = cuckoo_ops_t.SWAP;
				} else {
					cuckoo.op = cuckoo_ops_t.DONE;
					success = true;
				}
			} else {
				cuckoo.op = cuckoo_ops_t.DONE;
				success = true;
			}
		}
	}
}

control CuckooHashBloomFilter(inout cuckoo_h cuckoo, out fwd_op_t fwd_op) {
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_old_key;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_new_key;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_old_key_2;

	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES, 0) swap_transient;
	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES, 0) swapped_transient;

	bit<16> swapped_transient_val = 0;

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bool>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bool stable) {
			if (val <= swapped_transient_val) {
				stable = true;
			} else {
				stable = false;
			}
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bool>(swap_transient) swap_transient_conditional_inc = {
		void apply(inout bit<16> val, out bool new_insertion) {
			if (val <= swapped_transient_val) {
				val = swapped_transient_val |+| 1;
				new_insertion = true;
			} else {
				new_insertion = false;
			}
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	apply {
		bit<BLOOM_IDX_WIDTH> old_key_hash = hash_old_key.get({cuckoo.old_key});
		bit<BLOOM_IDX_WIDTH> old_key_hash_2 = hash_old_key_2.get({cuckoo.old_key});

		if (cuckoo.op == cuckoo_ops_t.DONE) {
			if (cuckoo.old_op == cuckoo_ops_t.INSERT || cuckoo.old_op == cuckoo_ops_t.SWAP) {
				swapped_transient_inc.execute(old_key_hash);
			}
		} else if (cuckoo.op == cuckoo_ops_t.LOOKUP) {
			swapped_transient_val = swapped_transient_read.execute(old_key_hash);
			if (swap_transient_read.execute(old_key_hash)) {
				// Cache miss.
				cuckoo.op = cuckoo_ops_t.DONE;
			}
		} else if (cuckoo.op == cuckoo_ops_t.UPDATE) {
			if (cuckoo.recirc_cntr >= MAX_LOOPS) {
				// Give up and send to KVS server.
				swapped_transient_inc.execute(old_key_hash_2);
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				swapped_transient_val = swapped_transient_read.execute(old_key_hash_2);
				bool new_insertion = swap_transient_conditional_inc.execute(old_key_hash_2);
				if (new_insertion) {
					cuckoo.op = cuckoo_ops_t.INSERT;
					cuckoo.recirc_cntr = 0;
				}
			}
		} else if (cuckoo.op == cuckoo_ops_t.SWAP) {
			swapped_transient_inc.execute(old_key_hash_2);
			if (cuckoo.recirc_cntr >= MAX_LOOPS) {
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				swap_transient_inc.execute(hash_new_key.get({cuckoo.key}));
			}
		}

		if (cuckoo.op != cuckoo_ops_t.DONE) {
			if (cuckoo.recirc_cntr >= MAX_LOOPS) {
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				fwd_op = fwd_op_t.RECIRCULATE;
				cuckoo.recirc_cntr = cuckoo.recirc_cntr + 1;
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
		hdr.cuckoo.setInvalid();
		fwd(CPU_PCIE_PORT);
	}

	action fwd_nf_dev(bit<16> port) {
		hdr.cpu.setInvalid();
		hdr.recirc.setInvalid();
		hdr.cuckoo.setInvalid();
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

	CuckooHashTable() cuckoo_hash_table;
	CuckooHashBloomFilter() cuckoo_bloom_filter;

	action build_cuckoo_hdr() {
		hdr.cuckoo.setValid();
		hdr.cuckoo.recirc_cntr = 0;
		hdr.cuckoo.ts = meta.time;
		hdr.cuckoo.key = hdr.kvs.key;
		hdr.cuckoo.val = hdr.kvs.val;
	}

	action recirculate() {
		hdr.recirc.setValid();
		hdr.recirc.ingress_port = meta.ingress_port;
		hdr.recirc.dev = meta.dev;
		hdr.recirc.code_path = 0xca;
	}

	apply {
		ingress_port_to_nf_dev.apply();

		if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
			nf_dev[15:0] = hdr.cpu.egress_dev;
		} else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
		} else {
			if (meta.dev == KVS_SERVER_NF_DEV) {
				nf_dev[15:0] = hdr.kvs.client_port;
			} else {
				hdr.kvs.client_port = meta.dev[15:0];

				if (!hdr.cuckoo.isValid()) {
					build_cuckoo_hdr();
					if (hdr.kvs.op == KVS_OP_GET) {
						hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
					} else {
						hdr.cuckoo.op = cuckoo_ops_t.UPDATE;
					}
				}

				bool success;
				cuckoo_hash_table.apply(meta.time, hdr.cuckoo, success);
				cuckoo_bloom_filter.apply(hdr.cuckoo, fwd_op);

				hdr.kvs.key = hdr.cuckoo.key;
				hdr.kvs.val = hdr.cuckoo.val;

				if (hdr.cuckoo.op != cuckoo_ops_t.DONE) {
					recirculate();
				} else {
					if (hdr.kvs.op == KVS_OP_GET) {
						if (success) {
							// BDD: on map_get success
							hdr.kvs.status = KVS_STATUS_HIT;
							nf_dev = meta.dev;
						} else {
							nf_dev = KVS_SERVER_NF_DEV;
						}
					} else {
						if (success) {
							// BDD: on dchain_allocate_new_index success
							hdr.kvs.status = KVS_STATUS_HIT;
							nf_dev = meta.dev;
						} else {
							nf_dev = KVS_SERVER_NF_DEV;
						}
					}
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