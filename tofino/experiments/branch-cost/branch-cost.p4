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
  bit<32> vector_table_1074085544_139_get_value_param0;
  @padding bit<31> pad_hit0;
  bool hit0;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> vector_table_1074085544_139_get_value_param0;
  bit<32> fcfs_ct_1074053136_table_163_get_value_param0;
  @padding bit<7> pad_hit0;
  bool hit0;

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
  bit<32> data3;
  bit<32> data4;
}
header hdr2_h {
  bit<16> data0;
  bit<16> data1;
}


struct tessera_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

}

struct tessera_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  bit<32> key_32b_0;
  bit<32> fcfs_ct_1074053136_key_32b_0;
  bit<32> fcfs_ct_1074053136_key_32b_1;
  bit<16> fcfs_ct_1074053136_key_16b_2;
  bit<16> fcfs_ct_1074053136_key_16b_3;

}

struct tessera_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;

}

struct tessera_egress_metadata_t {

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
  out tessera_ingress_headers_t hdr,
  out tessera_ingress_metadata_t meta,
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
    transition parser_135;
  }
  state parser_135 {
    transition parser_135_0;
  }
  state parser_135_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_136;
      default: parser_193;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_193 {
    transition reject;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_138;
      8w0x11: parser_138;
      default: parser_191;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_162;
  }
  state parser_191 {
    transition reject;
  }
  state parser_162 {
    transition accept;
  }

}



control Ingress(
  inout tessera_ingress_headers_t hdr,
  inout tessera_ingress_metadata_t meta,
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

  bit<32> vector_table_1074085544_139_get_value_param0 = 32w0;
  action vector_table_1074085544_139_get_value(bit<32> _vector_table_1074085544_139_get_value_param0) {
    vector_table_1074085544_139_get_value_param0 = _vector_table_1074085544_139_get_value_param0;
  }

  table vector_table_1074085544_139 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074085544_139_get_value;
    }
    size = 36;
  }

  Hash<bit<13>>(HashAlgorithm_t.CRC32) fcfs_ct_1074053136_hash_163;
  Hash<bit<13>>(HashAlgorithm_t.CRC32) fcfs_ct_1074053136_hash_142;
  Hash<bit<13>>(HashAlgorithm_t.CRC32) fcfs_ct_1074053136_hash_165;
  Register<bit<32>,_>(65536, 0) fcfs_ct_1074053136_reg_liveness;
  RegisterAction<bit<32>, bit<32>, bool>(fcfs_ct_1074053136_reg_liveness) fcfs_ct_1074053136_reg_liveness_query_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
      } else {
        was_alive = true;
      }
    }
  };

  RegisterAction<bit<32>, bit<32>, bool>(fcfs_ct_1074053136_reg_liveness) fcfs_ct_1074053136_reg_liveness_query_and_refresh_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
        alarm = meta.time + 16384;
      } else {
        was_alive = true;
      }
    }
  };

  Register<bit<32>,_>(8192, 0) fcfs_ct_1074053136_reg_key_0;
  RegisterAction<bit<32>, bit<13>, void>(fcfs_ct_1074053136_reg_key_0) fcfs_ct_1074053136_reg_key_0_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_ct_1074053136_key_32b_0;
    }
  };

  RegisterAction<bit<32>, bit<13>, bit<8>>(fcfs_ct_1074053136_reg_key_0) fcfs_ct_1074053136_reg_key_0_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_ct_1074053136_key_32b_0) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<32>,_>(8192, 0) fcfs_ct_1074053136_reg_key_1;
  RegisterAction<bit<32>, bit<13>, void>(fcfs_ct_1074053136_reg_key_1) fcfs_ct_1074053136_reg_key_1_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_ct_1074053136_key_32b_1;
    }
  };

  RegisterAction<bit<32>, bit<13>, bit<8>>(fcfs_ct_1074053136_reg_key_1) fcfs_ct_1074053136_reg_key_1_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_ct_1074053136_key_32b_1) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<16>,_>(8192, 0) fcfs_ct_1074053136_reg_key_2;
  RegisterAction<bit<16>, bit<13>, void>(fcfs_ct_1074053136_reg_key_2) fcfs_ct_1074053136_reg_key_2_write = {
    void apply(inout bit<16> value) {
      value = meta.fcfs_ct_1074053136_key_16b_2;
    }
  };

  RegisterAction<bit<16>, bit<13>, bit<8>>(fcfs_ct_1074053136_reg_key_2) fcfs_ct_1074053136_reg_key_2_check_value = {
    void apply(inout bit<16> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_ct_1074053136_key_16b_2) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<16>,_>(8192, 0) fcfs_ct_1074053136_reg_key_3;
  RegisterAction<bit<16>, bit<13>, void>(fcfs_ct_1074053136_reg_key_3) fcfs_ct_1074053136_reg_key_3_write = {
    void apply(inout bit<16> value) {
      value = meta.fcfs_ct_1074053136_key_16b_3;
    }
  };

  RegisterAction<bit<16>, bit<13>, bit<8>>(fcfs_ct_1074053136_reg_key_3) fcfs_ct_1074053136_reg_key_3_check_value = {
    void apply(inout bit<16> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_ct_1074053136_key_16b_3) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  bit<32> fcfs_ct_1074053136_table_163_get_value_param0 = 32w0;
  action fcfs_ct_1074053136_table_163_get_value(bit<32> _fcfs_ct_1074053136_table_163_get_value_param0) {
    fcfs_ct_1074053136_table_163_get_value_param0 = _fcfs_ct_1074053136_table_163_get_value_param0;
  }

  table fcfs_ct_1074053136_table_163 {
    key = {
      meta.fcfs_ct_1074053136_key_32b_0: exact;
      meta.fcfs_ct_1074053136_key_32b_1: exact;
      meta.fcfs_ct_1074053136_key_16b_2: exact;
      meta.fcfs_ct_1074053136_key_16b_3: exact;
    }
    actions = {
      fcfs_ct_1074053136_table_163_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<13> fcfs_ct_1074053136_hash_163_value;
  action fcfs_ct_1074053136_hash_163_calc() {
    fcfs_ct_1074053136_hash_163_value = fcfs_ct_1074053136_hash_163.get({
      meta.fcfs_ct_1074053136_key_32b_0,
      meta.fcfs_ct_1074053136_key_32b_1,
      meta.fcfs_ct_1074053136_key_16b_2,
      meta.fcfs_ct_1074053136_key_16b_3
      });
      fcfs_ct_1074053136_table_163_get_value_param0[12:0] = fcfs_ct_1074053136_hash_163_value;
  }
  bit<8> match_counter0 = 0;
  action fcfs_ct_1074053136_check_key_0_163() {
    match_counter0 = match_counter0 + fcfs_ct_1074053136_reg_key_0_check_value.execute(fcfs_ct_1074053136_hash_163_value);
  }
  action fcfs_ct_1074053136_check_key_1_163() {
    match_counter0 = match_counter0 + fcfs_ct_1074053136_reg_key_1_check_value.execute(fcfs_ct_1074053136_hash_163_value);
  }
  action fcfs_ct_1074053136_check_key_2_163() {
    match_counter0 = match_counter0 + fcfs_ct_1074053136_reg_key_2_check_value.execute(fcfs_ct_1074053136_hash_163_value);
  }
  action fcfs_ct_1074053136_check_key_3_163() {
    match_counter0 = match_counter0 + fcfs_ct_1074053136_reg_key_3_check_value.execute(fcfs_ct_1074053136_hash_163_value);
  }
  Register<bit<32>,_>(65536, 0) vector_register_1074066960_0;
  Register<bit<32>,_>(65536, 0) vector_register_1074066960_1;
  Register<bit<16>,_>(65536, 0) vector_register_1074066960_2;
  Register<bit<16>,_>(65536, 0) vector_register_1074066960_3;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074066960_0) vector_register_1074066960_0_read_1022 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074066960_1) vector_register_1074066960_1_read_1022 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074066960_2) vector_register_1074066960_2_read_1022 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074066960_3) vector_register_1074066960_3_read_1022 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  bit<16> vector_table_1074102760_149_get_value_param0 = 16w0;
  action vector_table_1074102760_149_get_value(bit<16> _vector_table_1074102760_149_get_value_param0) {
    vector_table_1074102760_149_get_value_param0 = _vector_table_1074102760_149_get_value_param0;
  }

  table vector_table_1074102760_149 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074102760_149_get_value;
    }
    size = 36;
  }

  bit<32> fcfs_ct_value0 = 0;
  bit<13> fcfs_ct_1074053136_hash_165_value;
  action fcfs_ct_1074053136_hash_165_calc() {
    fcfs_ct_1074053136_hash_165_value = fcfs_ct_1074053136_hash_165.get({
      meta.fcfs_ct_1074053136_key_32b_0,
      meta.fcfs_ct_1074053136_key_32b_1,
      meta.fcfs_ct_1074053136_key_16b_2,
      meta.fcfs_ct_1074053136_key_16b_3
      });
      fcfs_ct_value0[12:0] = fcfs_ct_1074053136_hash_165_value;
  }
  bit<16> vector_table_1074102760_175_get_value_param0 = 16w0;
  action vector_table_1074102760_175_get_value(bit<16> _vector_table_1074102760_175_get_value_param0) {
    vector_table_1074102760_175_get_value_param0 = _vector_table_1074102760_175_get_value_param0;
  }

  table vector_table_1074102760_175 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074102760_175_get_value;
    }
    size = 36;
  }


  RegisterAction<bit<32>, bit<32>, void>(vector_register_1074066960_0) vector_register_1074066960_0_write_4947 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr1.data3;
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1074066960_1) vector_register_1074066960_1_write_4947 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr1.data4;
    }
  };

  RegisterAction<bit<16>, bit<32>, void>(vector_register_1074066960_2) vector_register_1074066960_2_write_4947 = {
    void apply(inout bit<16> value) {
      value = hdr.hdr2.data0;
    }
  };

  RegisterAction<bit<16>, bit<32>, void>(vector_register_1074066960_3) vector_register_1074066960_3_write_4947 = {
    void apply(inout bit<16> value) {
      value = hdr.hdr2.data1;
    }
  };

  bit<16> vector_table_1074102760_183_get_value_param0 = 16w0;
  action vector_table_1074102760_183_get_value(bit<16> _vector_table_1074102760_183_get_value_param0) {
    vector_table_1074102760_183_get_value_param0 = _vector_table_1074102760_183_get_value_param0;
  }

  table vector_table_1074102760_183 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074102760_183_get_value;
    }
    size = 36;
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        nf_dev[15:0] = 0;
      }
      if (hdr.recirc.code_path == 1) {
        nf_dev[15:0] = 1;
      }

    } else {
      if(hdr.hdr0.isValid()) {
        if(hdr.hdr1.isValid()) {
          if(hdr.hdr2.isValid()) {
            nf_dev[15:0] = 3;
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
  inout tessera_ingress_headers_t hdr,
  in    tessera_ingress_metadata_t meta,
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
  out tessera_egress_headers_t hdr,
  out tessera_egress_metadata_t eg_md,
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
  inout tessera_egress_headers_t hdr,
  inout tessera_egress_metadata_t eg_md,
  in    egress_intrinsic_metadata_t eg_intr_md,
  in    egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
  inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
  inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
  apply {}
}

control EgressDeparser(
  packet_out pkt,
  inout tessera_egress_headers_t hdr,
  in    tessera_egress_metadata_t eg_md,
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
