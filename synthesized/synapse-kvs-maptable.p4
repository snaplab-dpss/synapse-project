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
  bit<16> code_path;                  // Written by the data plane
  bit<16> egress_dev;                 // Written by the control plane
  bit<8> trigger_dataplane_execution; // Written by the control plane
  bit<32> dev;

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
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr0.data2) {
      16w0x0800: parser_7;
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
    transition parser_8_0;
  }
  state parser_8_0 {
    transition select (hdr.hdr1.data1[23:16]) {
      8w0x11: parser_9;
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

  bit<32> map_table_1073923288_13_get_value_param0 = 32w0;
  action map_table_1073923288_13_get_value(bit<32> _map_table_1073923288_13_get_value_param0) {
    map_table_1073923288_13_get_value_param0 = _map_table_1073923288_13_get_value_param0;
  }

  bit<128> map_table_1073923288_13_key0 = 128w0;
  table map_table_1073923288_13 {
    key = {
      map_table_1073923288_13_key0: exact;
    }
    actions = {
      map_table_1073923288_13_get_value;
    }
    size = 8192;
    idle_timeout = true;
  }

  Register<bit<32>,_>(8192, 0) vector_register_1073956368_0;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_1;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_2;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_3;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_4;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_5;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_6;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_7;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_8;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_9;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_10;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_11;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_12;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_13;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_14;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_15;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_16;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_17;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_18;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_19;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_20;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_21;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_22;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_23;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_24;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_25;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_26;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_27;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_28;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_29;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_30;
  Register<bit<32>,_>(8192, 0) vector_register_1073956368_31;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_0) vector_register_1073956368_0_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_1) vector_register_1073956368_1_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_2) vector_register_1073956368_2_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_3) vector_register_1073956368_3_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_4) vector_register_1073956368_4_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_5) vector_register_1073956368_5_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_6) vector_register_1073956368_6_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_7) vector_register_1073956368_7_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_8) vector_register_1073956368_8_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_9) vector_register_1073956368_9_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_10) vector_register_1073956368_10_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_11) vector_register_1073956368_11_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_12) vector_register_1073956368_12_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_13) vector_register_1073956368_13_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_14) vector_register_1073956368_14_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_15) vector_register_1073956368_15_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_16) vector_register_1073956368_16_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_17) vector_register_1073956368_17_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_18) vector_register_1073956368_18_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_19) vector_register_1073956368_19_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_20) vector_register_1073956368_20_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_21) vector_register_1073956368_21_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_22) vector_register_1073956368_22_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_23) vector_register_1073956368_23_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_24) vector_register_1073956368_24_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_25) vector_register_1073956368_25_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_26) vector_register_1073956368_26_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_27) vector_register_1073956368_27_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_28) vector_register_1073956368_28_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_29) vector_register_1073956368_29_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_30) vector_register_1073956368_30_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073956368_31) vector_register_1073956368_31_read_442 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> dchain_table_1073989880_38_key0 = 32w0;
  table dchain_table_1073989880_38 {
    key = {
      dchain_table_1073989880_38_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 8192;
    idle_timeout = true;
  }


  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_0) vector_register_1073956368_0_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[1023:992];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_1) vector_register_1073956368_1_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[991:960];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_2) vector_register_1073956368_2_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[959:928];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_3) vector_register_1073956368_3_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[927:896];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_4) vector_register_1073956368_4_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[895:864];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_5) vector_register_1073956368_5_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[863:832];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_6) vector_register_1073956368_6_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[831:800];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_7) vector_register_1073956368_7_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[799:768];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_8) vector_register_1073956368_8_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[767:736];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_9) vector_register_1073956368_9_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[735:704];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_10) vector_register_1073956368_10_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[703:672];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_11) vector_register_1073956368_11_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[671:640];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_12) vector_register_1073956368_12_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[639:608];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_13) vector_register_1073956368_13_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[607:576];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_14) vector_register_1073956368_14_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[575:544];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_15) vector_register_1073956368_15_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[543:512];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_16) vector_register_1073956368_16_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[511:480];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_17) vector_register_1073956368_17_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[479:448];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_18) vector_register_1073956368_18_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[447:416];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_19) vector_register_1073956368_19_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[415:384];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_20) vector_register_1073956368_20_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[383:352];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_21) vector_register_1073956368_21_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[351:320];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_22) vector_register_1073956368_22_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[319:288];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_23) vector_register_1073956368_23_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[287:256];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_24) vector_register_1073956368_24_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[255:224];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_25) vector_register_1073956368_25_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[223:192];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_26) vector_register_1073956368_26_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[191:160];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_27) vector_register_1073956368_27_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[159:128];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_28) vector_register_1073956368_28_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[127:96];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_29) vector_register_1073956368_29_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[95:64];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_30) vector_register_1073956368_30_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[63:32];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073956368_31) vector_register_1073956368_31_write_1887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2[31:0];
    }
  };


  apply {
    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:Ignore
      // BDD node 4:expire_items_single_map(chain:(w64 1073989880), vector:(w64 1073937112), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073923288))
      // EP node  4:ParserExtraction
      // BDD node 5:packet_borrow_next_chunk(p:(w64 1074049568), length:(w32 14), chunk:(w64 1074060272)[ -> (w64 1073759648)])
      if(hdr.hdr0.isValid()) {
        // EP node  12:ParserCondition
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  13:Then
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  22:ParserExtraction
        // BDD node 7:packet_borrow_next_chunk(p:(w64 1074049568), length:(w32 20), chunk:(w64 1074061008)[ -> (w64 1073759904)])
        if(hdr.hdr1.isValid()) {
          // EP node  43:ParserCondition
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  44:Then
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  58:ParserExtraction
          // BDD node 9:packet_borrow_next_chunk(p:(w64 1074049568), length:(w32 8), chunk:(w64 1074061664)[ -> (w64 1073760160)])
          if(hdr.hdr2.isValid()) {
            // EP node  92:ParserCondition
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  93:Then
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  125:If
            // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
            if ((16w0x0000) != (meta.dev[15:0])) {
              // EP node  126:Then
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              // EP node  165:ParserExtraction
              // BDD node 11:packet_borrow_next_chunk(p:(w64 1074049568), length:(w32 148), chunk:(w64 1074062408)[ -> (w64 1073760416)])
              if(hdr.hdr3.isValid()) {
                // EP node  208:MapTableLookup
                // BDD node 13:map_get(map:(w64 1073923288), key:(w64 1073760417)[(ReadLSB w128 (w32 769) packet_chunks) -> (ReadLSB w128 (w32 769) packet_chunks)], value_out:(w64 1074059584)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                map_table_1073923288_13_key0 = hdr.hdr3.data1;
                bool hit0 = map_table_1073923288_13.apply().hit;
                // EP node  442:VectorRegisterLookup
                // BDD node 53:vector_borrow(vector:(w64 1073956368), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074062880)[ -> (w64 1073970264)])
                // EP node  570:If
                // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                if (!hit0) {
                  // EP node  571:Then
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  720:If
                  // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr3.data0)) {
                    // EP node  721:Then
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  787:SendToController
                    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073989880), index_out:(w64 1074063328)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    send_to_controller(787);
                    hdr.cpu.dev = meta.dev;
                  } else {
                    // EP node  722:Else
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  4268:ModifyHeader
                    // BDD node 33:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073760416)[(Concat w1184 (Read w8 (w32 1) DEVICE) (Concat w1176 (Read w8 (w32 0) DEVICE) (ReadLSB w1168 (w32 768) packet_chunks)))])
                    hdr.hdr3.data4[15:8] = meta.dev[7:0];
                    hdr.hdr3.data4[7:0] = meta.dev[15:8];
                    // EP node  9656:Forward
                    // BDD node 37:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                    trigger_forward = true;
                  }
                } else {
                  // EP node  572:Else
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  1301:DchainTableLookup
                  // BDD node 38:dchain_rejuvenate_index(chain:(w64 1073989880), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  dchain_table_1073989880_38_key0 = map_table_1073923288_13_get_value_param0;
                  dchain_table_1073989880_38.apply();
                  // EP node  1583:If
                  // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                  if ((8w0x01) != (hdr.hdr3.data0)) {
                    // EP node  1584:Then
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  4566:If
                    // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                    if ((8w0x00) != (hdr.hdr3.data0)) {
                      // EP node  4567:Then
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  5809:ModifyHeader
                      // BDD node 41:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073760416)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (ReadLSB w1160 (w32 768) packet_chunks))))])
                      hdr.hdr3.data3 = 8w0x01;
                      // EP node  7312:ModifyHeader
                      // BDD node 42:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073760160)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  8750:ModifyHeader
                      // BDD node 43:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073759904)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  10091:ModifyHeader
                      // BDD node 44:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073759648)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  10963:Forward
                      // BDD node 45:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    } else {
                      // EP node  4568:Else
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  6136:Ignore
                      // BDD node 47:vector_return(vector:(w64 1073956368), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073970264)[(ReadLSB w1024 (w32 0) vector_data_r1)])
                      // EP node  7717:ModifyHeader
                      // BDD node 48:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073760416)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (Concat w1160 (Read w8 (w32 127) vector_data_r1) (Concat w1152 (Read w8 (w32 126) vector_data_r1) (Concat w1144 (Read w8 (w32 125) vector_data_r1) (Concat w1136 (Read w8 (w32 124) vector_data_r1) (Concat w1128 (Read w8 (w32 123) vector_data_r1) (Concat w1120 (Read w8 (w32 122) vector_data_r1) (Concat w1112 (Read w8 (w32 121) vector_data_r1) (Concat w1104 (Read w8 (w32 120) vector_data_r1) (Concat w1096 (Read w8 (w32 119) vector_data_r1) (Concat w1088 (Read w8 (w32 118) vector_data_r1) (Concat w1080 (Read w8 (w32 117) vector_data_r1) (Concat w1072 (Read w8 (w32 116) vector_data_r1) (Concat w1064 (Read w8 (w32 115) vector_data_r1) (Concat w1056 (Read w8 (w32 114) vector_data_r1) (Concat w1048 (Read w8 (w32 113) vector_data_r1) (Concat w1040 (Read w8 (w32 112) vector_data_r1) (Concat w1032 (Read w8 (w32 111) vector_data_r1) (Concat w1024 (Read w8 (w32 110) vector_data_r1) (Concat w1016 (Read w8 (w32 109) vector_data_r1) (Concat w1008 (Read w8 (w32 108) vector_data_r1) (Concat w1000 (Read w8 (w32 107) vector_data_r1) (Concat w992 (Read w8 (w32 106) vector_data_r1) (Concat w984 (Read w8 (w32 105) vector_data_r1) (Concat w976 (Read w8 (w32 104) vector_data_r1) (Concat w968 (Read w8 (w32 103) vector_data_r1) (Concat w960 (Read w8 (w32 102) vector_data_r1) (Concat w952 (Read w8 (w32 101) vector_data_r1) (Concat w944 (Read w8 (w32 100) vector_data_r1) (Concat w936 (Read w8 (w32 99) vector_data_r1) (Concat w928 (Read w8 (w32 98) vector_data_r1) (Concat w920 (Read w8 (w32 97) vector_data_r1) (Concat w912 (Read w8 (w32 96) vector_data_r1) (Concat w904 (Read w8 (w32 95) vector_data_r1) (Concat w896 (Read w8 (w32 94) vector_data_r1) (Concat w888 (Read w8 (w32 93) vector_data_r1) (Concat w880 (Read w8 (w32 92) vector_data_r1) (Concat w872 (Read w8 (w32 91) vector_data_r1) (Concat w864 (Read w8 (w32 90) vector_data_r1) (Concat w856 (Read w8 (w32 89) vector_data_r1) (Concat w848 (Read w8 (w32 88) vector_data_r1) (Concat w840 (Read w8 (w32 87) vector_data_r1) (Concat w832 (Read w8 (w32 86) vector_data_r1) (Concat w824 (Read w8 (w32 85) vector_data_r1) (Concat w816 (Read w8 (w32 84) vector_data_r1) (Concat w808 (Read w8 (w32 83) vector_data_r1) (Concat w800 (Read w8 (w32 82) vector_data_r1) (Concat w792 (Read w8 (w32 81) vector_data_r1) (Concat w784 (Read w8 (w32 80) vector_data_r1) (Concat w776 (Read w8 (w32 79) vector_data_r1) (Concat w768 (Read w8 (w32 78) vector_data_r1) (Concat w760 (Read w8 (w32 77) vector_data_r1) (Concat w752 (Read w8 (w32 76) vector_data_r1) (Concat w744 (Read w8 (w32 75) vector_data_r1) (Concat w736 (Read w8 (w32 74) vector_data_r1) (Concat w728 (Read w8 (w32 73) vector_data_r1) (Concat w720 (Read w8 (w32 72) vector_data_r1) (Concat w712 (Read w8 (w32 71) vector_data_r1) (Concat w704 (Read w8 (w32 70) vector_data_r1) (Concat w696 (Read w8 (w32 69) vector_data_r1) (Concat w688 (Read w8 (w32 68) vector_data_r1) (Concat w680 (Read w8 (w32 67) vector_data_r1) (Concat w672 (Read w8 (w32 66) vector_data_r1) (Concat w664 (Read w8 (w32 65) vector_data_r1) (Concat w656 (Read w8 (w32 64) vector_data_r1) (Concat w648 (Read w8 (w32 63) vector_data_r1) (Concat w640 (Read w8 (w32 62) vector_data_r1) (Concat w632 (Read w8 (w32 61) vector_data_r1) (Concat w624 (Read w8 (w32 60) vector_data_r1) (Concat w616 (Read w8 (w32 59) vector_data_r1) (Concat w608 (Read w8 (w32 58) vector_data_r1) (Concat w600 (Read w8 (w32 57) vector_data_r1) (Concat w592 (Read w8 (w32 56) vector_data_r1) (Concat w584 (Read w8 (w32 55) vector_data_r1) (Concat w576 (Read w8 (w32 54) vector_data_r1) (Concat w568 (Read w8 (w32 53) vector_data_r1) (Concat w560 (Read w8 (w32 52) vector_data_r1) (Concat w552 (Read w8 (w32 51) vector_data_r1) (Concat w544 (Read w8 (w32 50) vector_data_r1) (Concat w536 (Read w8 (w32 49) vector_data_r1) (Concat w528 (Read w8 (w32 48) vector_data_r1) (Concat w520 (Read w8 (w32 47) vector_data_r1) (Concat w512 (Read w8 (w32 46) vector_data_r1) (Concat w504 (Read w8 (w32 45) vector_data_r1) (Concat w496 (Read w8 (w32 44) vector_data_r1) (Concat w488 (Read w8 (w32 43) vector_data_r1) (Concat w480 (Read w8 (w32 42) vector_data_r1) (Concat w472 (Read w8 (w32 41) vector_data_r1) (Concat w464 (Read w8 (w32 40) vector_data_r1) (Concat w456 (Read w8 (w32 39) vector_data_r1) (Concat w448 (Read w8 (w32 38) vector_data_r1) (Concat w440 (Read w8 (w32 37) vector_data_r1) (Concat w432 (Read w8 (w32 36) vector_data_r1) (Concat w424 (Read w8 (w32 35) vector_data_r1) (Concat w416 (Read w8 (w32 34) vector_data_r1) (Concat w408 (Read w8 (w32 33) vector_data_r1) (Concat w400 (Read w8 (w32 32) vector_data_r1) (Concat w392 (Read w8 (w32 31) vector_data_r1) (Concat w384 (Read w8 (w32 30) vector_data_r1) (Concat w376 (Read w8 (w32 29) vector_data_r1) (Concat w368 (Read w8 (w32 28) vector_data_r1) (Concat w360 (Read w8 (w32 27) vector_data_r1) (Concat w352 (Read w8 (w32 26) vector_data_r1) (Concat w344 (Read w8 (w32 25) vector_data_r1) (Concat w336 (Read w8 (w32 24) vector_data_r1) (Concat w328 (Read w8 (w32 23) vector_data_r1) (Concat w320 (Read w8 (w32 22) vector_data_r1) (Concat w312 (Read w8 (w32 21) vector_data_r1) (Concat w304 (Read w8 (w32 20) vector_data_r1) (Concat w296 (Read w8 (w32 19) vector_data_r1) (Concat w288 (Read w8 (w32 18) vector_data_r1) (Concat w280 (Read w8 (w32 17) vector_data_r1) (Concat w272 (Read w8 (w32 16) vector_data_r1) (Concat w264 (Read w8 (w32 15) vector_data_r1) (Concat w256 (Read w8 (w32 14) vector_data_r1) (Concat w248 (Read w8 (w32 13) vector_data_r1) (Concat w240 (Read w8 (w32 12) vector_data_r1) (Concat w232 (Read w8 (w32 11) vector_data_r1) (Concat w224 (Read w8 (w32 10) vector_data_r1) (Concat w216 (Read w8 (w32 9) vector_data_r1) (Concat w208 (Read w8 (w32 8) vector_data_r1) (Concat w200 (Read w8 (w32 7) vector_data_r1) (Concat w192 (Read w8 (w32 6) vector_data_r1) (Concat w184 (Read w8 (w32 5) vector_data_r1) (Concat w176 (Read w8 (w32 4) vector_data_r1) (Concat w168 (Read w8 (w32 3) vector_data_r1) (Concat w160 (Read w8 (w32 2) vector_data_r1) (Concat w152 (Read w8 (w32 1) vector_data_r1) (Concat w144 (Read w8 (w32 0) vector_data_r1) (ReadLSB w136 (w32 768) packet_chunks))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))])
                      hdr.hdr3.data2[1023:992] = vector_register_1073956368_0_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[895:864] = vector_register_1073956368_4_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[767:736] = vector_register_1073956368_8_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[639:608] = vector_register_1073956368_12_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[511:480] = vector_register_1073956368_16_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[383:352] = vector_register_1073956368_20_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[255:224] = vector_register_1073956368_24_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[127:96] = vector_register_1073956368_28_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data3 = 8w0x01;
                      hdr.hdr3.data2[991:960] = vector_register_1073956368_1_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[863:832] = vector_register_1073956368_5_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[735:704] = vector_register_1073956368_9_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[607:576] = vector_register_1073956368_13_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[479:448] = vector_register_1073956368_17_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[351:320] = vector_register_1073956368_21_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[223:192] = vector_register_1073956368_25_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[95:64] = vector_register_1073956368_29_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[959:928] = vector_register_1073956368_2_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[831:800] = vector_register_1073956368_6_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[703:672] = vector_register_1073956368_10_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[575:544] = vector_register_1073956368_14_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[447:416] = vector_register_1073956368_18_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[319:288] = vector_register_1073956368_22_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[191:160] = vector_register_1073956368_26_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[63:32] = vector_register_1073956368_30_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[927:896] = vector_register_1073956368_3_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[799:768] = vector_register_1073956368_7_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[671:640] = vector_register_1073956368_11_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[543:512] = vector_register_1073956368_15_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[415:384] = vector_register_1073956368_19_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[287:256] = vector_register_1073956368_23_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[159:128] = vector_register_1073956368_27_read_442.execute(map_table_1073923288_13_get_value_param0);
                      hdr.hdr3.data2[31:0] = vector_register_1073956368_31_read_442.execute(map_table_1073923288_13_get_value_param0);
                      // EP node  9173:ModifyHeader
                      // BDD node 49:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073760160)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  10532:ModifyHeader
                      // BDD node 50:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073759904)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  11422:ModifyHeader
                      // BDD node 51:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073759648)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  11792:Forward
                      // BDD node 52:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    }
                  } else {
                    // EP node  1585:Else
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  1887:VectorRegisterUpdate
                    // BDD node 54:vector_return(vector:(w64 1073956368), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073970264)[(ReadLSB w1024 (w32 785) packet_chunks)])
                    vector_register_1073956368_0_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_1_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_2_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_3_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_4_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_5_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_6_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_7_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_8_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_9_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_10_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_11_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_12_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_13_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_14_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_15_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_16_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_17_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_18_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_19_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_20_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_21_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_22_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_23_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_24_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_25_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_26_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_27_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_28_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_29_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_30_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    vector_register_1073956368_31_write_1887.execute(map_table_1073923288_13_get_value_param0);
                    // EP node  2245:ModifyHeader
                    // BDD node 55:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073760416)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 1) (ReadLSB w1160 (w32 768) packet_chunks))))])
                    hdr.hdr3.data3 = 8w0x01;
                    // EP node  2562:ModifyHeader
                    // BDD node 56:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073760160)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                    // EP node  2885:ModifyHeader
                    // BDD node 57:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073759904)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                    swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                    swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                    swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                    // EP node  3214:ModifyHeader
                    // BDD node 58:packet_return_chunk(p:(w64 1074049568), the_chunk:(w64 1073759648)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                    swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                    // EP node  3479:Forward
                    // BDD node 59:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                    trigger_forward = true;
                  }
                }
              }
            } else {
              // EP node  127:Else
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              // EP node  3868:ParserExtraction
              // BDD node 138:packet_borrow_next_chunk(p:(w64 1074049568), length:(w32 148), chunk:(w64 1074062408)[ -> (w64 1073760416)])
              if(hdr.hdr3.isValid()) {
                // EP node  10603:Forward
                // BDD node 64:FORWARD
                nf_dev[15:0] = bswap16(hdr.hdr3.data4);
                trigger_forward = true;
              }
            }
            // EP node  94:Else
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 148) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  7782:ParserReject
            // BDD node 68:DROP
          }
          // EP node  45:Else
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  6258:ParserReject
          // BDD node 71:DROP
        }
        // EP node  14:Else
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  4682:ParserReject
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
