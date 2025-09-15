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

const bit<16> CUCKOO_CODE_PATH = 0xffff;

enum bit<8> cuckoo_ops_t {
  LOOKUP  = 0x00,
  UPDATE  = 0x01,
  INSERT  = 0x02,
  SWAP    = 0x03,
  DONE    = 0x04,
}

enum bit<2> fwd_op_t {
  FORWARD_NF_DEV  = 0,
  FORWARD_TO_CPU  = 1,
  RECIRCULATE     = 2,
  DROP            = 3,
}

header cpu_h {
  bit<16> code_path;                  // Written by the data plane
  bit<16> egress_dev;                 // Written by the control plane
  bit<8> trigger_dataplane_execution; // Written by the control plane

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;

};

header cuckoo_h {
  bit<8>  op;
  bit<8>  recirc_cntr;
  bit<32> ts;
  bit<32> key;
  bit<32> val;
  bit<8>  old_op;
  bit<32> old_key;
}

header hdr0_h {
  bit<48> data0;
  bit<48> data1;
  bit<16> data2;
}
header hdr1_h {
  bit<72> data0;
  bit<24> data1;
  bit<32> data2;
  bit<32> data3;
}
header hdr2_h {
  bit<16> data0;
  bit<16> data1;
  bit<32> data2;
}
header hdr3_h {
  bit<8> data0;
  bit<32> data1;
  bit<32> data2;
  bit<8> data3;
  bit<16> data4;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;

}

struct synapse_egress_metadata_t {

}

parser TofinoIngressParser(
  packet_in pkt,
  out ingress_intrinsic_metadata_t ig_intr_md
) {
  state start {
    pkt.extract(ig_intr_md);
    transition select(ig_intr_md.resubmit_flag) {
      1: parse_resubmit;
      0: parse_port_metadata;
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
  out synapse_ingress_headers_t hdr,
  out synapse_ingress_metadata_t meta,
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
      CUCKOO_CODE_PATH: parse_cuckoo;
      default: parser_init;
    }
  }

  state parse_cuckoo {
    pkt.extract(hdr.cuckoo);
    transition parser_init;
  }

  state parser_init {
    pkt.extract(hdr.hdr0);
    transition parser_6;
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr0.data2) {
      16w0x0800: parser_7;
      default: parser_66;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr1);
    transition parser_8;
  }
  state parser_66 {
    transition reject;
  }
  state parser_8 {
    transition parser_8_0;
  }
  state parser_8_0 {
    transition select (hdr.hdr1.data1[23:16]) {
      8w0x11: parser_9;
      default: parser_64;
    }
  }
  state parser_9 {
    pkt.extract(hdr.hdr2);
    transition parser_10;
  }
  state parser_64 {
    transition reject;
  }
  state parser_10 {
    transition parser_10_0;
  }
  state parser_10_0 {
    transition select (hdr.hdr2.data0) {
      16w0x029e: parser_11;
      default: parser_10_1;
    }
  }
  state parser_10_1 {
    transition select (hdr.hdr2.data1) {
      16w0x029e: parser_11;
      default: parser_61;
    }
  }
  state parser_11 {
    pkt.extract(hdr.hdr3);
    transition parser_57;
  }
  state parser_61 {
    transition reject;
  }
  state parser_57 {
    transition accept;
  }

}

// Entry Timeout Expiration (units of 65536 ns).
#define CUCKOO_ENTRY_TIMEOUT 16384 // 1 s
#define CUCKOO_MAX_LOOPS 4

const bit<32> CUCKOO_HASH_SALT_1 = 0xfbc31fc7;
const bit<32> CUCKOO_HASH_SALT_2 = 0x2681580b;

control CuckooHashTable(in bit<32> now, inout cuckoo_h cuckoo, out bool success) {
	Hash<bit<12>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_1;
	Hash<bit<12>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2;
	Hash<bit<12>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2_r;

	bit<12> cuckoo_hash_1 = 0;
	bit<12> cuckoo_hash_2 = 0;
	bit<12> cuckoo_hash_2_r = 0;

	action calc_cuckoo_hash_1() { cuckoo_hash_1	= cuckoo_hash_func_1.get({CUCKOO_HASH_SALT_1, cuckoo.key}); }
	action calc_cuckoo_hash_2() { cuckoo_hash_2	= cuckoo_hash_func_2.get({CUCKOO_HASH_SALT_2, cuckoo.key}); }
	action calc_cuckoo_hash_2_r() { cuckoo_hash_2_r = cuckoo_hash_func_2_r.get({CUCKOO_HASH_SALT_2, cuckoo.key}); }

	Register<bit<32>, bit<12>>(4096, 0) reg_k_1;
	Register<bit<32>, bit<12>>(4096, 0) reg_k_2;

	Register<bit<32>, bit<12>>(4096, 0) reg_v_1;
	Register<bit<32>, bit<12>>(4096, 0) reg_v_2;

	Register<bit<32>, bit<12>>(4096, 0) reg_ts_1;
	Register<bit<32>, bit<12>>(4096, 0) reg_ts_2;

	RegisterAction<bit<32>, bit<12>, bool>(reg_k_1) k_1_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<12>, bool>(reg_k_2) k_2_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_k_1) k_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_k_2) k_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_v_1) v_1_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_v_2) v_2_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_v_1) v_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_v_2) v_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit<12>, bool>(reg_ts_1) ts_1_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > CUCKOO_ENTRY_TIMEOUT) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<12>, bool>(reg_ts_2) ts_2_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > CUCKOO_ENTRY_TIMEOUT) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_ts_1) ts_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	RegisterAction<bit<32>, bit<12>, bit<32>>(reg_ts_2) ts_2_swap = {
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

			if (ts_1_diff < CUCKOO_ENTRY_TIMEOUT) {
				cuckoo.key = k_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.ts = ts_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.val = v_2_swap.execute(cuckoo_hash_2_r);

				bit<32> ts_2_diff;
				ts_diff(cuckoo.ts, ts_2_diff);

				if (ts_2_diff < CUCKOO_ENTRY_TIMEOUT) {
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
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash_old_key;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash_new_key;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash_old_key_2;

	Register<bit<16>, bit<16>>(65536, 0) swap_transient;
	Register<bit<16>, bit<16>>(65536, 0) swapped_transient;

	bit<16> swapped_transient_val = 0;

	RegisterAction<bit<16>, bit<16>, bool>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bool stable) {
			if (val <= swapped_transient_val) {
				stable = true;
			} else {
				stable = false;
			}
		}
	};

	RegisterAction<bit<16>, bit<16>, bool>(swap_transient) swap_transient_conditional_inc = {
		void apply(inout bit<16> val, out bool new_insertion) {
			if (val <= swapped_transient_val) {
				val = swapped_transient_val |+| 1;
				new_insertion = true;
			} else {
				new_insertion = false;
			}
		}
	};

	RegisterAction<bit<16>, bit<16>, bit<16>>(swap_transient) swap_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<16>, bit<16>>(swapped_transient) swapped_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<16>, bit<16>>(swapped_transient) swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	apply {
		bit<16> old_key_hash = hash_old_key.get({cuckoo.old_key});
		bit<16> old_key_hash_2 = hash_old_key_2.get({cuckoo.old_key});

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
			if (cuckoo.recirc_cntr >= CUCKOO_MAX_LOOPS) {
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
			if (cuckoo.recirc_cntr >= CUCKOO_MAX_LOOPS) {
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				swap_transient_inc.execute(hash_new_key.get({cuckoo.key}));
			}
		}

		if (cuckoo.op != cuckoo_ops_t.DONE) {
			if (cuckoo.recirc_cntr >= CUCKOO_MAX_LOOPS) {
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				fwd_op = fwd_op_t.RECIRCULATE;
				cuckoo.recirc_cntr = cuckoo.recirc_cntr + 1;
			}
		}
	}
}


control Ingress(
  inout synapse_ingress_headers_t hdr,
  inout synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_t ig_intr_md,
  in    ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
  inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
  inout ingress_intrinsic_metadata_for_tm_t ig_tm_md
) {
  action drop() {
    ig_dprsr_md.drop_ctl = 1;
  }
  
  action fwd(bit<16> port) {
    ig_tm_md.ucast_egress_port = port[8:0];
  }

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

  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

  bit<1> diff_sign_bit;
  action calculate_diff_32b(bit<32> a, bit<32> b) { diff_sign_bit = (a - b)[31:31]; }
  action calculate_diff_16b(bit<16> a, bit<16> b) { diff_sign_bit = (a - b)[15:15]; }
  action calculate_diff_8b(bit<8> a, bit<8> b) { diff_sign_bit = (a - b)[7:7]; }

  action build_cpu_hdr(bit<16> code_path) {
    hdr.cpu.setValid();
    hdr.cpu.code_path = code_path;
    fwd(CPU_PCIE_PORT);
  }

  action build_recirc_hdr(bit<16> code_path) {
    hdr.recirc.setValid();
    hdr.recirc.ingress_port = meta.ingress_port;
    hdr.recirc.dev = meta.dev;
    hdr.recirc.code_path = code_path;
  }

  action build_cuckoo_hdr(bit<32> key, bit<32> val) {
		hdr.cuckoo.setValid();
		hdr.cuckoo.recirc_cntr = 0;
		hdr.cuckoo.ts = meta.time;
		hdr.cuckoo.key = key;
		hdr.cuckoo.val = val;
	}

  CuckooHashTable() cuckoo_hash_table;
  CuckooHashBloomFilter() cuckoo_bloom_filter;

  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:Ignore
      // BDD node 4:expire_items_single_map(chain:(w64 1073971976), vector:(w64 1073937624), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073923800))
      // EP node  4:ParserExtraction
      // BDD node 5:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 14), chunk:(w64 1074039328)[ -> (w64 1073760160)])
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  14:Then
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  26:ParserExtraction
        // BDD node 7:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 20), chunk:(w64 1074040064)[ -> (w64 1073760416)])
        if(hdr.hdr1.isValid()) {
          // EP node  52:ParserCondition
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  53:Then
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  74:ParserExtraction
          // BDD node 9:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 8), chunk:(w64 1074040720)[ -> (w64 1073760672)])
          if(hdr.hdr2.isValid()) {
            // EP node  117:ParserCondition
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  118:Then
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  175:ParserExtraction
            // BDD node 11:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 12), chunk:(w64 1074041464)[ -> (w64 1073760928)])
            if(hdr.hdr3.isValid()) {
              // EP node  250:If
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              if ((16w0x0000) != (meta.dev[15:0])){
                // EP node  251:Then
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  502:CuckooHashTableReadWrite
                // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                if (!hdr.cuckoo.isValid()) {
                  build_cuckoo_hdr(hdr.hdr3.data1, hdr.hdr3.data2);
                  if ((8w0x01) == (hdr.hdr3.data0)) {
                    hdr.cuckoo.op = cuckoo_ops_t.UPDATE;
                  } else {
                    hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
                  }
                }
                bool cuckoo_hash_table_1073923800_13_success0;
                cuckoo_hash_table.apply(meta.time, hdr.cuckoo, cuckoo_hash_table_1073923800_13_success0);
                cuckoo_bloom_filter.apply(hdr.cuckoo, fwd_op);
                if (hdr.cuckoo.op != cuckoo_ops_t.DONE) {
                  build_recirc_hdr(CUCKOO_CODE_PATH);
                } else {
                  // EP node  503:If
                  // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                  if ((8w0x01) == (hdr.hdr3.data0)){
                    // EP node  504:Then
                    // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                    // EP node  506:If
                    // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                    if (cuckoo_hash_table_1073923800_13_success0){
                      // EP node  507:Then
                      // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                      // EP node  2164:ModifyHeader
                      // BDD node 42:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
                      hdr.hdr3.data3 = 8w0x01;
                      // EP node  2293:ModifyHeader
                      // BDD node 43:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  2425:ModifyHeader
                      // BDD node 44:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  2560:ModifyHeader
                      // BDD node 45:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  2698:Forward
                      // BDD node 46:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                    } else {
                      // EP node  508:Else
                      // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                      // EP node  1598:ModifyHeader
                      // BDD node 18:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
                      hdr.hdr3.data4[15:8] = meta.dev[7:0];
                      hdr.hdr3.data4[7:0] = meta.dev[15:8];
                      // EP node  2090:Forward
                      // BDD node 22:FORWARD
                      nf_dev[15:0] = 16w0x0000;
                    }
                  } else {
                    // EP node  505:Else
                    // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                    // EP node  509:If
                    // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                    if (cuckoo_hash_table_1073923800_13_success0){
                      // EP node  510:Then
                      // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                      // EP node  1078:ModifyHeader
                      // BDD node 48:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (Concat w72 (Read w8 (w32 3) vector_data_384) (Concat w64 (Read w8 (w32 2) vector_data_384) (Concat w56 (Read w8 (w32 1) vector_data_384) (Concat w48 (Read w8 (w32 0) vector_data_384) (ReadLSB w40 (w32 768) packet_chunks))))))))])
                      hdr.hdr3.data2 = hdr.cuckoo.val;
                      hdr.hdr3.data3 = 8w0x01;
                      // EP node  1186:ModifyHeader
                      // BDD node 49:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  1297:ModifyHeader
                      // BDD node 50:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  1411:ModifyHeader
                      // BDD node 51:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  1528:Forward
                      // BDD node 52:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                    } else {
                      // EP node  511:Else
                      // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                      // EP node  610:ModifyHeader
                      // BDD node 33:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
                      hdr.hdr3.data4[15:8] = meta.dev[7:0];
                      hdr.hdr3.data4[7:0] = meta.dev[15:8];
                      // EP node  1018:Forward
                      // BDD node 37:FORWARD
                      nf_dev[15:0] = 16w0x0000;
                    }
                  }
                }
              } else {
                // EP node  252:Else
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  4372:Forward
                // BDD node 57:FORWARD
                nf_dev[15:0] = bswap16(hdr.hdr3.data4);
              }
            }
            // EP node  119:Else
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  4140:ParserReject
            // BDD node 61:DROP
          }
          // EP node  54:Else
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  3779:ParserReject
          // BDD node 64:DROP
        }
        // EP node  15:Else
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  3296:ParserReject
        // BDD node 66:DROP
      }

    }

    forwarding_tbl.apply();
    ig_tm_md.bypass_egress = 1;
  }
}

control IngressDeparser(
  packet_out pkt,
  inout synapse_ingress_headers_t hdr,
  in    synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
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
  out synapse_egress_headers_t hdr,
  out synapse_egress_metadata_t eg_md,
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
  inout synapse_egress_headers_t hdr,
  inout synapse_egress_metadata_t eg_md,
  in    egress_intrinsic_metadata_t eg_intr_md,
  in    egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
  inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
  inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
  apply {}
}

control EgressDeparser(
  packet_out pkt,
  inout synapse_egress_headers_t hdr,
  in    synapse_egress_metadata_t eg_md,
  in    egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md
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
