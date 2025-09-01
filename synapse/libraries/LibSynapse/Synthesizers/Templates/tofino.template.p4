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
/*@{CPU_HEADER}@*/
}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
/*@{RECIRCULATION_HEADER}@*/
};

header cuckoo_h {
  bit<8> op;
  bit<8> recirc_cntr;
  bit<32> ts;
  bit<8> old_op;
  /*@{CUCKOO_HEADER_EXTRA_FIELDS}@*/
}

/*@{CUSTOM_HEADERS}@*/

struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
/*@{INGRESS_HEADERS}@*/
}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
/*@{INGRESS_METADATA}@*/
}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
/*@{EGRESS_HEADERS}@*/
}

struct synapse_egress_metadata_t {
/*@{EGRESS_METADATA}@*/
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

/*@{INGRESS_PARSER}@*/
}

control CuckooHashTable(in bit<32> now, inout cuckoo_h cuckoo, out bool success) {
	Hash<bit</*@{CUCKOO_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_1;
	Hash<bit</*@{CUCKOO_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2;
	Hash<bit</*@{CUCKOO_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2_r;

	bit</*@{CUCKOO_IDX_WIDTH}@*/> cuckoo_hash_1 = 0;
	bit</*@{CUCKOO_IDX_WIDTH}@*/> cuckoo_hash_2 = 0;
	bit</*@{CUCKOO_IDX_WIDTH}@*/> cuckoo_hash_2_r = 0;

	action calc_cuckoo_hash_1() { cuckoo_hash_1	= cuckoo_hash_func_1.get({HASH_SALT_1, cuckoo.key}); }
	action calc_cuckoo_hash_2() { cuckoo_hash_2	= cuckoo_hash_func_2.get({HASH_SALT_2, cuckoo.key}); }
	action calc_cuckoo_hash_2_r() { cuckoo_hash_2_r = cuckoo_hash_func_2_r.get({HASH_SALT_2, cuckoo.key}); }

	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_k_1;
	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_k_2;

	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_v_1;
	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_v_2;

	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_ts_1;
	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_ts_2;

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_k_1) k_1_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_k_2) k_2_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_k_1) k_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_k_2) k_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_1) v_1_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_2) v_2_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_1) v_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_2) v_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_ts_1) ts_1_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > /*@{CUCKOO_ENTRY_TIMEOUT}@*/) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_ts_2) ts_2_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > /*@{CUCKOO_ENTRY_TIMEOUT}@*/) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_ts_1) ts_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_ts_2) ts_2_swap = {
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

			if (ts_1_diff < /*@{CUCKOO_ENTRY_TIMEOUT}@*/) {
				cuckoo.key = k_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.ts = ts_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.val = v_2_swap.execute(cuckoo_hash_2_r);

				bit<32> ts_2_diff;
				ts_diff(cuckoo.ts, ts_2_diff);

				if (ts_2_diff < /*@{CUCKOO_ENTRY_TIMEOUT}@*/) {
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
	Hash<bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) hash_old_key;
	Hash<bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) hash_new_key;
	Hash<bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) hash_old_key_2;

	Register<bit<16>, bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>>(/*@{CUCKOO_BLOOM_ENTRIES}@*/, 0) swap_transient;
	Register<bit<16>, bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>>(/*@{CUCKOO_BLOOM_ENTRIES}@*/, 0) swapped_transient;

	bit<16> swapped_transient_val = 0;

	RegisterAction<bit<16>, bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>, bool>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bool stable) {
			if (val <= swapped_transient_val) {
				stable = true;
			} else {
				stable = false;
			}
		}
	};

	RegisterAction<bit<16>, bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>, bool>(swap_transient) swap_transient_conditional_inc = {
		void apply(inout bit<16> val, out bool new_insertion) {
			if (val <= swapped_transient_val) {
				val = swapped_transient_val |+| 1;
				new_insertion = true;
			} else {
				new_insertion = false;
			}
		}
	};

	RegisterAction<bit<16>, bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>, bit<16>>(swap_transient) swap_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>, bit<16>>(swapped_transient) swapped_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/>, bit<16>>(swapped_transient) swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	apply {
		bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/> old_key_hash = hash_old_key.get({cuckoo.old_key});
		bit</*@{CUCKOO_BLOOM_IDX_WIDTH}@*/> old_key_hash_2 = hash_old_key_2.get({cuckoo.old_key});

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
			if (cuckoo.recirc_cntr >= /*@{CUCKOO_MAX_LOOPS}@*/) {
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
			if (cuckoo.recirc_cntr >= /*@{CUCKOO_MAX_LOOPS}@*/) {
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				swap_transient_inc.execute(hash_new_key.get({cuckoo.key}));
			}
		}

		if (cuckoo.op != cuckoo_ops_t.DONE) {
			if (cuckoo.recirc_cntr >= /*@{CUCKOO_MAX_LOOPS}@*/) {
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

/*@{INGRESS_CONTROL}@*/
  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
/*@{INGRESS_CONTROL_APPLY_RECIRC}@*/
    } else {
/*@{INGRESS_CONTROL_APPLY}@*/
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
/*@{INGRESS_DEPARSER}@*/
  apply {
/*@{INGRESS_DEPARSER_APPLY}@*/
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
