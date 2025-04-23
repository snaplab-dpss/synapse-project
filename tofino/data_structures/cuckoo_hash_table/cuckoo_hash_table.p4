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
	INSERT	= 0x01,
	SWAP	= 0x02,
	SWAPPED = 0x03,
	DONE	= 0x04,
}

#define KEY_WIDTH 128
#define VAL_WIDTH 128

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
	bit<16> code_path;                  // Written by the data plane
	bit<16> egress_dev;                 // Written by the control plane
	bit<8> trigger_dataplane_execution; // Written by the control plane
}

header recirc_h {
	bit<16> code_path;
	bit<8>	op;
	bit<8>	recirc_cntr;
	key_t	key;
	val_t	val;
	bit<32> ts;
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

	key_t cur_key = 0;
	key_t table_1_key = 0;
	val_t cur_val = 0;
	val_t table_1_val = 0;
	bit<16> swapped_transient_val = 0;
	bit<32> entry_ts = 0;
	cuckoo_ops_t cuckoo_op = cuckoo_ops_t.LOOKUP;

	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cucko_hash_1;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cucko_hash_2;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) cucko_hash_2_r;

	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_1;
	action calc_cuckoo_hash_1() { cuckoo_hash_1	= cucko_hash_1.get({cur_key, HASH_SALT_1});	}

	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_2;
	action calc_cuckoo_hash_2() { cuckoo_hash_2	= cucko_hash_2.get({cur_key, HASH_SALT_2});	}

	bit<CUCKOO_IDX_WIDTH> cuckoo_hash_2_r;
	action calc_cuckoo_hash_2_r() { cuckoo_hash_2_r = cucko_hash_2_r.get({cur_key, HASH_SALT_2}); }

	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swap;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swap_2;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swapped;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swapped_2;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_swapped_3;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_1_ts;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_2_ts;

	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swap_transient;
	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swapped_transient;

	#define KEY_READ(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bool>(reg_##table##_k##lsb##_##msb) read_##table##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bool match) { \
				if (val == cur_key[msb:lsb]) { \
					match = true; \
				} else { \
					match = false; \
				} \
			} \
		};

	#define KEY_SWAP_1(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1##_k##lsb##_##msb) swap_1##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = cur_key[msb:lsb]; \
			} \
		};

	#define KEY_SWAP_2(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2##_k##lsb##_##msb) swap_2##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = table_1_key[msb:lsb]; \
			} \
		};

	#define VAL_READ(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_v##lsb##_##msb) read_##table##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
			} \
		};

	#define VAL_WRITE(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_v##lsb##_##msb) write_##table##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val) { \
				val = cur_val[msb:lsb]; \
			} \
		};

	#define VAL_SWAP_1(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1##_v##lsb##_##msb) swap_1##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = cur_val[msb:lsb]; \
			} \
		};

	#define VAL_SWAP_2(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2##_v##lsb##_##msb) swap_2##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val= table_1_val[msb:lsb]; \
			} \
		};

	KEY_READ(1, 31, 0)
	KEY_READ(1, 63, 32)
	KEY_READ(1, 95, 64)
	KEY_READ(1, 127, 96)

	KEY_READ(2, 31, 0)
	KEY_READ(2, 63, 32)
	KEY_READ(2, 95, 64)
	KEY_READ(2, 127, 96)

	// KEY_SWAP_1(31, 0)
	// KEY_SWAP_1(63, 32)
	// KEY_SWAP_1(95, 64)
	// KEY_SWAP_1(127, 96)

	// KEY_SWAP_2(31, 0)
	// KEY_SWAP_2(63, 32)
	// KEY_SWAP_2(95, 64)
	// KEY_SWAP_2(127, 96)

	VAL_READ(1, 31, 0)
	VAL_READ(1, 63, 32)
	VAL_READ(1, 95, 64)
	VAL_READ(1, 127, 96)

	VAL_READ(2, 31, 0)
	VAL_READ(2, 63, 32)
	VAL_READ(2, 95, 64)
	VAL_READ(2, 127, 96)

	VAL_WRITE(1, 31, 0)
	VAL_WRITE(1, 63, 32)
	VAL_WRITE(1, 95, 64)
	VAL_WRITE(1, 127, 96)

	VAL_WRITE(2, 31, 0)
	VAL_WRITE(2, 63, 32)
	VAL_WRITE(2, 95, 64)
	VAL_WRITE(2, 127, 96)

	// VAL_SWAP_1(31, 0)
	// VAL_SWAP_1(63, 32)
	// VAL_SWAP_1(95, 64)
	// VAL_SWAP_1(127, 96)

	// VAL_SWAP_2(31, 0)
	// VAL_SWAP_2(63, 32)
	// VAL_SWAP_2(95, 64)
	// VAL_SWAP_2(127, 96)

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts) table_1_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
				val = 0;
			} else {
				res = val;
				val = entry_ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts) table_2_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
				val = 0;
			} else {
				res = val;
				val = entry_ts;
			}
		}
	};

	// RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts) table_1_ts_swap = {
	// 	void apply(inout bit<32> val, out bit<32> res) {
	// 		res = val;
	// 		val = entry_ts;
	// 	}
	// };

	// RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts) table_2_ts_swap = {
	// 	void apply(inout bit<32> val, out bit<32> res) {
	// 		res = val;
	// 		val = entry_ts;
	// 	}
	// };

	// RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_read = {
	// 	void apply(inout bit<16> val, out bit<16> res) {
	// 		res = val;
	// 	}
	// };

	// RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bool>(swap_transient) swap_transient_conditional_inc = {
	// 	void apply(inout bit<16> val, out bool new_insertion) {
	// 		if (val <= swapped_transient_val) {
	// 			val = swapped_transient_val |+| 1;
	// 			new_insertion = true;
	// 		} else {
	// 			new_insertion = false;
	// 		}
	// 	}
	// };

	// RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_incr = {
	// 	void apply(inout bit<16> val) {
	// 		val = val |+| 1;
	// 	}
	// };

	// RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_incr = {
	// 	void apply(inout bit<16> val) {
	// 		val = val |+| 1;
	// 	}
	// };

	// RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_read = {
	// 	void apply(inout bit<16> val, out bit<16> res) {
	// 		res = val;
	// 	}
	// };

	apply {
		if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
			nf_dev[15:0] = hdr.cpu.egress_dev;
			hdr.cpu.setInvalid();
			trigger_forward = true;
		} else if (hdr.recirc.isValid()) {
			// TODO: recirculation
			/*
				The cuckoo packet can arrive here and be:
					- Lookup
					- Insert
					- Swap
			*/
		} else {
			ingress_port_to_nf_dev.apply();

			if (meta.dev == 0) {
				nf_dev[15:0] = hdr.kvs.client_port;
			} else {
				cur_key = hdr.kvs.key;
				cur_val = hdr.kvs.val;
				entry_ts = meta.time;

				calc_cuckoo_hash_1();

				bool key_match_1 = false;
				if (read_1_k0_31.execute(cuckoo_hash_1)) {
					if (read_1_k32_63.execute(cuckoo_hash_1)) {
						if (read_1_k64_95.execute(cuckoo_hash_1)) {
							if (read_1_k96_127.execute(cuckoo_hash_1)) {
								key_match_1 = true;
							}
						}
					}
				}

				bool cuckoo_table_1_lookup_success = false;

				if (key_match_1) {
					bit<32> stored_table_1_ts = table_1_ts_query_and_refresh.execute(cuckoo_hash_1);
					if (stored_table_1_ts != 0) {
						/*****************************************
						* BDD: on map_get hit with table 1
						*****************************************/
						
						cuckoo_table_1_lookup_success = true;
						if (hdr.kvs.op == KVS_OP_GET) {
							hdr.kvs.val[31:0]	= read_1_v0_31.execute(cuckoo_hash_1);
							hdr.kvs.val[63:32]	= read_1_v32_63.execute(cuckoo_hash_1);
							hdr.kvs.val[95:64]	= read_1_v64_95.execute(cuckoo_hash_1);
							hdr.kvs.val[127:96]	= read_1_v96_127.execute(cuckoo_hash_1);
						} else {
							write_1_v0_31.execute(cuckoo_hash_1);
							write_1_v32_63.execute(cuckoo_hash_1);
							write_1_v64_95.execute(cuckoo_hash_1);
							write_1_v96_127.execute(cuckoo_hash_1);
						}

						hdr.kvs.status = KVS_STATUS_HIT;
					}
				}

				if (!cuckoo_table_1_lookup_success) {
					calc_cuckoo_hash_2() ;

					bool key_match_2 = false;
					if (read_2_k0_31.execute(cuckoo_hash_2)) {
						if (read_2_k32_63.execute(cuckoo_hash_2)) {
							if (read_2_k64_95.execute(cuckoo_hash_2)) {
								if (read_2_k96_127.execute(cuckoo_hash_2)) {
									key_match_2 = true;
								}
							}
						}
					}

					bool cuckoo_table_2_lookup_success = false;

					if (key_match_2) {
						bit<32> stored_table_2_ts = table_2_ts_query_and_refresh.execute(cuckoo_hash_2);
						if (stored_table_2_ts != 0) {
							/*****************************************
							 * BDD: on map_get hit with table 2
							 *****************************************/
							cuckoo_table_2_lookup_success = true;
							if (hdr.kvs.op == KVS_OP_GET) {
								hdr.kvs.val[31:0]	= read_2_v0_31.execute(cuckoo_hash_2);
								hdr.kvs.val[63:32]	= read_2_v32_63.execute(cuckoo_hash_2);
								hdr.kvs.val[95:64]	= read_2_v64_95.execute(cuckoo_hash_2);
								hdr.kvs.val[127:96]	= read_2_v96_127.execute(cuckoo_hash_2);
							} else {
								write_2_v0_31.execute(cuckoo_hash_2);
								write_2_v32_63.execute(cuckoo_hash_2);
								write_2_v64_95.execute(cuckoo_hash_2);
								write_2_v96_127.execute(cuckoo_hash_2);
							}
							
							hdr.kvs.status = KVS_STATUS_HIT;
						}
					}

					if (!cuckoo_table_2_lookup_success) {
						/*****************************************
						* BDD: on map_get miss
						*****************************************/
						if (hdr.kvs.op == KVS_OP_GET) {
							nf_dev[15:0] = KVS_SERVER_NF_DEV;
						} else {
							/*****************************************
							* BDD: dchain_allocate_new_index + map_put, etc
							* EP: CuckooHashTableInsert
							*****************************************/
							cuckoo_op = cuckoo_ops_t.INSERT;

							hdr.recirc.setValid();
							hdr.recirc.code_path = 0;
							hdr.recirc.op = cuckoo_op;
							hdr.recirc.recirc_cntr = 0;
							hdr.recirc.key = cur_key;
							hdr.recirc.val = cur_val;
							hdr.recirc.ts = entry_ts;
							meta.recirculate = true;

							bit<BLOOM_IDX_WIDTH> swap_transient_idx = hash_swap.get({cur_key});

							if (hdr.cuckoo.recirc_cntr >= MAX_LOOPS) {
								// Assume the insertion packet was lost.
								swapped_transient_incr.execute(swap_transient_idx);
								hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
							} else {
								ig_md.swapped_transient_val	= swapped_transient_read.execute(swap_transient_idx);
								bool new_insertion = swap_transient_conditional_inc.execute(swap_transient_idx);

								if (!new_insertion) {
									// However, if another pkt matching the same bloom idx is already recirculating, the current pkt will be sent back as a LOOKUP.
									hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
								} else {
									hdr.cuckoo.recirc_cntr = 0;
								}
							}
						}
					}
				}
			}
		}

		/*****************************************
		* Cuckoo Pervasive Logic
		* -> Bloom Filter
		*****************************************/

		if (cuckoo_op == cuckoo_ops_t.INSERT) {
			bit<BLOOM_IDX_WIDTH> swap_transient_idx = hash_swap.get({cur_key});

			if (hdr.cuckoo.recirc_cntr >= MAX_LOOPS) {
				// Assume the insertion packet was lost.
				swapped_transient_incr.execute(swap_transient_idx);
				hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
			} else {
				ig_md.swapped_transient_val	= swapped_transient_read.execute(swap_transient_idx);
				bool new_insertion = swap_transient_conditional_inc.execute(swap_transient_idx);

				if (!new_insertion) {
					// However, if another pkt matching the same bloom idx is already recirculating, the current pkt will be sent back as a LOOKUP.
					hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
				} else {
					hdr.cuckoo.recirc_cntr = 0;
				}
			}
		} else if (hdr.cuckoo.op == cuckoo_ops_t.DONE && ig_md.was_insert_op) {
			swapped_transient_incr.execute(hash_swapped.get({ig_md.cur_key}));
		} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
			swapped_transient_incr.execute(hash_swapped.get({ig_md.cur_key}));
			swap_transient_incr.execute(hash_swap_2.get({hdr.cuckoo.key}));
		} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAPPED) {
			if (ig_md.has_next_swap == 0) {
				swapped_transient_incr.execute(hash_swapped_2.get({hdr.cuckoo.key}));
				hdr.cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				swapped_transient_incr.execute(hash_swapped_3.get({ig_md.swapped_key}));
				swap_transient_incr.execute(hash_swap_2.get({hdr.cuckoo.key}));
				hdr.cuckoo.op = cuckoo_ops_t.SWAP;
			}
		}

		if (ig_md.send_to_kvs_server) {
			set_out_port(KVS_SERVER_PORT);
		} else if (hdr.cuckoo.op == cuckoo_ops_t.DONE) {
			set_out_port((bit<9>)hdr.kv.port);
		} else if (hdr.cuckoo.recirc_cntr >= MAX_LOOPS) {
			set_out_port(KVS_SERVER_PORT);
		} else {
			hdr.cuckoo.recirc_cntr = hdr.cuckoo.recirc_cntr + 1;
			select_recirc_port.apply();
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