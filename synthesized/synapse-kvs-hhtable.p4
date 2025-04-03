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

header cpu_h {
  bit<16> code_path;
  bit<16> egress_dev;

}

header recirc_h {
  bit<16> code_path;

};

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
  bit<128> data1;
  bit<1024> data2;
  bit<8> data3;
  bit<16> data4;
}
header hh_table_1073923128_digest_hdr {
  bit<128> data0;
}



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;

}

struct synapse_ingress_metadata_t {
  bit<32> dev;
  bit<32> time;
  bool recirculate;

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
  out synapse_ingress_headers_t hdr,
  out synapse_ingress_metadata_t meta,
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
    pkt.extract(hdr.hdr0);
    transition parser_6;
  }
  state parser_6 {
    transition select (hdr.hdr0.data2) {
      2048: parser_7;
      default: parser_73;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr1);
    transition parser_8;
  }
  state parser_73 {
    transition reject;
  }
  state parser_8 {
    transition select (hdr.hdr1.data1[23:16]) {
      17: parser_9;
      default: parser_71;
    }
  }
  state parser_9 {
    pkt.extract(hdr.hdr2);
    transition parser_10;
  }
  state parser_71 {
    transition reject;
  }
  state parser_10 {
    transition select (hdr.hdr2.data1) {
      670: parser_11;
      default: parser_68;
    }
  }
  state parser_11 {
    pkt.extract(hdr.hdr3);
    transition parser_64;
  }
  state parser_68 {
    transition reject;
  }
  state parser_64 {
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

  bit<32> hh_table_1073923128_table_13_get_value_param0 = 32w0;
  action hh_table_1073923128_table_13_get_value(bit<32> _hh_table_1073923128_table_13_get_value_param0) {
    hh_table_1073923128_table_13_get_value_param0 = _hh_table_1073923128_table_13_get_value_param0;
  }

  bit<128> hh_table_1073923128_table_13_key0 = 128w0;
  table hh_table_1073923128_table_13 {
    key = {
      hh_table_1073923128_table_13_key0: exact;
    }
    actions = {
      hh_table_1073923128_table_13_get_value;
    }
    size = 8192;
  }

  Register<bit<32>,_>(8192, 0) hh_table_1073923128_cached_counters;
  RegisterAction<bit<32>, bit<32>, void>(hh_table_1073923128_cached_counters) hh_table_1073923128_cached_counters_inc_248 = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923128_hash_calc_0;

  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923128_hash_calc_1;

  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923128_hash_calc_2;

  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923128_hash_calc_3;

  Register<bit<32>,_>(1024, 0) hh_table_1073923128_cms_row_0;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923128_cms_row_0) hh_table_1073923128_cms_row_0_inc_and_read_248 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1073923128_cms_row_1;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923128_cms_row_1) hh_table_1073923128_cms_row_1_inc_and_read_248 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1073923128_cms_row_2;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923128_cms_row_2) hh_table_1073923128_cms_row_2_inc_and_read_248 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1073923128_cms_row_3;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923128_cms_row_3) hh_table_1073923128_cms_row_3_inc_and_read_248 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(8192, 0) hh_table_1073923128_threshold;
  bit<32> hh_table_1073923128_threshold_diff_248_cmp;
  RegisterAction<bit<32>, bit<1>, bit<32>>(hh_table_1073923128_threshold) hh_table_1073923128_threshold_diff_248 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = hh_table_1073923128_threshold_diff_248_cmp - value;
    }
  };

  Register<bit<8>,_>(1024, 0) hh_table_1073923128_bloom_row_0;
  RegisterAction<bit<8>, bit<10>, bit<8>>(hh_table_1073923128_bloom_row_0) hh_table_1073923128_bloom_row_0_read_and_set_248 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
      value = 1;
    }
  };

  Register<bit<8>,_>(1024, 0) hh_table_1073923128_bloom_row_1;
  RegisterAction<bit<8>, bit<10>, bit<8>>(hh_table_1073923128_bloom_row_1) hh_table_1073923128_bloom_row_1_read_and_set_248 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
      value = 1;
    }
  };

  Register<bit<8>,_>(1024, 0) hh_table_1073923128_bloom_row_2;
  RegisterAction<bit<8>, bit<10>, bit<8>>(hh_table_1073923128_bloom_row_2) hh_table_1073923128_bloom_row_2_read_and_set_248 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
      value = 1;
    }
  };

  Register<bit<8>,_>(1024, 0) hh_table_1073923128_bloom_row_3;
  RegisterAction<bit<8>, bit<10>, bit<8>>(hh_table_1073923128_bloom_row_3) hh_table_1073923128_bloom_row_3_read_and_set_248 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
      value = 1;
    }
  };

  bit<10> hh_table_1073923128_hash_calc_0_value;
  action hh_table_1073923128_hash_calc_0_value_calc() {
    bit<10> hh_table_1073923128_hash_calc_0_value = hh_table_1073923128_hash_calc_0.get({
      hh_table_1073923128_table_13_key0,
      32w0xfbc31fc7
    });
  }
  bit<10> hh_table_1073923128_hash_calc_1_value;
  action hh_table_1073923128_hash_calc_1_value_calc() {
    bit<10> hh_table_1073923128_hash_calc_1_value = hh_table_1073923128_hash_calc_1.get({
      hh_table_1073923128_table_13_key0,
      32w0x2681580b
    });
  }
  bit<10> hh_table_1073923128_hash_calc_2_value;
  action hh_table_1073923128_hash_calc_2_value_calc() {
    bit<10> hh_table_1073923128_hash_calc_2_value = hh_table_1073923128_hash_calc_2.get({
      hh_table_1073923128_table_13_key0,
      32w0x486d7e2f
    });
  }
  bit<10> hh_table_1073923128_hash_calc_3_value;
  action hh_table_1073923128_hash_calc_3_value_calc() {
    bit<10> hh_table_1073923128_hash_calc_3_value = hh_table_1073923128_hash_calc_3.get({
      hh_table_1073923128_table_13_key0,
      32w0x1f3a2b4d
    });
  }
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_0;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_1;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_2;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_3;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_4;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_5;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_6;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_7;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_8;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_9;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_10;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_11;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_12;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_13;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_14;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_15;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_16;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_17;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_18;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_19;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_20;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_21;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_22;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_23;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_24;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_25;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_26;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_27;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_28;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_29;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_30;
  Register<bit<32>,_>(8192, 0) vector_register_1073956208_31;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_0) vector_register_1073956208_0_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_1) vector_register_1073956208_1_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_2) vector_register_1073956208_2_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_3) vector_register_1073956208_3_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_4) vector_register_1073956208_4_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_5) vector_register_1073956208_5_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_6) vector_register_1073956208_6_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_7) vector_register_1073956208_7_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_8) vector_register_1073956208_8_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_9) vector_register_1073956208_9_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_10) vector_register_1073956208_10_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_11) vector_register_1073956208_11_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_12) vector_register_1073956208_12_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_13) vector_register_1073956208_13_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_14) vector_register_1073956208_14_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_15) vector_register_1073956208_15_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_16) vector_register_1073956208_16_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_17) vector_register_1073956208_17_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_18) vector_register_1073956208_18_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_19) vector_register_1073956208_19_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_20) vector_register_1073956208_20_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_21) vector_register_1073956208_21_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_22) vector_register_1073956208_22_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_23) vector_register_1073956208_23_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_24) vector_register_1073956208_24_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_25) vector_register_1073956208_25_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_26) vector_register_1073956208_26_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_27) vector_register_1073956208_27_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_28) vector_register_1073956208_28_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_29) vector_register_1073956208_29_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_30) vector_register_1073956208_30_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_31) vector_register_1073956208_31_read_1171 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_0) vector_register_1073956208_0_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[1023:992];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_1) vector_register_1073956208_1_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[991:960];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_2) vector_register_1073956208_2_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[959:928];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_3) vector_register_1073956208_3_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[927:896];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_4) vector_register_1073956208_4_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[895:864];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_5) vector_register_1073956208_5_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[863:832];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_6) vector_register_1073956208_6_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[831:800];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_7) vector_register_1073956208_7_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[799:768];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_8) vector_register_1073956208_8_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[767:736];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_9) vector_register_1073956208_9_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[735:704];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_10) vector_register_1073956208_10_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[703:672];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_11) vector_register_1073956208_11_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[671:640];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_12) vector_register_1073956208_12_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[639:608];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_13) vector_register_1073956208_13_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[607:576];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_14) vector_register_1073956208_14_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[575:544];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_15) vector_register_1073956208_15_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[543:512];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_16) vector_register_1073956208_16_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[511:480];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_17) vector_register_1073956208_17_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[479:448];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_18) vector_register_1073956208_18_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[447:416];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_19) vector_register_1073956208_19_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[415:384];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_20) vector_register_1073956208_20_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[383:352];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_21) vector_register_1073956208_21_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[351:320];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_22) vector_register_1073956208_22_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[319:288];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_23) vector_register_1073956208_23_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[287:256];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_24) vector_register_1073956208_24_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[255:224];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_25) vector_register_1073956208_25_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[223:192];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_26) vector_register_1073956208_26_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[191:160];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_27) vector_register_1073956208_27_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[159:128];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_28) vector_register_1073956208_28_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[127:96];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_29) vector_register_1073956208_29_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[95:64];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_30) vector_register_1073956208_30_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[63:32];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_31) vector_register_1073956208_31_write_1475 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[31:0];
    }
  };


  apply {
    if (hdr.cpu.isValid()) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:Ignore
      // BDD node 4:expire_items_single_map(chain:(w64 1073989720), vector:(w64 1073936952), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073923128))
      // EP node  4:ParserExtraction
      // BDD node 5:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 14), chunk:(w64 1074060080)[ -> (w64 1073759488)])
      if(hdr.hdr0.isValid()) {
        // EP node  14:ParserCondition
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  15:Then
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  23:ParserExtraction
        // BDD node 7:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 20), chunk:(w64 1074060816)[ -> (w64 1073759744)])
        if(hdr.hdr1.isValid()) {
          // EP node  42:ParserCondition
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  43:Then
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  55:ParserExtraction
          // BDD node 9:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 8), chunk:(w64 1074061472)[ -> (w64 1073760000)])
          if(hdr.hdr2.isValid()) {
            // EP node  86:ParserCondition
            // BDD node 10:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks)) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  87:Then
            // BDD node 10:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks)) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  116:ParserExtraction
            // BDD node 11:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 148), chunk:(w64 1074062200)[ -> (w64 1073760256)])
            if(hdr.hdr3.isValid()) {
              // EP node  159:If
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              if ((16w0x0000) != (meta.dev[15:0])) {
                // EP node  160:Then
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  248:HHTableRead
                // BDD node 13:map_get(map:(w64 1073923128), key:(w64 1073760257)[(ReadLSB w128 (w32 769) packet_chunks) -> (ReadLSB w128 (w32 769) packet_chunks)], value_out:(w64 1074059392)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                hh_table_1073923128_table_13_key0 = hdr.hdr3.data1;
                bool hit0 = hh_table_1073923128_table_13.apply().hit;
                if (hit0) {
                  hh_table_1073923128_cached_counters_inc_248.execute(hh_table_1073923128_table_13_get_value_param0);
                } else {
                  hh_table_1073923128_hash_calc_0_value_calc();
                  hh_table_1073923128_hash_calc_1_value_calc();
                  hh_table_1073923128_hash_calc_2_value_calc();
                  hh_table_1073923128_hash_calc_3_value_calc();
                  bit<32> hh_table_1073923128_cms_min_0 = hh_table_1073923128_cms_row_0_inc_and_read_248.execute(hh_table_1073923128_hash_calc_0_value);
                  bit<32> hh_table_1073923128_cms_min_1 = min(hh_table_1073923128_cms_min_0, hh_table_1073923128_cms_row_1_inc_and_read_248.execute(hh_table_1073923128_hash_calc_1_value));
                  bit<32> hh_table_1073923128_cms_min_2 = min(hh_table_1073923128_cms_min_1, hh_table_1073923128_cms_row_2_inc_and_read_248.execute(hh_table_1073923128_hash_calc_2_value));
                  bit<32> hh_table_1073923128_cms_min_3 = min(hh_table_1073923128_cms_min_2, hh_table_1073923128_cms_row_3_inc_and_read_248.execute(hh_table_1073923128_hash_calc_3_value));
                  hh_table_1073923128_threshold_diff_248_cmp = hh_table_1073923128_cms_min_3;
                  bit<32> hh_table_1073923128_threshold_diff = hh_table_1073923128_threshold_diff_248.execute(0);
                  if (hh_table_1073923128_threshold_diff_248_cmp[31:31] == 0) {
                    bit<8> hh_table_1073923128_bloom_counter = 0;
                    hh_table_1073923128_bloom_counter = hh_table_1073923128_bloom_counter + hh_table_1073923128_bloom_row_0_read_and_set_248.execute(hh_table_1073923128_hash_calc_0_value);
                    hh_table_1073923128_bloom_counter = hh_table_1073923128_bloom_counter + hh_table_1073923128_bloom_row_1_read_and_set_248.execute(hh_table_1073923128_hash_calc_1_value);
                    hh_table_1073923128_bloom_counter = hh_table_1073923128_bloom_counter + hh_table_1073923128_bloom_row_2_read_and_set_248.execute(hh_table_1073923128_hash_calc_2_value);
                    hh_table_1073923128_bloom_counter = hh_table_1073923128_bloom_counter + hh_table_1073923128_bloom_row_3_read_and_set_248.execute(hh_table_1073923128_hash_calc_3_value);
                    if (hh_table_1073923128_bloom_counter != 4) {
                      ig_dprsr_md.digest_type = 1;
                    }
                  }
                }
                // EP node  549:If
                // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                if (!hit0) {
                  // EP node  550:Then
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  2535:If
                  // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr3.data0)) {
                    // EP node  2536:Then
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  2750:HHTableOutOfBandUpdate
                    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073989720), index_out:(w64 1074063120)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    // EP node  3007:ModifyHeader
                    // BDD node 18:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 1) DEVICE) (Concat w1176 (Read w8 (w32 0) DEVICE) (ReadLSB w1168 (w32 768) packet_chunks)))])
                    hdr.hdr3.data4[15:8] = meta.dev[7:0];
                    hdr.hdr3.data4[7:0] = meta.dev[15:8];
                    // EP node  3878:Forward
                    // BDD node 22:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                    trigger_forward = true;
                  } else {
                    // EP node  2537:Else
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  4674:ModifyHeader
                    // BDD node 33:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 1) DEVICE) (Concat w1176 (Read w8 (w32 0) DEVICE) (ReadLSB w1168 (w32 768) packet_chunks)))])
                    hdr.hdr3.data4[15:8] = meta.dev[7:0];
                    hdr.hdr3.data4[7:0] = meta.dev[15:8];
                    // EP node  8916:Forward
                    // BDD node 37:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                    trigger_forward = true;
                  }
                } else {
                  // EP node  551:Else
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  898:Ignore
                  // BDD node 38:dchain_rejuvenate_index(chain:(w64 1073989720), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  1171:VectorRegisterLookup
                  // BDD node 46:vector_borrow(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074062640)[ -> (w64 1073970104)])
                  // EP node  1314:If
                  // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                  if ((8w0x01) != (hdr.hdr3.data0)) {
                    // EP node  1315:Then
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  4382:If
                    // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                    if ((8w0x00) != (hdr.hdr3.data0)) {
                      // EP node  4383:Then
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  5242:ModifyHeader
                      // BDD node 41:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (ReadLSB w1160 (w32 768) packet_chunks))))])
                      hdr.hdr3.data3 = 8w0x01;
                      // EP node  6301:ModifyHeader
                      // BDD node 42:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760000)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  7330:ModifyHeader
                      // BDD node 43:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759744)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  8317:ModifyHeader
                      // BDD node 44:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759488)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  9196:Forward
                      // BDD node 45:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    } else {
                      // EP node  4384:Else
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  5468:Ignore
                      // BDD node 47:vector_return(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073970104)[(ReadLSB w1024 (w32 0) vector_data_r24)])
                      // EP node  6589:ModifyHeader
                      // BDD node 48:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (Concat w1160 (Read w8 (w32 127) vector_data_r24) (Concat w1152 (Read w8 (w32 126) vector_data_r24) (Concat w1144 (Read w8 (w32 125) vector_data_r24) (Concat w1136 (Read w8 (w32 124) vector_data_r24) (Concat w1128 (Read w8 (w32 123) vector_data_r24) (Concat w1120 (Read w8 (w32 122) vector_data_r24) (Concat w1112 (Read w8 (w32 121) vector_data_r24) (Concat w1104 (Read w8 (w32 120) vector_data_r24) (Concat w1096 (Read w8 (w32 119) vector_data_r24) (Concat w1088 (Read w8 (w32 118) vector_data_r24) (Concat w1080 (Read w8 (w32 117) vector_data_r24) (Concat w1072 (Read w8 (w32 116) vector_data_r24) (Concat w1064 (Read w8 (w32 115) vector_data_r24) (Concat w1056 (Read w8 (w32 114) vector_data_r24) (Concat w1048 (Read w8 (w32 113) vector_data_r24) (Concat w1040 (Read w8 (w32 112) vector_data_r24) (Concat w1032 (Read w8 (w32 111) vector_data_r24) (Concat w1024 (Read w8 (w32 110) vector_data_r24) (Concat w1016 (Read w8 (w32 109) vector_data_r24) (Concat w1008 (Read w8 (w32 108) vector_data_r24) (Concat w1000 (Read w8 (w32 107) vector_data_r24) (Concat w992 (Read w8 (w32 106) vector_data_r24) (Concat w984 (Read w8 (w32 105) vector_data_r24) (Concat w976 (Read w8 (w32 104) vector_data_r24) (Concat w968 (Read w8 (w32 103) vector_data_r24) (Concat w960 (Read w8 (w32 102) vector_data_r24) (Concat w952 (Read w8 (w32 101) vector_data_r24) (Concat w944 (Read w8 (w32 100) vector_data_r24) (Concat w936 (Read w8 (w32 99) vector_data_r24) (Concat w928 (Read w8 (w32 98) vector_data_r24) (Concat w920 (Read w8 (w32 97) vector_data_r24) (Concat w912 (Read w8 (w32 96) vector_data_r24) (Concat w904 (Read w8 (w32 95) vector_data_r24) (Concat w896 (Read w8 (w32 94) vector_data_r24) (Concat w888 (Read w8 (w32 93) vector_data_r24) (Concat w880 (Read w8 (w32 92) vector_data_r24) (Concat w872 (Read w8 (w32 91) vector_data_r24) (Concat w864 (Read w8 (w32 90) vector_data_r24) (Concat w856 (Read w8 (w32 89) vector_data_r24) (Concat w848 (Read w8 (w32 88) vector_data_r24) (Concat w840 (Read w8 (w32 87) vector_data_r24) (Concat w832 (Read w8 (w32 86) vector_data_r24) (Concat w824 (Read w8 (w32 85) vector_data_r24) (Concat w816 (Read w8 (w32 84) vector_data_r24) (Concat w808 (Read w8 (w32 83) vector_data_r24) (Concat w800 (Read w8 (w32 82) vector_data_r24) (Concat w792 (Read w8 (w32 81) vector_data_r24) (Concat w784 (Read w8 (w32 80) vector_data_r24) (Concat w776 (Read w8 (w32 79) vector_data_r24) (Concat w768 (Read w8 (w32 78) vector_data_r24) (Concat w760 (Read w8 (w32 77) vector_data_r24) (Concat w752 (Read w8 (w32 76) vector_data_r24) (Concat w744 (Read w8 (w32 75) vector_data_r24) (Concat w736 (Read w8 (w32 74) vector_data_r24) (Concat w728 (Read w8 (w32 73) vector_data_r24) (Concat w720 (Read w8 (w32 72) vector_data_r24) (Concat w712 (Read w8 (w32 71) vector_data_r24) (Concat w704 (Read w8 (w32 70) vector_data_r24) (Concat w696 (Read w8 (w32 69) vector_data_r24) (Concat w688 (Read w8 (w32 68) vector_data_r24) (Concat w680 (Read w8 (w32 67) vector_data_r24) (Concat w672 (Read w8 (w32 66) vector_data_r24) (Concat w664 (Read w8 (w32 65) vector_data_r24) (Concat w656 (Read w8 (w32 64) vector_data_r24) (Concat w648 (Read w8 (w32 63) vector_data_r24) (Concat w640 (Read w8 (w32 62) vector_data_r24) (Concat w632 (Read w8 (w32 61) vector_data_r24) (Concat w624 (Read w8 (w32 60) vector_data_r24) (Concat w616 (Read w8 (w32 59) vector_data_r24) (Concat w608 (Read w8 (w32 58) vector_data_r24) (Concat w600 (Read w8 (w32 57) vector_data_r24) (Concat w592 (Read w8 (w32 56) vector_data_r24) (Concat w584 (Read w8 (w32 55) vector_data_r24) (Concat w576 (Read w8 (w32 54) vector_data_r24) (Concat w568 (Read w8 (w32 53) vector_data_r24) (Concat w560 (Read w8 (w32 52) vector_data_r24) (Concat w552 (Read w8 (w32 51) vector_data_r24) (Concat w544 (Read w8 (w32 50) vector_data_r24) (Concat w536 (Read w8 (w32 49) vector_data_r24) (Concat w528 (Read w8 (w32 48) vector_data_r24) (Concat w520 (Read w8 (w32 47) vector_data_r24) (Concat w512 (Read w8 (w32 46) vector_data_r24) (Concat w504 (Read w8 (w32 45) vector_data_r24) (Concat w496 (Read w8 (w32 44) vector_data_r24) (Concat w488 (Read w8 (w32 43) vector_data_r24) (Concat w480 (Read w8 (w32 42) vector_data_r24) (Concat w472 (Read w8 (w32 41) vector_data_r24) (Concat w464 (Read w8 (w32 40) vector_data_r24) (Concat w456 (Read w8 (w32 39) vector_data_r24) (Concat w448 (Read w8 (w32 38) vector_data_r24) (Concat w440 (Read w8 (w32 37) vector_data_r24) (Concat w432 (Read w8 (w32 36) vector_data_r24) (Concat w424 (Read w8 (w32 35) vector_data_r24) (Concat w416 (Read w8 (w32 34) vector_data_r24) (Concat w408 (Read w8 (w32 33) vector_data_r24) (Concat w400 (Read w8 (w32 32) vector_data_r24) (Concat w392 (Read w8 (w32 31) vector_data_r24) (Concat w384 (Read w8 (w32 30) vector_data_r24) (Concat w376 (Read w8 (w32 29) vector_data_r24) (Concat w368 (Read w8 (w32 28) vector_data_r24) (Concat w360 (Read w8 (w32 27) vector_data_r24) (Concat w352 (Read w8 (w32 26) vector_data_r24) (Concat w344 (Read w8 (w32 25) vector_data_r24) (Concat w336 (Read w8 (w32 24) vector_data_r24) (Concat w328 (Read w8 (w32 23) vector_data_r24) (Concat w320 (Read w8 (w32 22) vector_data_r24) (Concat w312 (Read w8 (w32 21) vector_data_r24) (Concat w304 (Read w8 (w32 20) vector_data_r24) (Concat w296 (Read w8 (w32 19) vector_data_r24) (Concat w288 (Read w8 (w32 18) vector_data_r24) (Concat w280 (Read w8 (w32 17) vector_data_r24) (Concat w272 (Read w8 (w32 16) vector_data_r24) (Concat w264 (Read w8 (w32 15) vector_data_r24) (Concat w256 (Read w8 (w32 14) vector_data_r24) (Concat w248 (Read w8 (w32 13) vector_data_r24) (Concat w240 (Read w8 (w32 12) vector_data_r24) (Concat w232 (Read w8 (w32 11) vector_data_r24) (Concat w224 (Read w8 (w32 10) vector_data_r24) (Concat w216 (Read w8 (w32 9) vector_data_r24) (Concat w208 (Read w8 (w32 8) vector_data_r24) (Concat w200 (Read w8 (w32 7) vector_data_r24) (Concat w192 (Read w8 (w32 6) vector_data_r24) (Concat w184 (Read w8 (w32 5) vector_data_r24) (Concat w176 (Read w8 (w32 4) vector_data_r24) (Concat w168 (Read w8 (w32 3) vector_data_r24) (Concat w160 (Read w8 (w32 2) vector_data_r24) (Concat w152 (Read w8 (w32 1) vector_data_r24) (Concat w144 (Read w8 (w32 0) vector_data_r24) (ReadLSB w136 (w32 768) packet_chunks))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))])
                      hdr.hdr3.data2[1023:992] = vector_register_1073956208_0_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[895:864] = vector_register_1073956208_4_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[767:736] = vector_register_1073956208_8_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[639:608] = vector_register_1073956208_12_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[511:480] = vector_register_1073956208_16_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[383:352] = vector_register_1073956208_20_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[255:224] = vector_register_1073956208_24_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[127:96] = vector_register_1073956208_28_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data3 = 8w0x01;
                      hdr.hdr3.data2[991:960] = vector_register_1073956208_1_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[863:832] = vector_register_1073956208_5_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[735:704] = vector_register_1073956208_9_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[607:576] = vector_register_1073956208_13_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[479:448] = vector_register_1073956208_17_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[351:320] = vector_register_1073956208_21_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[223:192] = vector_register_1073956208_25_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[95:64] = vector_register_1073956208_29_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[959:928] = vector_register_1073956208_2_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[831:800] = vector_register_1073956208_6_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[703:672] = vector_register_1073956208_10_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[575:544] = vector_register_1073956208_14_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[447:416] = vector_register_1073956208_18_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[319:288] = vector_register_1073956208_22_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[191:160] = vector_register_1073956208_26_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[63:32] = vector_register_1073956208_30_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[927:896] = vector_register_1073956208_3_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[799:768] = vector_register_1073956208_7_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[671:640] = vector_register_1073956208_11_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[543:512] = vector_register_1073956208_15_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[415:384] = vector_register_1073956208_19_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[287:256] = vector_register_1073956208_23_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[159:128] = vector_register_1073956208_27_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      hdr.hdr3.data2[31:0] = vector_register_1073956208_31_read_1171.execute(hh_table_1073923128_table_13_get_value_param0);
                      // EP node  7636:ModifyHeader
                      // BDD node 49:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760000)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  8641:ModifyHeader
                      // BDD node 50:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759744)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  9538:ModifyHeader
                      // BDD node 51:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759488)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  9828:Forward
                      // BDD node 52:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    }
                  } else {
                    // EP node  1316:Else
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  1475:VectorRegisterUpdate
                    // BDD node 54:vector_return(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073970104)[(ReadLSB w1024 (w32 785) packet_chunks)])
                    vector_register_1073956208_0_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_1_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_2_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_3_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_4_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_5_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_6_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_7_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_8_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_9_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_10_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_11_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_12_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_13_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_14_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_15_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_16_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_17_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_18_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_19_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_20_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_21_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_22_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_23_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_24_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_25_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_26_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_27_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_28_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_29_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_30_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    vector_register_1073956208_31_write_1475.execute(hh_table_1073923128_table_13_get_value_param0);
                    // EP node  1669:ModifyHeader
                    // BDD node 55:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (ReadLSB w1160 (w32 768) packet_chunks))))])
                    hdr.hdr3.data3 = 8w0x01;
                    // EP node  1843:ModifyHeader
                    // BDD node 56:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760000)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                    // EP node  2023:ModifyHeader
                    // BDD node 57:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759744)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                    swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                    swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                    swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                    // EP node  2209:ModifyHeader
                    // BDD node 58:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759488)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                    swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                    // EP node  2369:Forward
                    // BDD node 59:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                    trigger_forward = true;
                  }
                }
              } else {
                // EP node  161:Else
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  7999:Forward
                // BDD node 64:FORWARD
                nf_dev[15:0] = hdr.hdr3.data4;
                trigger_forward = true;
              }
            }
            // EP node  88:Else
            // BDD node 10:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks)) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  6931:ParserReject
            // BDD node 68:DROP
          }
          // EP node  44:Else
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  5833:ParserReject
          // BDD node 71:DROP
        }
        // EP node  16:Else
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  4717:ParserReject
        // BDD node 73:DROP
      }

    }

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
  inout synapse_ingress_headers_t hdr,
  in    synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
  Digest<hh_table_1073923128_digest_hdr>() hh_table_1073923128_digest;

  apply {
    if (ig_dprsr_md.digest_type == 1) {
      hh_table_1073923128_digest.pack({
        hdr.hdr3.data1,
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
