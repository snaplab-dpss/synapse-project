#include <core.p4>

#if __TARGET_TOFINO__ == 2
  #include <t2na.p4>
  #define CPU_PCIE_PORT 0
  #define RECIRCULATION_PORT 6
#else
  #include <tna.p4>
  #define CPU_PCIE_PORT 192
  #define RECIRCULATION_PORT 320
#endif

header cpu_h {
  bit<16> code_path;
  bit<16> egress_dev;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;

};

header hdr_0_h {
  bit<112> data;
}
header hdr_1_h {
  bit<160> data;
}
header hdr_2_h {
  bit<64> data;
}
header hdr_3_h {
  bit<1184> data;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  hdr_0_h hdr_0;
  hdr_1_h hdr_1;
  hdr_2_h hdr_2;
  hdr_3_h hdr_3;

}

struct synapse_ingress_metadata_t {
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
    
    transition select(ig_intr_md.ingress_port) {
      CPU_PCIE_PORT: parse_cpu;
      RECIRCULATION_PORT: parse_recirc;
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
    pkt.extract(hdr.hdr_0);
    transition parser_6;
  }
  state parser_6 {
    transition select (hdr.hdr_0.data[15:0]) {
      2048: parser_7;
      default: parser_73;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr_1);
    transition parser_8;
  }
  state parser_73 {
    transition reject;
  }
  state parser_8 {
    transition select (hdr.hdr_1.data[87:80]) {
      17: parser_9;
      default: parser_71;
    }
  }
  state parser_9 {
    pkt.extract(hdr.hdr_2);
    transition parser_10;
  }
  state parser_71 {
    transition reject;
  }
  state parser_10 {
    transition select (hdr.hdr_2.data[47:32]) {
      670: parser_11;
      default: parser_68;
    }
  }
  state parser_11 {
    pkt.extract(hdr.hdr_3);
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

  bit<32> map_table_1073923128_13_get_value_param_0 = 32w0;
  action map_table_1073923128_13_get_value(bit<32> _map_table_1073923128_13_get_value_param_0) {
    map_table_1073923128_13_get_value_param_0 = _map_table_1073923128_13_get_value_param_0;
  }

  bit<128> map_table_1073923128_13_key_0 = 128w0;
  table map_table_1073923128_13 {
    key = {
      map_table_1073923128_13_key_0: exact;
    }
    actions = {
      map_table_1073923128_13_get_value;
    }
    size = 8192;
    idle_timeout = true;
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

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_0) vector_register_1073956208_0_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_1) vector_register_1073956208_1_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_2) vector_register_1073956208_2_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_3) vector_register_1073956208_3_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_4) vector_register_1073956208_4_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_5) vector_register_1073956208_5_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_6) vector_register_1073956208_6_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_7) vector_register_1073956208_7_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_8) vector_register_1073956208_8_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_9) vector_register_1073956208_9_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_10) vector_register_1073956208_10_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_11) vector_register_1073956208_11_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_12) vector_register_1073956208_12_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_13) vector_register_1073956208_13_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_14) vector_register_1073956208_14_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_15) vector_register_1073956208_15_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_16) vector_register_1073956208_16_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_17) vector_register_1073956208_17_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_18) vector_register_1073956208_18_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_19) vector_register_1073956208_19_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_20) vector_register_1073956208_20_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_21) vector_register_1073956208_21_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_22) vector_register_1073956208_22_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_23) vector_register_1073956208_23_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_24) vector_register_1073956208_24_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_25) vector_register_1073956208_25_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_26) vector_register_1073956208_26_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_27) vector_register_1073956208_27_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_28) vector_register_1073956208_28_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_29) vector_register_1073956208_29_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_30) vector_register_1073956208_30_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956208_31) vector_register_1073956208_31_read_641 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> dchain_table_1073989720_38_key_0 = 32w0;
  table dchain_table_1073989720_38 {
    key = {
      dchain_table_1073989720_38_key_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 8192;
    idle_timeout = true;
  }


  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_0) vector_register_1073956208_0_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[1047:1016];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_1) vector_register_1073956208_1_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[1015:984];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_2) vector_register_1073956208_2_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[983:952];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_3) vector_register_1073956208_3_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[951:920];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_4) vector_register_1073956208_4_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[919:888];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_5) vector_register_1073956208_5_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[887:856];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_6) vector_register_1073956208_6_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[855:824];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_7) vector_register_1073956208_7_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[823:792];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_8) vector_register_1073956208_8_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[791:760];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_9) vector_register_1073956208_9_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[759:728];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_10) vector_register_1073956208_10_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[727:696];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_11) vector_register_1073956208_11_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[695:664];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_12) vector_register_1073956208_12_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[663:632];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_13) vector_register_1073956208_13_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[631:600];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_14) vector_register_1073956208_14_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[599:568];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_15) vector_register_1073956208_15_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[567:536];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_16) vector_register_1073956208_16_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[535:504];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_17) vector_register_1073956208_17_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[503:472];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_18) vector_register_1073956208_18_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[471:440];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_19) vector_register_1073956208_19_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[439:408];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_20) vector_register_1073956208_20_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[407:376];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_21) vector_register_1073956208_21_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[375:344];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_22) vector_register_1073956208_22_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[343:312];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_23) vector_register_1073956208_23_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[311:280];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_24) vector_register_1073956208_24_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[279:248];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_25) vector_register_1073956208_25_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[247:216];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_26) vector_register_1073956208_26_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[215:184];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_27) vector_register_1073956208_27_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[183:152];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_28) vector_register_1073956208_28_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[151:120];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_29) vector_register_1073956208_29_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[119:88];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_30) vector_register_1073956208_30_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[87:56];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956208_31) vector_register_1073956208_31_write_3513 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[55:24];
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
      // EP node  1
      // BDD node 4:expire_items_single_map(chain:(w64 1073989720), vector:(w64 1073936952), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073923128))
      // EP node  4
      // BDD node 5:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 14), chunk:(w64 1074060080)[ -> (w64 1073759488)])
      if(hdr.hdr_0.isValid()) {
        // EP node  14
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  15
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  23
        // BDD node 7:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 20), chunk:(w64 1074060816)[ -> (w64 1073759744)])
        if(hdr.hdr_1.isValid()) {
          // EP node  42
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  43
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  55
          // BDD node 9:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 8), chunk:(w64 1074061472)[ -> (w64 1073760000)])
          if(hdr.hdr_2.isValid()) {
            // EP node  86
            // BDD node 10:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks)) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  87
            // BDD node 10:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks)) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  379
            // BDD node 11:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 148), chunk:(w64 1074062200)[ -> (w64 1073760256)])
            if(hdr.hdr_3.isValid()) {
              // EP node  428
              // BDD node 13:map_get(map:(w64 1073923128), key:(w64 1073760257)[(ReadLSB w128 (w32 769) packet_chunks) -> (ReadLSB w128 (w32 769) packet_chunks)], value_out:(w64 1074059392)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index_r1)])
              map_table_1073923128_13_key_0 = hdr.hdr_3.data[1175:1048];
              bool hit_0 = map_table_1073923128_13.apply().hit;
              // EP node  641
              // BDD node 46:vector_borrow(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) allocated_index_r1), val_out:(w64 1074062640)[ -> (w64 1073970104)])
              bit<1024> vector_reg_value_0 = 1024w0;
              vector_reg_value_0[31:0] = vector_register_1073956208_0_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[63:32] = vector_register_1073956208_1_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[95:64] = vector_register_1073956208_2_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[127:96] = vector_register_1073956208_3_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[159:128] = vector_register_1073956208_4_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[191:160] = vector_register_1073956208_5_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[223:192] = vector_register_1073956208_6_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[255:224] = vector_register_1073956208_7_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[287:256] = vector_register_1073956208_8_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[319:288] = vector_register_1073956208_9_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[351:320] = vector_register_1073956208_10_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[383:352] = vector_register_1073956208_11_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[415:384] = vector_register_1073956208_12_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[447:416] = vector_register_1073956208_13_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[479:448] = vector_register_1073956208_14_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[511:480] = vector_register_1073956208_15_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[543:512] = vector_register_1073956208_16_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[575:544] = vector_register_1073956208_17_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[607:576] = vector_register_1073956208_18_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[639:608] = vector_register_1073956208_19_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[671:640] = vector_register_1073956208_20_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[703:672] = vector_register_1073956208_21_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[735:704] = vector_register_1073956208_22_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[767:736] = vector_register_1073956208_23_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[799:768] = vector_register_1073956208_24_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[831:800] = vector_register_1073956208_25_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[863:832] = vector_register_1073956208_26_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[895:864] = vector_register_1073956208_27_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[927:896] = vector_register_1073956208_28_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[959:928] = vector_register_1073956208_29_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[991:960] = vector_register_1073956208_30_read_641.execute(map_table_1073923128_13_get_value_param_0);
              vector_reg_value_0[1023:992] = vector_register_1073956208_31_read_641.execute(map_table_1073923128_13_get_value_param_0);
              // EP node  754
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              if ((16w0x0000) != (meta.dev[15:0])) {
                // EP node  755
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  1692
                // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r1))
                if (!hit_0) {
                  // EP node  1693
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r1))
                  // EP node  6828
                  // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr_3.data[1183:1176])) {
                    // EP node  6829
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  6934
                    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073989720), index_out:(w64 1074063120)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index_r7)], time:(ReadLSB w64 (w32 0) next_time))
                    send_to_controller(6934);
                    hdr.cpu.dev = meta.dev;
                  } else {
                    // EP node  6830
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  7645
                    // BDD node 33:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 1) DEVICE) (Concat w1176 (Read w8 (w32 0) DEVICE) (Concat w1168 (w8 0) (ReadLSB w1160 (w32 768) packet_chunks))))])
                    hdr.hdr_3.data[23:16] = 8w0x00;
                    hdr.hdr_3.data[15:8] = meta.dev[7:0];
                    hdr.hdr_3.data[7:0] = meta.dev[15:8];
                    // EP node  9210
                    // BDD node 37:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                    trigger_forward = true;
                  }
                } else {
                  // EP node  1694
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r1))
                  // EP node  1853
                  // BDD node 38:dchain_rejuvenate_index(chain:(w64 1073989720), index:(ReadLSB w32 (w32 0) allocated_index_r1), time:(ReadLSB w64 (w32 0) next_time))
                  dchain_table_1073989720_38_key_0 = map_table_1073923128_13_get_value_param_0;
                  dchain_table_1073989720_38.apply();
                  // EP node  2020
                  // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                  if ((8w0x01) != (hdr.hdr_3.data[1183:1176])) {
                    // EP node  2021
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  2205
                    // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                    if ((8w0x00) != (hdr.hdr_3.data[1183:1176])) {
                      // EP node  2206
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  2441
                      // BDD node 41:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (ReadLSB w1160 (w32 768) packet_chunks))))])
                      hdr.hdr_3.data[23:16] = 8w0x01;
                      // EP node  2651
                      // BDD node 42:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760000)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr_2.data[63:56], hdr.hdr_2.data[47:40]);
                      swap(hdr.hdr_2.data[55:48], hdr.hdr_2.data[39:32]);
                      // EP node  2867
                      // BDD node 43:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759744)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr_1.data[63:56], hdr.hdr_1.data[31:24]);
                      swap(hdr.hdr_1.data[55:48], hdr.hdr_1.data[23:16]);
                      swap(hdr.hdr_1.data[47:40], hdr.hdr_1.data[15:8]);
                      swap(hdr.hdr_1.data[39:32], hdr.hdr_1.data[7:0]);
                      // EP node  3089
                      // BDD node 44:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759488)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr_0.data[111:104], hdr.hdr_0.data[63:56]);
                      swap(hdr.hdr_0.data[103:96], hdr.hdr_0.data[55:48]);
                      swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                      swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                      swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                      swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                      // EP node  3317
                      // BDD node 45:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    } else {
                      // EP node  2207
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  5037
                      // BDD node 47:vector_return(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) allocated_index_r1), value:(w64 1073970104)[(ReadLSB w1024 (w32 0) vector_data_r2)])
                      // EP node  5357
                      // BDD node 48:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (Concat w1160 (Read w8 (w32 127) vector_data_r2) (Concat w1152 (Read w8 (w32 126) vector_data_r2) (Concat w1144 (Read w8 (w32 125) vector_data_r2) (Concat w1136 (Read w8 (w32 124) vector_data_r2) (Concat w1128 (Read w8 (w32 123) vector_data_r2) (Concat w1120 (Read w8 (w32 122) vector_data_r2) (Concat w1112 (Read w8 (w32 121) vector_data_r2) (Concat w1104 (Read w8 (w32 120) vector_data_r2) (Concat w1096 (Read w8 (w32 119) vector_data_r2) (Concat w1088 (Read w8 (w32 118) vector_data_r2) (Concat w1080 (Read w8 (w32 117) vector_data_r2) (Concat w1072 (Read w8 (w32 116) vector_data_r2) (Concat w1064 (Read w8 (w32 115) vector_data_r2) (Concat w1056 (Read w8 (w32 114) vector_data_r2) (Concat w1048 (Read w8 (w32 113) vector_data_r2) (Concat w1040 (Read w8 (w32 112) vector_data_r2) (Concat w1032 (Read w8 (w32 111) vector_data_r2) (Concat w1024 (Read w8 (w32 110) vector_data_r2) (Concat w1016 (Read w8 (w32 109) vector_data_r2) (Concat w1008 (Read w8 (w32 108) vector_data_r2) (Concat w1000 (Read w8 (w32 107) vector_data_r2) (Concat w992 (Read w8 (w32 106) vector_data_r2) (Concat w984 (Read w8 (w32 105) vector_data_r2) (Concat w976 (Read w8 (w32 104) vector_data_r2) (Concat w968 (Read w8 (w32 103) vector_data_r2) (Concat w960 (Read w8 (w32 102) vector_data_r2) (Concat w952 (Read w8 (w32 101) vector_data_r2) (Concat w944 (Read w8 (w32 100) vector_data_r2) (Concat w936 (Read w8 (w32 99) vector_data_r2) (Concat w928 (Read w8 (w32 98) vector_data_r2) (Concat w920 (Read w8 (w32 97) vector_data_r2) (Concat w912 (Read w8 (w32 96) vector_data_r2) (Concat w904 (Read w8 (w32 95) vector_data_r2) (Concat w896 (Read w8 (w32 94) vector_data_r2) (Concat w888 (Read w8 (w32 93) vector_data_r2) (Concat w880 (Read w8 (w32 92) vector_data_r2) (Concat w872 (Read w8 (w32 91) vector_data_r2) (Concat w864 (Read w8 (w32 90) vector_data_r2) (Concat w856 (Read w8 (w32 89) vector_data_r2) (Concat w848 (Read w8 (w32 88) vector_data_r2) (Concat w840 (Read w8 (w32 87) vector_data_r2) (Concat w832 (Read w8 (w32 86) vector_data_r2) (Concat w824 (Read w8 (w32 85) vector_data_r2) (Concat w816 (Read w8 (w32 84) vector_data_r2) (Concat w808 (Read w8 (w32 83) vector_data_r2) (Concat w800 (Read w8 (w32 82) vector_data_r2) (Concat w792 (Read w8 (w32 81) vector_data_r2) (Concat w784 (Read w8 (w32 80) vector_data_r2) (Concat w776 (Read w8 (w32 79) vector_data_r2) (Concat w768 (Read w8 (w32 78) vector_data_r2) (Concat w760 (Read w8 (w32 77) vector_data_r2) (Concat w752 (Read w8 (w32 76) vector_data_r2) (Concat w744 (Read w8 (w32 75) vector_data_r2) (Concat w736 (Read w8 (w32 74) vector_data_r2) (Concat w728 (Read w8 (w32 73) vector_data_r2) (Concat w720 (Read w8 (w32 72) vector_data_r2) (Concat w712 (Read w8 (w32 71) vector_data_r2) (Concat w704 (Read w8 (w32 70) vector_data_r2) (Concat w696 (Read w8 (w32 69) vector_data_r2) (Concat w688 (Read w8 (w32 68) vector_data_r2) (Concat w680 (Read w8 (w32 67) vector_data_r2) (Concat w672 (Read w8 (w32 66) vector_data_r2) (Concat w664 (Read w8 (w32 65) vector_data_r2) (Concat w656 (Read w8 (w32 64) vector_data_r2) (Concat w648 (Read w8 (w32 63) vector_data_r2) (Concat w640 (Read w8 (w32 62) vector_data_r2) (Concat w632 (Read w8 (w32 61) vector_data_r2) (Concat w624 (Read w8 (w32 60) vector_data_r2) (Concat w616 (Read w8 (w32 59) vector_data_r2) (Concat w608 (Read w8 (w32 58) vector_data_r2) (Concat w600 (Read w8 (w32 57) vector_data_r2) (Concat w592 (Read w8 (w32 56) vector_data_r2) (Concat w584 (Read w8 (w32 55) vector_data_r2) (Concat w576 (Read w8 (w32 54) vector_data_r2) (Concat w568 (Read w8 (w32 53) vector_data_r2) (Concat w560 (Read w8 (w32 52) vector_data_r2) (Concat w552 (Read w8 (w32 51) vector_data_r2) (Concat w544 (Read w8 (w32 50) vector_data_r2) (Concat w536 (Read w8 (w32 49) vector_data_r2) (Concat w528 (Read w8 (w32 48) vector_data_r2) (Concat w520 (Read w8 (w32 47) vector_data_r2) (Concat w512 (Read w8 (w32 46) vector_data_r2) (Concat w504 (Read w8 (w32 45) vector_data_r2) (Concat w496 (Read w8 (w32 44) vector_data_r2) (Concat w488 (Read w8 (w32 43) vector_data_r2) (Concat w480 (Read w8 (w32 42) vector_data_r2) (Concat w472 (Read w8 (w32 41) vector_data_r2) (Concat w464 (Read w8 (w32 40) vector_data_r2) (Concat w456 (Read w8 (w32 39) vector_data_r2) (Concat w448 (Read w8 (w32 38) vector_data_r2) (Concat w440 (Read w8 (w32 37) vector_data_r2) (Concat w432 (Read w8 (w32 36) vector_data_r2) (Concat w424 (Read w8 (w32 35) vector_data_r2) (Concat w416 (Read w8 (w32 34) vector_data_r2) (Concat w408 (Read w8 (w32 33) vector_data_r2) (Concat w400 (Read w8 (w32 32) vector_data_r2) (Concat w392 (Read w8 (w32 31) vector_data_r2) (Concat w384 (Read w8 (w32 30) vector_data_r2) (Concat w376 (Read w8 (w32 29) vector_data_r2) (Concat w368 (Read w8 (w32 28) vector_data_r2) (Concat w360 (Read w8 (w32 27) vector_data_r2) (Concat w352 (Read w8 (w32 26) vector_data_r2) (Concat w344 (Read w8 (w32 25) vector_data_r2) (Concat w336 (Read w8 (w32 24) vector_data_r2) (Concat w328 (Read w8 (w32 23) vector_data_r2) (Concat w320 (Read w8 (w32 22) vector_data_r2) (Concat w312 (Read w8 (w32 21) vector_data_r2) (Concat w304 (Read w8 (w32 20) vector_data_r2) (Concat w296 (Read w8 (w32 19) vector_data_r2) (Concat w288 (Read w8 (w32 18) vector_data_r2) (Concat w280 (Read w8 (w32 17) vector_data_r2) (Concat w272 (Read w8 (w32 16) vector_data_r2) (Concat w264 (Read w8 (w32 15) vector_data_r2) (Concat w256 (Read w8 (w32 14) vector_data_r2) (Concat w248 (Read w8 (w32 13) vector_data_r2) (Concat w240 (Read w8 (w32 12) vector_data_r2) (Concat w232 (Read w8 (w32 11) vector_data_r2) (Concat w224 (Read w8 (w32 10) vector_data_r2) (Concat w216 (Read w8 (w32 9) vector_data_r2) (Concat w208 (Read w8 (w32 8) vector_data_r2) (Concat w200 (Read w8 (w32 7) vector_data_r2) (Concat w192 (Read w8 (w32 6) vector_data_r2) (Concat w184 (Read w8 (w32 5) vector_data_r2) (Concat w176 (Read w8 (w32 4) vector_data_r2) (Concat w168 (Read w8 (w32 3) vector_data_r2) (Concat w160 (Read w8 (w32 2) vector_data_r2) (Concat w152 (Read w8 (w32 1) vector_data_r2) (Concat w144 (Read w8 (w32 0) vector_data_r2) (ReadLSB w136 (w32 768) packet_chunks))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))])
                      hdr.hdr_3.data[1047:1040] = vector_reg_value_0[7:0];
                      hdr.hdr_3.data[1039:1032] = vector_reg_value_0[15:8];
                      hdr.hdr_3.data[1031:1024] = vector_reg_value_0[23:16];
                      hdr.hdr_3.data[1023:1016] = vector_reg_value_0[31:24];
                      hdr.hdr_3.data[1015:1008] = vector_reg_value_0[39:32];
                      hdr.hdr_3.data[1007:1000] = vector_reg_value_0[47:40];
                      hdr.hdr_3.data[999:992] = vector_reg_value_0[55:48];
                      hdr.hdr_3.data[991:984] = vector_reg_value_0[63:56];
                      hdr.hdr_3.data[983:976] = vector_reg_value_0[71:64];
                      hdr.hdr_3.data[975:968] = vector_reg_value_0[79:72];
                      hdr.hdr_3.data[967:960] = vector_reg_value_0[87:80];
                      hdr.hdr_3.data[959:952] = vector_reg_value_0[95:88];
                      hdr.hdr_3.data[951:944] = vector_reg_value_0[103:96];
                      hdr.hdr_3.data[943:936] = vector_reg_value_0[111:104];
                      hdr.hdr_3.data[935:928] = vector_reg_value_0[119:112];
                      hdr.hdr_3.data[927:920] = vector_reg_value_0[127:120];
                      hdr.hdr_3.data[919:912] = vector_reg_value_0[135:128];
                      hdr.hdr_3.data[911:904] = vector_reg_value_0[143:136];
                      hdr.hdr_3.data[903:896] = vector_reg_value_0[151:144];
                      hdr.hdr_3.data[895:888] = vector_reg_value_0[159:152];
                      hdr.hdr_3.data[887:880] = vector_reg_value_0[167:160];
                      hdr.hdr_3.data[879:872] = vector_reg_value_0[175:168];
                      hdr.hdr_3.data[871:864] = vector_reg_value_0[183:176];
                      hdr.hdr_3.data[863:856] = vector_reg_value_0[191:184];
                      hdr.hdr_3.data[855:848] = vector_reg_value_0[199:192];
                      hdr.hdr_3.data[847:840] = vector_reg_value_0[207:200];
                      hdr.hdr_3.data[839:832] = vector_reg_value_0[215:208];
                      hdr.hdr_3.data[831:824] = vector_reg_value_0[223:216];
                      hdr.hdr_3.data[823:816] = vector_reg_value_0[231:224];
                      hdr.hdr_3.data[815:808] = vector_reg_value_0[239:232];
                      hdr.hdr_3.data[807:800] = vector_reg_value_0[247:240];
                      hdr.hdr_3.data[799:792] = vector_reg_value_0[255:248];
                      hdr.hdr_3.data[791:784] = vector_reg_value_0[263:256];
                      hdr.hdr_3.data[783:776] = vector_reg_value_0[271:264];
                      hdr.hdr_3.data[775:768] = vector_reg_value_0[279:272];
                      hdr.hdr_3.data[767:760] = vector_reg_value_0[287:280];
                      hdr.hdr_3.data[759:752] = vector_reg_value_0[295:288];
                      hdr.hdr_3.data[751:744] = vector_reg_value_0[303:296];
                      hdr.hdr_3.data[743:736] = vector_reg_value_0[311:304];
                      hdr.hdr_3.data[735:728] = vector_reg_value_0[319:312];
                      hdr.hdr_3.data[727:720] = vector_reg_value_0[327:320];
                      hdr.hdr_3.data[719:712] = vector_reg_value_0[335:328];
                      hdr.hdr_3.data[711:704] = vector_reg_value_0[343:336];
                      hdr.hdr_3.data[703:696] = vector_reg_value_0[351:344];
                      hdr.hdr_3.data[695:688] = vector_reg_value_0[359:352];
                      hdr.hdr_3.data[687:680] = vector_reg_value_0[367:360];
                      hdr.hdr_3.data[679:672] = vector_reg_value_0[375:368];
                      hdr.hdr_3.data[671:664] = vector_reg_value_0[383:376];
                      hdr.hdr_3.data[663:656] = vector_reg_value_0[391:384];
                      hdr.hdr_3.data[655:648] = vector_reg_value_0[399:392];
                      hdr.hdr_3.data[647:640] = vector_reg_value_0[407:400];
                      hdr.hdr_3.data[639:632] = vector_reg_value_0[415:408];
                      hdr.hdr_3.data[631:624] = vector_reg_value_0[423:416];
                      hdr.hdr_3.data[623:616] = vector_reg_value_0[431:424];
                      hdr.hdr_3.data[615:608] = vector_reg_value_0[439:432];
                      hdr.hdr_3.data[607:600] = vector_reg_value_0[447:440];
                      hdr.hdr_3.data[599:592] = vector_reg_value_0[455:448];
                      hdr.hdr_3.data[591:584] = vector_reg_value_0[463:456];
                      hdr.hdr_3.data[583:576] = vector_reg_value_0[471:464];
                      hdr.hdr_3.data[575:568] = vector_reg_value_0[479:472];
                      hdr.hdr_3.data[567:560] = vector_reg_value_0[487:480];
                      hdr.hdr_3.data[559:552] = vector_reg_value_0[495:488];
                      hdr.hdr_3.data[551:544] = vector_reg_value_0[503:496];
                      hdr.hdr_3.data[543:536] = vector_reg_value_0[511:504];
                      hdr.hdr_3.data[535:528] = vector_reg_value_0[519:512];
                      hdr.hdr_3.data[527:520] = vector_reg_value_0[527:520];
                      hdr.hdr_3.data[519:512] = vector_reg_value_0[535:528];
                      hdr.hdr_3.data[511:504] = vector_reg_value_0[543:536];
                      hdr.hdr_3.data[503:496] = vector_reg_value_0[551:544];
                      hdr.hdr_3.data[495:488] = vector_reg_value_0[559:552];
                      hdr.hdr_3.data[487:480] = vector_reg_value_0[567:560];
                      hdr.hdr_3.data[479:472] = vector_reg_value_0[575:568];
                      hdr.hdr_3.data[471:464] = vector_reg_value_0[583:576];
                      hdr.hdr_3.data[463:456] = vector_reg_value_0[591:584];
                      hdr.hdr_3.data[455:448] = vector_reg_value_0[599:592];
                      hdr.hdr_3.data[447:440] = vector_reg_value_0[607:600];
                      hdr.hdr_3.data[439:432] = vector_reg_value_0[615:608];
                      hdr.hdr_3.data[431:424] = vector_reg_value_0[623:616];
                      hdr.hdr_3.data[423:416] = vector_reg_value_0[631:624];
                      hdr.hdr_3.data[415:408] = vector_reg_value_0[639:632];
                      hdr.hdr_3.data[407:400] = vector_reg_value_0[647:640];
                      hdr.hdr_3.data[399:392] = vector_reg_value_0[655:648];
                      hdr.hdr_3.data[391:384] = vector_reg_value_0[663:656];
                      hdr.hdr_3.data[383:376] = vector_reg_value_0[671:664];
                      hdr.hdr_3.data[375:368] = vector_reg_value_0[679:672];
                      hdr.hdr_3.data[367:360] = vector_reg_value_0[687:680];
                      hdr.hdr_3.data[359:352] = vector_reg_value_0[695:688];
                      hdr.hdr_3.data[351:344] = vector_reg_value_0[703:696];
                      hdr.hdr_3.data[343:336] = vector_reg_value_0[711:704];
                      hdr.hdr_3.data[335:328] = vector_reg_value_0[719:712];
                      hdr.hdr_3.data[327:320] = vector_reg_value_0[727:720];
                      hdr.hdr_3.data[319:312] = vector_reg_value_0[735:728];
                      hdr.hdr_3.data[311:304] = vector_reg_value_0[743:736];
                      hdr.hdr_3.data[303:296] = vector_reg_value_0[751:744];
                      hdr.hdr_3.data[295:288] = vector_reg_value_0[759:752];
                      hdr.hdr_3.data[287:280] = vector_reg_value_0[767:760];
                      hdr.hdr_3.data[279:272] = vector_reg_value_0[775:768];
                      hdr.hdr_3.data[271:264] = vector_reg_value_0[783:776];
                      hdr.hdr_3.data[263:256] = vector_reg_value_0[791:784];
                      hdr.hdr_3.data[255:248] = vector_reg_value_0[799:792];
                      hdr.hdr_3.data[247:240] = vector_reg_value_0[807:800];
                      hdr.hdr_3.data[239:232] = vector_reg_value_0[815:808];
                      hdr.hdr_3.data[231:224] = vector_reg_value_0[823:816];
                      hdr.hdr_3.data[223:216] = vector_reg_value_0[831:824];
                      hdr.hdr_3.data[215:208] = vector_reg_value_0[839:832];
                      hdr.hdr_3.data[207:200] = vector_reg_value_0[847:840];
                      hdr.hdr_3.data[199:192] = vector_reg_value_0[855:848];
                      hdr.hdr_3.data[191:184] = vector_reg_value_0[863:856];
                      hdr.hdr_3.data[183:176] = vector_reg_value_0[871:864];
                      hdr.hdr_3.data[175:168] = vector_reg_value_0[879:872];
                      hdr.hdr_3.data[167:160] = vector_reg_value_0[887:880];
                      hdr.hdr_3.data[159:152] = vector_reg_value_0[895:888];
                      hdr.hdr_3.data[151:144] = vector_reg_value_0[903:896];
                      hdr.hdr_3.data[143:136] = vector_reg_value_0[911:904];
                      hdr.hdr_3.data[135:128] = vector_reg_value_0[919:912];
                      hdr.hdr_3.data[127:120] = vector_reg_value_0[927:920];
                      hdr.hdr_3.data[119:112] = vector_reg_value_0[935:928];
                      hdr.hdr_3.data[111:104] = vector_reg_value_0[943:936];
                      hdr.hdr_3.data[103:96] = vector_reg_value_0[951:944];
                      hdr.hdr_3.data[95:88] = vector_reg_value_0[959:952];
                      hdr.hdr_3.data[87:80] = vector_reg_value_0[967:960];
                      hdr.hdr_3.data[79:72] = vector_reg_value_0[975:968];
                      hdr.hdr_3.data[71:64] = vector_reg_value_0[983:976];
                      hdr.hdr_3.data[63:56] = vector_reg_value_0[991:984];
                      hdr.hdr_3.data[55:48] = vector_reg_value_0[999:992];
                      hdr.hdr_3.data[47:40] = vector_reg_value_0[1007:1000];
                      hdr.hdr_3.data[39:32] = vector_reg_value_0[1015:1008];
                      hdr.hdr_3.data[31:24] = vector_reg_value_0[1023:1016];
                      hdr.hdr_3.data[23:16] = 8w0x01;
                      // EP node  5639
                      // BDD node 49:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760000)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr_2.data[63:56], hdr.hdr_2.data[47:40]);
                      swap(hdr.hdr_2.data[55:48], hdr.hdr_2.data[39:32]);
                      // EP node  5927
                      // BDD node 50:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759744)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr_1.data[63:56], hdr.hdr_1.data[31:24]);
                      swap(hdr.hdr_1.data[55:48], hdr.hdr_1.data[23:16]);
                      swap(hdr.hdr_1.data[47:40], hdr.hdr_1.data[15:8]);
                      swap(hdr.hdr_1.data[39:32], hdr.hdr_1.data[7:0]);
                      // EP node  6221
                      // BDD node 51:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759488)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr_0.data[111:104], hdr.hdr_0.data[63:56]);
                      swap(hdr.hdr_0.data[103:96], hdr.hdr_0.data[55:48]);
                      swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                      swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                      swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                      swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                      // EP node  6521
                      // BDD node 52:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    }
                  } else {
                    // EP node  2022
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  3513
                    // BDD node 54:vector_return(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) allocated_index_r1), value:(w64 1073970104)[(ReadLSB w1024 (w32 785) packet_chunks)])
                    vector_register_1073956208_0_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_1_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_2_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_3_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_4_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_5_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_6_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_7_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_8_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_9_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_10_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_11_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_12_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_13_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_14_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_15_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_16_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_17_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_18_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_19_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_20_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_21_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_22_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_23_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_24_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_25_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_26_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_27_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_28_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_29_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_30_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    vector_register_1073956208_31_write_3513.execute(map_table_1073923128_13_get_value_param_0);
                    // EP node  3791
                    // BDD node 55:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (ReadLSB w1160 (w32 768) packet_chunks))))])
                    hdr.hdr_3.data[23:16] = 8w0x01;
                    // EP node  4037
                    // BDD node 56:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760000)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr_2.data[63:56], hdr.hdr_2.data[47:40]);
                    swap(hdr.hdr_2.data[55:48], hdr.hdr_2.data[39:32]);
                    // EP node  4289
                    // BDD node 57:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759744)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr_1.data[63:56], hdr.hdr_1.data[31:24]);
                    swap(hdr.hdr_1.data[55:48], hdr.hdr_1.data[23:16]);
                    swap(hdr.hdr_1.data[47:40], hdr.hdr_1.data[15:8]);
                    swap(hdr.hdr_1.data[39:32], hdr.hdr_1.data[7:0]);
                    // EP node  4547
                    // BDD node 58:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759488)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr_0.data[111:104], hdr.hdr_0.data[63:56]);
                    swap(hdr.hdr_0.data[103:96], hdr.hdr_0.data[55:48]);
                    swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                    swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                    swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                    swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                    // EP node  4811
                    // BDD node 59:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                    trigger_forward = true;
                  }
                }
              } else {
                // EP node  756
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  1547
                // BDD node 64:FORWARD
                nf_dev[15:0] = hdr.hdr_3.data[15:0];
                trigger_forward = true;
              }
            }
            // EP node  88
            // BDD node 10:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks)) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  347
            // BDD node 68:DROP
          }
          // EP node  44
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  170
          // BDD node 71:DROP
        }
        // EP node  16
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  861
        // BDD node 73:DROP
      }
      ig_tm_md.bypass_egress = 1;
    }

    if (trigger_forward) {
      forward_nf_dev.apply();
    }
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
