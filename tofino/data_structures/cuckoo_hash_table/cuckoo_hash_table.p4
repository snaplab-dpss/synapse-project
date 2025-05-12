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

const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<8>  IP_PROTO_TCP = 6;
const bit<8>  IP_PROTO_UDP = 17;
const bit<16> KVS_PORT = 670;

const bit<16> KVS_SERVER_NF_DEV = 136;

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
	SWAPPED = 0x04,
	DONE	= 0x05,
}

#define KEY_WIDTH 32
#define VAL_WIDTH 32

typedef bit<KEY_WIDTH> key_t;
typedef bit<VAL_WIDTH> val_t;

// Entry Timeout Expiration (units of 65536 ns).
#define ENTRY_TIMEOUT 256 // 16ms
#define MAX_LOOPS 10
#define CUCKOO_ENTRIES 8192
#define CUCKOO_IDX_WIDTH 13
#define BLOOM_ENTRIES CUCKOO_ENTRIES
#define BLOOM_IDX_WIDTH CUCKOO_IDX_WIDTH

header cpu_h {
	bit<16>	code_path;                   // Written by the data plane
	bit<16>	egress_dev;                  // Written by the control plane
	bit<8>	trigger_dataplane_execution; // Written by the control plane
}

header recirc_h {
	bit<16>	 code_path;
}

header cuckoo_h {
	bit<8>  op;
	bit<8>  recirc_cntr;
	bit<32> ts;
	key_t   key;
	val_t   val;
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
	bit<32> dev;
	bit<32> time;
	bool recirculate;
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

		meta.dev = 0;
		meta.time = ig_intr_md.ingress_mac_tstamp[47:16];
		meta.recirculate = false;

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
			0: parse_cuckoo;
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

control CuckooHashTable(
	in bit<32> now,
	inout cuckoo_h cuckoo,
	out bool has_next_swap,
	out bool success
) {
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cucko_hash_1;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cucko_hash_2;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cucko_hash_2_r;

	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_1;
	action calc_cuckoo_hash_1() { cuckoo_hash_1	= cucko_hash_1.get({cuckoo.key, HASH_SALT_1});	}

	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_2;
	action calc_cuckoo_hash_2() { cuckoo_hash_2	= cucko_hash_2.get({cuckoo.key, HASH_SALT_2});	}

	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_2_r;
	action calc_cuckoo_hash_2_r() { cuckoo_hash_2_r = cucko_hash_2_r.get({cuckoo.key, HASH_SALT_2}); }

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_1_ts;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_2_ts;

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bool>(reg_1_k) read_1_k = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bool>(reg_2_k) read_2_k = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_k) swap_1_k = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_k) swap_2_k = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_v) read_1_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_v) read_2_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_v) write_1_v = {
		void apply(inout bit<32> val) {
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_v) write_2_v = {
		void apply(inout bit<32> val) {
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_v) swap_1_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_v) swap_2_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts) table_1_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
				val = 0;
			} else {
				res = val;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts) table_2_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
				val = 0;
			} else {
				res = val;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts) table_1_ts_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts) table_2_ts_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	action ts_diff(in bit<32> ts, out bit<32> diff) {
		diff = now - ts;
	}

	apply {
		calc_cuckoo_hash_1();

		if (cuckoo.op == cuckoo_ops_t.LOOKUP || cuckoo.op == cuckoo_ops_t.UPDATE) {
			bool key_match_1 = read_1_k.execute(cuckoo_hash_1);
			success = false;

			if (key_match_1) {
				bit<32> stored_table_1_ts = table_1_ts_query_and_refresh.execute(cuckoo_hash_1);
				if (stored_table_1_ts != 0) {
					/*****************************************
					* BDD: on map_get hit with table 1
					*****************************************/
					
					success = true;

					if (cuckoo.op == cuckoo_ops_t.LOOKUP) {
						cuckoo.val = read_1_v.execute(cuckoo_hash_1);
					} else {
						write_1_v.execute(cuckoo_hash_1);
					}
				}
			}

			if (!success) {
				calc_cuckoo_hash_2() ;

				bool key_match_2 = read_2_k.execute(cuckoo_hash_2);

				if (key_match_2) {
					bit<32> stored_table_2_ts = table_2_ts_query_and_refresh.execute(cuckoo_hash_2);
					if (stored_table_2_ts != 0) {
						/*****************************************
						* BDD: on map_get hit with table 2
						*****************************************/
						
						success = true;

						if (cuckoo.op == cuckoo_ops_t.LOOKUP) {
							cuckoo.val = read_2_v.execute(cuckoo_hash_2);
						} else {
							write_2_v.execute(cuckoo_hash_2);
						}
					}
				}
			}
		} else if (cuckoo.op == cuckoo_ops_t.INSERT || cuckoo.op == cuckoo_ops_t.SWAP) {
			cuckoo.key = swap_1_k.execute(cuckoo_hash_1);
			cuckoo.ts = table_1_ts_swap.execute(cuckoo_hash_1);
			cuckoo.val = swap_1_v.execute(cuckoo_hash_1);

			bit<32> table_1_ts_diff;
			ts_diff(cuckoo.ts, table_1_ts_diff);

			// Compare the previously stored values against the current pkt's.
			// If the previous stored values haven't expired and they don't match
			// the current pkt's, we will swap them to Table 2.

			// The previous Table 1 entry was occupied and not yet expired,
			// so we'll swap it to Table 2.
			if (table_1_ts_diff < ENTRY_TIMEOUT) {
				calc_cuckoo_hash_2_r();
				
				cuckoo.key = swap_2_k.execute(cuckoo_hash_2_r);
				cuckoo.ts = table_2_ts_swap.execute(cuckoo_hash_2_r);
				cuckoo.val = swap_2_v.execute(cuckoo_hash_2_r);

				bit<32> table_2_ts_diff;
				ts_diff(cuckoo.ts, table_2_ts_diff);

				// The previous Table 2 entry was occupied and not yet expired,
				// so we'll recirculate and swap it to Table 1.
				if (table_2_ts_diff < ENTRY_TIMEOUT) {
					has_next_swap = true;

					if (cuckoo.op == cuckoo_ops_t.INSERT) {
						// First (re)circulation for the current pkt.
						// Store the swap entry values and change the op to SWAP.
						// The packet will be recirculated later.
						cuckoo.op = cuckoo_ops_t.SWAP;
					} else if (cuckoo.op == cuckoo_ops_t.SWAP) {
						// The current pkt is already a mirrored pkt.
						// Store the new swap entry values and change the op to SWAPPED.
						// The packet will be recirculated later.
						cuckoo.op = cuckoo_ops_t.SWAPPED;
					}
				} else {
					// The previous Table 2 entry was expired/replaced.
					// In its place is now the cur pkt.
					if (cuckoo.op == cuckoo_ops_t.INSERT) {
						// Send the current pkt to the bloom filter with op == DONE, so it'll be sent out.
						cuckoo.op = cuckoo_ops_t.DONE;
					} else if (cuckoo.op == cuckoo_ops_t.SWAP) {
						// Send the current pkt to the bloom filter with op == SWAPPED, to update the transient.
						cuckoo.op = cuckoo_ops_t.SWAPPED;
					}
				}
			} else {
				// The previous Table 1 entry was expired/replaced.
				// In its place is now the cur pkt.
				if (cuckoo.op == cuckoo_ops_t.INSERT) {
					// Send the current pkt to the bloom filter with op == DONE, so it'll be sent out.
					cuckoo.op = cuckoo_ops_t.DONE;
				} else if (cuckoo.op == cuckoo_ops_t.SWAP) {
					// Send the current pkt to the bloom filter with op == SWAPPED, to update the transient.
					cuckoo.op = cuckoo_ops_t.SWAPPED;
				}
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

	action send_to_controller(bit<16> code_path) {
		hdr.cpu.setValid();
		hdr.cpu.code_path = code_path;
		fwd(CPU_PCIE_PORT);
	}

	action swap(inout bit<8> a, inout bit<8> b) {
		bit<8> tmp = a;
		a = b;
		b = tmp;
	}

	#define bswap32(x) (x[7:0] ++ x[15:8] ++ x[23:16] ++ x[31:24])
	#define bswap16(x) (x[7:0] ++ x[15:8])

	action set_ingress_dev(bit<32> nf_dev) { meta.dev = nf_dev; }
	table ingress_port_to_nf_dev {
		key = { ig_intr_md.ingress_port: exact; }
		actions = { set_ingress_dev; drop; }
		size = 32;
		const default_action = drop();
	}

	bool trigger_forward = false;
	bit<32> nf_dev = 0;
	table forward_nf_dev {
		key = { nf_dev: exact; }
		actions = { fwd; }
		size = 64;
	}


	bit<16> swapped_transient_val = 0;
	bool was_insert_op = false;
	bool has_next_swap = false;

	CuckooHashTable() cuckoo_hash_table;

	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swap;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swap_2;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swapped;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swapped_2;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swapped_3;

	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_old_key;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_new_key;

	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swap_transient;
	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swapped_transient;

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bool>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bool transient) {
			if (val <= swapped_transient_val) {
				transient = true;
			} else {
				transient = false;
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

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	action set_out_port(bit<9> out_port) {
		ig_tm_md.ucast_egress_port = out_port;
	}

	table cuckoo_select_recirc_port {
		key = {
			ig_intr_md.ingress_port[8:7] : exact;
		}
		actions = {
			set_out_port;
		}
		size = 4;
		const entries = {
			0 : set_out_port(RECIRCULATION_PORT_0);
			1 : set_out_port(RECIRCULATION_PORT_1);
			#if __TARGET_TOFINO__ == 2
			2 : set_out_port(RECIRCULATION_PORT_2);
			3 : set_out_port(RECIRCULATION_PORT_3);
			#endif
		}
	}

	action build_cuckoo_hdr() {
		hdr.cuckoo.setValid();
		hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
		hdr.cuckoo.recirc_cntr = 0;
		hdr.cuckoo.ts = meta.time;
		hdr.cuckoo.key = hdr.kvs.key;
		hdr.cuckoo.val = hdr.kvs.val;
		hdr.cuckoo.old_key = hdr.kvs.key;
	}

	apply {
		if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
			nf_dev[15:0] = hdr.cpu.egress_dev;
			hdr.cpu.setInvalid();
			trigger_forward = true;
		} else if (hdr.recirc.isValid()) {
			if (hdr.recirc.code_path == 0) {
				if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
					was_insert_op = true;
				}

				bool success;
				cuckoo_hash_table.apply(
					meta.time,
					hdr.cuckoo,
					has_next_swap,
					success
				);

				if (success) {
					/*****************************************
					* BDD: on map_get success
					*****************************************/
					if (hdr.kvs.op == KVS_OP_GET) {
						hdr.kvs.val = hdr.cuckoo.val;
					}
					hdr.kvs.status = KVS_STATUS_HIT;
				} else {
					if (hdr.kvs.op == KVS_OP_GET) {
						nf_dev[15:0] = KVS_SERVER_NF_DEV;
					} else {
						/*****************************************
						* BDD: dchain_allocate_new_index + map_put, etc
						* EP: CuckooHashTableInsert
						*****************************************/
						hdr.cuckoo.op = cuckoo_ops_t.INSERT;

						hdr.recirc.setValid();
						hdr.recirc.code_path = 0;
						meta.recirculate = true;
					}
				}
			}
		} else {
			ingress_port_to_nf_dev.apply();

			if (meta.dev == 0) {
				nf_dev[15:0] = hdr.kvs.client_port;
			} else {
				build_cuckoo_hdr();

				if (hdr.kvs.op == KVS_OP_PUT) {
					hdr.cuckoo.op = cuckoo_ops_t.UPDATE;
				}

				bool success;
				cuckoo_hash_table.apply(
					meta.time,
					hdr.cuckoo,
					has_next_swap,
					success
				);

				if (success) {
					/*****************************************
					* BDD: on map_get success
					*****************************************/
					if (hdr.kvs.op == KVS_OP_GET) {
						hdr.kvs.val = hdr.cuckoo.val;
					}
					hdr.kvs.status = KVS_STATUS_HIT;
				} else {
					if (hdr.kvs.op == KVS_OP_GET) {
						nf_dev[15:0] = KVS_SERVER_NF_DEV;
					} else {
						/*****************************************
						* BDD: dchain_allocate_new_index + map_put, etc
						* EP: CuckooHashTableInsert
						*****************************************/
						hdr.cuckoo.op = cuckoo_ops_t.INSERT;

						hdr.recirc.setValid();
						hdr.recirc.code_path = 0;
						meta.recirculate = true;
					}
				}
			}
		}

		/*****************************************
		* Cuckoo Pervasive Logic
		* -> Bloom Filter
		*****************************************/

		bit<BLOOM_IDX_WIDTH> old_key_hash = hash_old_key.get(hdr.cuckoo.old_key);
		bit<BLOOM_IDX_WIDTH> new_key_hash = hash_new_key.get(hdr.cuckoo.key);

		if (hdr.cuckoo.op == cuckoo_ops_t.DONE) {
			if (was_insert_op) {
				swapped_transient_incr.execute(old_key_hash);
			}
		} else if (hdr.cuckoo.op == cuckoo_ops_t.LOOKUP) {
			swapped_transient_val = swapped_transient_read.execute(old_key_hash);
			bool transient = swap_transient_read.execute(old_key_hash);

			if (transient) {
				hdr.recirc.setValid();
				hdr.recirc.code_path = 0;
				meta.recirculate = true;
			}
		} else if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
			if (hdr.cuckoo.recirc_cntr >= MAX_LOOPS) {
				// Assume the insertion packet was lost.
				swapped_transient_incr.execute(old_key_hash);

				hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
			} else {
				swapped_transient_val = swapped_transient_read.execute(old_key_hash);
				bool new_insertion = swap_transient_conditional_inc.execute(old_key_hash);

				if (!new_insertion) {
					// However, if another pkt matching the same bloom idx is already recirculating, the current pkt will be sent back as a LOOKUP.
					hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
				} else {
					hdr.cuckoo.recirc_cntr = 0;
				}
			}
		} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
			swapped_transient_incr.execute(old_key_hash);
			swap_transient_incr.execute(new_key_hash);
		} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAPPED) {
			swapped_transient_incr.execute(old_key_hash);
			if (has_next_swap) {
				swap_transient_incr.execute(new_key_hash);
				hdr.cuckoo.op = cuckoo_ops_t.SWAP;
			} else {
				hdr.cuckoo.op = cuckoo_ops_t.DONE;
			}
		}

		if (hdr.cuckoo.isValid() && hdr.cuckoo.op != cuckoo_ops_t.DONE) {
			hdr.cuckoo.recirc_cntr = hdr.cuckoo.recirc_cntr + 1;
			cuckoo_select_recirc_port.apply();
		}

		// =============================================================================

		if (trigger_forward) {
			forward_nf_dev.apply();
		}

		if (!meta.recirculate) {
			hdr.recirc.setInvalid();
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