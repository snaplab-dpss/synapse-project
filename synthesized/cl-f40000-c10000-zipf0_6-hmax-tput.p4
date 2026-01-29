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
  bit<32> cached_insert_success0;
  bit<32> vector_table_1074092960_141_get_value_param0;
  @padding bit<31> pad_hit0;
  bool hit0;
  bit<32> cms_1074080384_min0;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> vector_table_1074092960_141_get_value_param0;
  @padding bit<7> pad_hit0;
  bool hit0;
  bit<32> cms_1074080384_min0;

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
  bit<96> data0;
  bit<16> data1;
}
header hdr1_h {
  bit<72> data0;
  bit<8> data1;
  bit<16> data2;
  bit<64> data3;
}
header hdr2_h {
  bit<32> data0;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  bit<32> key_32b_0;
  bit<32> fcfs_cs_1074047984_key_32b_0;
  bit<32> fcfs_cs_1074047984_key_32b_1;
  bit<32> fcfs_cs_1074047984_key_32b_2;
  bit<64> key_64b_0;

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
    transition parser_137;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_138;
      default: parser_187;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr1);
    transition parser_139;
  }
  state parser_187 {
    transition reject;
  }
  state parser_139 {
    transition parser_139_0;
  }
  state parser_139_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_140;
      8w0x11: parser_140;
      default: parser_185;
    }
  }
  state parser_140 {
    pkt.extract(hdr.hdr2);
    transition parser_182;
  }
  state parser_185 {
    transition reject;
  }
  state parser_182 {
    transition accept;
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

  bit<32> vector_table_1074092960_141_get_value_param0 = 32w0;
  action vector_table_1074092960_141_get_value(bit<32> _vector_table_1074092960_141_get_value_param0) {
    vector_table_1074092960_141_get_value_param0 = _vector_table_1074092960_141_get_value_param0;
  }

  table vector_table_1074092960_141 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074092960_141_get_value;
    }
    size = 36;
  }

  Hash<bit<12>>(HashAlgorithm_t.CRC32) fcfs_cs_1074047984_hash_144;
  Hash<bit<12>>(HashAlgorithm_t.CRC32) fcfs_cs_1074047984_hash_149;
  Register<bit<32>,_>(4096, 0) fcfs_cs_1074047984_reg_liveness;
  RegisterAction<bit<32>, bit<12>, bool>(fcfs_cs_1074047984_reg_liveness) fcfs_cs_1074047984_reg_liveness_query_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
      } else {
        was_alive = true;
      }
    }
  };

  RegisterAction<bit<32>, bit<12>, bool>(fcfs_cs_1074047984_reg_liveness) fcfs_cs_1074047984_reg_liveness_query_and_refresh_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
      } else {
        was_alive = true;
      }
      alarm = meta.time + 16384;
    }
  };

  Register<bit<32>,_>(4096, 0) fcfs_cs_1074047984_reg_key_0;
  RegisterAction<bit<32>, bit<12>, void>(fcfs_cs_1074047984_reg_key_0) fcfs_cs_1074047984_reg_key_0_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_cs_1074047984_key_32b_0;
    }
  };

  RegisterAction<bit<32>, bit<12>, bit<8>>(fcfs_cs_1074047984_reg_key_0) fcfs_cs_1074047984_reg_key_0_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_cs_1074047984_key_32b_0) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<32>,_>(4096, 0) fcfs_cs_1074047984_reg_key_1;
  RegisterAction<bit<32>, bit<12>, void>(fcfs_cs_1074047984_reg_key_1) fcfs_cs_1074047984_reg_key_1_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_cs_1074047984_key_32b_1;
    }
  };

  RegisterAction<bit<32>, bit<12>, bit<8>>(fcfs_cs_1074047984_reg_key_1) fcfs_cs_1074047984_reg_key_1_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_cs_1074047984_key_32b_1) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<32>,_>(4096, 0) fcfs_cs_1074047984_reg_key_2;
  RegisterAction<bit<32>, bit<12>, void>(fcfs_cs_1074047984_reg_key_2) fcfs_cs_1074047984_reg_key_2_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_cs_1074047984_key_32b_2;
    }
  };

  RegisterAction<bit<32>, bit<12>, bit<8>>(fcfs_cs_1074047984_reg_key_2) fcfs_cs_1074047984_reg_key_2_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_cs_1074047984_key_32b_2) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  table fcfs_cs_1074047984_table_144 {
    key = {
      meta.fcfs_cs_1074047984_key_32b_0: exact;
      meta.fcfs_cs_1074047984_key_32b_1: exact;
      meta.fcfs_cs_1074047984_key_32b_2: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<12> fcfs_cs_1074047984_hash_144_value;
  action fcfs_cs_1074047984_hash_144_calc() {
    fcfs_cs_1074047984_hash_144_value = fcfs_cs_1074047984_hash_144.get({
      meta.fcfs_cs_1074047984_key_32b_0,
      meta.fcfs_cs_1074047984_key_32b_1,
      meta.fcfs_cs_1074047984_key_32b_2
      });
  }
  bit<8> match_counter0 = 0;
  action fcfs_cs_1074047984_check_key_0_144() {
    match_counter0 = match_counter0 + fcfs_cs_1074047984_reg_key_0_check_value.execute(fcfs_cs_1074047984_hash_144_value);
  }
  action fcfs_cs_1074047984_check_key_1_144() {
    match_counter0 = match_counter0 + fcfs_cs_1074047984_reg_key_1_check_value.execute(fcfs_cs_1074047984_hash_144_value);
  }
  action fcfs_cs_1074047984_check_key_2_144() {
    match_counter0 = match_counter0 + fcfs_cs_1074047984_reg_key_2_check_value.execute(fcfs_cs_1074047984_hash_144_value);
  }
  Register<bit<32>,_>(1024, 0) cms_1074080384_row_0;
  Register<bit<32>,_>(1024, 0) cms_1074080384_row_1;
  Register<bit<32>,_>(1024, 0) cms_1074080384_row_2;
  Register<bit<32>,_>(1024, 0) cms_1074080384_row_3;

  bit<10> cms_1074080384_hash_0_value;
  bit<10> cms_1074080384_hash_1_value;
  bit<10> cms_1074080384_hash_2_value;
  bit<10> cms_1074080384_hash_3_value;

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_0) cms_1074080384_row_0_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_0_inc_and_read_value;
  action cms_1074080384_row_0_inc_and_read_execute() {
    cms_1074080384_row_0_inc_and_read_value = cms_1074080384_row_0_inc_and_read.execute(cms_1074080384_hash_0_value);
  }

  RegisterAction<bit<32>, bit<10>, void>(cms_1074080384_row_0) cms_1074080384_row_0_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080384_row_0_inc_execute() {
    cms_1074080384_row_0_inc.execute(cms_1074080384_hash_0_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_0) cms_1074080384_row_0_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_0_read_value;
  action cms_1074080384_row_0_read_execute() {
    cms_1074080384_row_0_read_value = cms_1074080384_row_0_read.execute(cms_1074080384_hash_0_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_1) cms_1074080384_row_1_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_1_inc_and_read_value;
  action cms_1074080384_row_1_inc_and_read_execute() {
    cms_1074080384_row_1_inc_and_read_value = cms_1074080384_row_1_inc_and_read.execute(cms_1074080384_hash_1_value);
  }

  RegisterAction<bit<32>, bit<10>, void>(cms_1074080384_row_1) cms_1074080384_row_1_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080384_row_1_inc_execute() {
    cms_1074080384_row_1_inc.execute(cms_1074080384_hash_1_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_1) cms_1074080384_row_1_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_1_read_value;
  action cms_1074080384_row_1_read_execute() {
    cms_1074080384_row_1_read_value = cms_1074080384_row_1_read.execute(cms_1074080384_hash_1_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_2) cms_1074080384_row_2_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_2_inc_and_read_value;
  action cms_1074080384_row_2_inc_and_read_execute() {
    cms_1074080384_row_2_inc_and_read_value = cms_1074080384_row_2_inc_and_read.execute(cms_1074080384_hash_2_value);
  }

  RegisterAction<bit<32>, bit<10>, void>(cms_1074080384_row_2) cms_1074080384_row_2_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080384_row_2_inc_execute() {
    cms_1074080384_row_2_inc.execute(cms_1074080384_hash_2_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_2) cms_1074080384_row_2_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_2_read_value;
  action cms_1074080384_row_2_read_execute() {
    cms_1074080384_row_2_read_value = cms_1074080384_row_2_read.execute(cms_1074080384_hash_2_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_3) cms_1074080384_row_3_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_3_inc_and_read_value;
  action cms_1074080384_row_3_inc_and_read_execute() {
    cms_1074080384_row_3_inc_and_read_value = cms_1074080384_row_3_inc_and_read.execute(cms_1074080384_hash_3_value);
  }

  RegisterAction<bit<32>, bit<10>, void>(cms_1074080384_row_3) cms_1074080384_row_3_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080384_row_3_inc_execute() {
    cms_1074080384_row_3_inc.execute(cms_1074080384_hash_3_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080384_row_3) cms_1074080384_row_3_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080384_row_3_read_value;
  action cms_1074080384_row_3_read_execute() {
    cms_1074080384_row_3_read_value = cms_1074080384_row_3_read.execute(cms_1074080384_hash_3_value);
  }

  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080384_hash_0_1907;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080384_hash_1_1907;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080384_hash_2_1907;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080384_hash_3_1907;

  action cms_1074080384_hash_0_1907_calc_1907() {
    cms_1074080384_hash_0_value = cms_1074080384_hash_0_1907.get({
      meta.key_64b_0,
      32w0xfbc31fc7
    });
  }
  action cms_1074080384_hash_1_1907_calc_1907() {
    cms_1074080384_hash_1_value = cms_1074080384_hash_1_1907.get({
      meta.key_64b_0,
      32w0x2681580b
    });
  }
  action cms_1074080384_hash_2_1907_calc_1907() {
    cms_1074080384_hash_2_value = cms_1074080384_hash_2_1907.get({
      meta.key_64b_0,
      32w0x486d7e2f
    });
  }
  action cms_1074080384_hash_3_1907_calc_1907() {
    cms_1074080384_hash_3_value = cms_1074080384_hash_3_1907.get({
      meta.key_64b_0,
      32w0x1f3a2b4d
    });
  }
  bit<12> fcfs_cs_1074047984_hash_149_value;
  action fcfs_cs_1074047984_hash_149_calc() {
    fcfs_cs_1074047984_hash_149_value = fcfs_cs_1074047984_hash_149.get({
      meta.fcfs_cs_1074047984_key_32b_0,
      meta.fcfs_cs_1074047984_key_32b_1,
      meta.fcfs_cs_1074047984_key_32b_2
      });
  }
  bit<16> vector_table_1074110176_160_get_value_param0 = 16w0;
  action vector_table_1074110176_160_get_value(bit<16> _vector_table_1074110176_160_get_value_param0) {
    vector_table_1074110176_160_get_value_param0 = _vector_table_1074110176_160_get_value_param0;
  }

  table vector_table_1074110176_160 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074110176_160_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074110176_171_get_value_param0 = 16w0;
  action vector_table_1074110176_171_get_value(bit<16> _vector_table_1074110176_171_get_value_param0) {
    vector_table_1074110176_171_get_value_param0 = _vector_table_1074110176_171_get_value_param0;
  }

  table vector_table_1074110176_171 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074110176_171_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074110176_177_get_value_param0 = 16w0;
  action vector_table_1074110176_177_get_value(bit<16> _vector_table_1074110176_177_get_value_param0) {
    vector_table_1074110176_177_get_value_param0 = _vector_table_1074110176_177_get_value_param0;
  }

  table vector_table_1074110176_177 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074110176_177_get_value;
    }
    size = 36;
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  2191:FCFSCachedSetInsert
        // BDD node 149:dchain_allocate_new_index
        meta.fcfs_cs_1074047984_key_32b_0 = hdr.hdr1.data3[63:32];
        meta.fcfs_cs_1074047984_key_32b_1 = hdr.hdr1.data3[31:0];
        meta.fcfs_cs_1074047984_key_32b_2 = hdr.hdr2.data0;
        bit<32> cached_insert_success0 = 0;
        fcfs_cs_1074047984_hash_149_calc();
        bool fcfs_cs_is_alive1 = fcfs_cs_1074047984_reg_liveness_query_and_refresh_timestamp.execute(fcfs_cs_1074047984_hash_149_value);
        if (!fcfs_cs_is_alive1) {
          fcfs_cs_1074047984_reg_key_0_write.execute(fcfs_cs_1074047984_hash_149_value);
          fcfs_cs_1074047984_reg_key_1_write.execute(fcfs_cs_1074047984_hash_149_value);
          fcfs_cs_1074047984_reg_key_2_write.execute(fcfs_cs_1074047984_hash_149_value);
          cached_insert_success0 = 1;
        }
        // EP node  2192:If
        // BDD node 149:dchain_allocate_new_index
        if ((cached_insert_success0) != (32w0x00000000)){
          // EP node  2193:Then
          // BDD node 149:dchain_allocate_new_index
          // EP node  2399:VectorTableLookup
          // BDD node 160:vector_borrow
          meta.key_32b_0 = meta.dev;
          vector_table_1074110176_160.apply();
          // EP node  2598:Ignore
          // BDD node 161:vector_return
          // EP node  3469:Forward
          // BDD node 165:FORWARD
          nf_dev[15:0] = vector_table_1074110176_160_get_value_param0;
        } else {
          // EP node  2194:Else
          // BDD node 149:dchain_allocate_new_index
          // EP node  2339:SendToController
          // BDD node 266:tofino_force_send_to_controller
          fwd_op = fwd_op_t.FORWARD_TO_CPU;
          build_cpu_hdr(2339);
          hdr.cpu.cached_insert_success0 = cached_insert_success0;
          hdr.cpu.vector_table_1074092960_141_get_value_param0 = hdr.recirc.vector_table_1074092960_141_get_value_param0;
          hdr.cpu.hit0 = hdr.recirc.hit0;
          hdr.cpu.cms_1074080384_min0 = hdr.recirc.cms_1074080384_min0;
          hdr.cpu.dev = meta.dev;
        }
      }

    } else {
      // EP node  0:Ignore
      // BDD node 134:expire_items_single_map
      // EP node  4:Ignore
      // BDD node 135:cms_periodic_cleanup
      // EP node  11:ParserExtraction
      // BDD node 136:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  24:ParserCondition
        // BDD node 137:if
        // EP node  25:Then
        // BDD node 137:if
        // EP node  39:ParserExtraction
        // BDD node 138:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  69:ParserCondition
          // BDD node 139:if
          // EP node  70:Then
          // BDD node 139:if
          // EP node  103:ParserExtraction
          // BDD node 140:packet_borrow_next_chunk
          if(hdr.hdr2.isValid()) {
            // EP node  174:VectorTableLookup
            // BDD node 141:vector_borrow
            meta.key_32b_0 = meta.dev;
            vector_table_1074092960_141.apply();
            // EP node  394:FCFSCachedSetRead
            // BDD node 144:map_get
            meta.fcfs_cs_1074047984_key_32b_0 = hdr.hdr1.data3[63:32];
            meta.fcfs_cs_1074047984_key_32b_1 = hdr.hdr1.data3[31:0];
            meta.fcfs_cs_1074047984_key_32b_2 = hdr.hdr2.data0;
            bool hit0 = fcfs_cs_1074047984_table_144.apply().hit;
            fcfs_cs_1074047984_hash_144_calc();
            bool fcfs_cs_is_alive0 = fcfs_cs_1074047984_reg_liveness_query_timestamp.execute(fcfs_cs_1074047984_hash_144_value);
            if (!hit0 && fcfs_cs_is_alive0) {
              fcfs_cs_1074047984_check_key_0_144();
              fcfs_cs_1074047984_check_key_1_144();
              fcfs_cs_1074047984_check_key_2_144();
              if (match_counter0 == 3) {
                hit0 = true;
              }
            }
            // EP node  646:Ignore
            // BDD node 142:vector_return
            // EP node  692:If
            // BDD node 143:if
            if ((32w0x00000000) == (vector_table_1074092960_141_get_value_param0)){
              // EP node  693:Then
              // BDD node 143:if
              // EP node  745:If
              // BDD node 145:if
              if (!hit0){
                // EP node  746:Then
                // BDD node 145:if
                // EP node  1907:CMSIncAndQuery
                // BDD node 146:cms_increment
                meta.key_64b_0 = hdr.hdr1.data3;
                cms_1074080384_hash_0_1907_calc_1907();
                cms_1074080384_hash_1_1907_calc_1907();
                cms_1074080384_hash_2_1907_calc_1907();
                cms_1074080384_hash_3_1907_calc_1907();
                cms_1074080384_row_0_inc_and_read_execute();
                cms_1074080384_row_1_inc_and_read_execute();
                cms_1074080384_row_2_inc_and_read_execute();
                cms_1074080384_row_3_inc_and_read_execute();
                bit<32> cms_1074080384_min0 = cms_1074080384_row_0_inc_and_read_value;
                cms_1074080384_min0 = min(cms_1074080384_min0, cms_1074080384_row_1_inc_and_read_value);
                cms_1074080384_min0 = min(cms_1074080384_min0, cms_1074080384_row_2_inc_and_read_value);
                cms_1074080384_min0 = min(cms_1074080384_min0, cms_1074080384_row_3_inc_and_read_value);
                // EP node  2005:If
                // BDD node 148:if
                if ((cms_1074080384_min0) <= (32w0x0001ffff)){
                  // EP node  2006:Then
                  // BDD node 148:if
                  // EP node  2109:Recirculate
                  // BDD node 149:dchain_allocate_new_index
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(0);
                  hdr.recirc.vector_table_1074092960_141_get_value_param0 = vector_table_1074092960_141_get_value_param0;
                  hdr.recirc.hit0 = hit0;
                  hdr.recirc.cms_1074080384_min0 = cms_1074080384_min0;
                } else {
                  // EP node  2007:Else
                  // BDD node 148:if
                  // EP node  5025:Drop
                  // BDD node 169:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              } else {
                // EP node  747:Else
                // BDD node 145:if
                // EP node  1278:Ignore
                // BDD node 170:dchain_rejuvenate_index
                // EP node  1364:VectorTableLookup
                // BDD node 171:vector_borrow
                meta.key_32b_0 = meta.dev;
                vector_table_1074110176_171.apply();
                // EP node  1453:Ignore
                // BDD node 172:vector_return
                // EP node  1850:Forward
                // BDD node 176:FORWARD
                nf_dev[15:0] = vector_table_1074110176_171_get_value_param0;
              }
            } else {
              // EP node  694:Else
              // BDD node 143:if
              // EP node  833:VectorTableLookup
              // BDD node 177:vector_borrow
              meta.key_32b_0 = meta.dev;
              vector_table_1074110176_177.apply();
              // EP node  905:Ignore
              // BDD node 178:vector_return
              // EP node  1230:Forward
              // BDD node 182:FORWARD
              nf_dev[15:0] = vector_table_1074110176_177_get_value_param0;
            }
          }
          // EP node  71:Else
          // BDD node 139:if
          // EP node  4689:ParserReject
          // BDD node 185:DROP
        }
        // EP node  26:Else
        // BDD node 137:if
        // EP node  4168:ParserReject
        // BDD node 187:DROP
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
