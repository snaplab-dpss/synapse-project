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
  @padding bit<7> pad_in_port;
  bit<9> in_port;
  @padding bit<7> pad_out_port;
  bit<9> out_port;
  @padding bit<7> pad_hit_0;
  bool hit_0;
  @padding bit<8> pad_table_1073912440_14_value_0;
  bit<32> table_1073912440_14_value_0;
  @padding bit<8> pad_table_1073952504_39_key_0;
  bit<32> table_1073952504_39_key_0;

}

header recirc_h {
  bit<16> code_path;
/*@{RECIRCULATION_HEADER}@*/
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
  bit<1136> data;
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
  bit<16> port;
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
    meta.port = 7w0 ++ ig_intr_md.ingress_port;
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
    transition parser_2;
  }
  state parser_2 {
    transition select (hdr.hdr_0.data[111:96]) {
      2048: parser_3;
      default: parser_77;
    }
  }
  state parser_3 {
    pkt.extract(hdr.hdr_1);
    transition parser_4;
  }
  state parser_77 {
    transition reject;
  }
  state parser_4 {
    transition select (hdr.hdr_1.data[79:72]) {
      17: parser_5;
      default: parser_75;
    }
  }
  state parser_5 {
    pkt.extract(hdr.hdr_2);
    transition parser_6;
  }
  state parser_75 {
    transition reject;
  }
  state parser_6 {
    transition select (hdr.hdr_2.data[31:16]) {
      670: parser_7;
      default: parser_72;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr_3);
    transition parser_68;
  }
  state parser_72 {
    transition reject;
  }
  state parser_68 {
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

  action fwd(bit<9> port) {
    ig_tm_md.ucast_egress_port = port;
  }

  action send_to_controller(bit<16> code_path) {
    hdr.cpu.setValid();
    hdr.cpu.code_path = code_path;
    hdr.cpu.in_port = ig_intr_md.ingress_port;
    fwd(CPU_PCIE_PORT);
  }

  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

  bit<32> table_1073912440_14_value_0 = 32w0;
  action table_1073912440_14_get_value(bit<32> _table_1073912440_14_value_0) {
    table_1073912440_14_value_0 = _table_1073912440_14_value_0;
  }

  bit<96> table_1073912440_14_key_0 = 96w0;
  table table_1073912440_14 {
    key = {
      table_1073912440_14_key_0: exact;
    }
    actions = {
      table_1073912440_14_get_value;
    }
    size = 8192;
  }

  bit<32> table_1073952504_39_key_0 = 32w0;
  table table_1073952504_39 {
    key = {
      table_1073952504_39_key_0: exact;
    }
    actions = {}
    size = 8192;
  }

  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_0;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_1;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_2;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_3;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_4;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_5;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_6;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_7;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_8;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_9;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_10;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_11;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_12;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_13;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_14;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_15;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_16;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_17;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_18;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_19;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_20;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_21;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_22;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_23;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_24;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_25;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_26;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_27;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_28;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_29;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_30;
  Register<bit<32>,_>(8192, 0) vector_reg_1073938944_31;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_0) vector_reg_1073938944_0_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_1) vector_reg_1073938944_1_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_2) vector_reg_1073938944_2_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_3) vector_reg_1073938944_3_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_4) vector_reg_1073938944_4_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_5) vector_reg_1073938944_5_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_6) vector_reg_1073938944_6_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_7) vector_reg_1073938944_7_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_8) vector_reg_1073938944_8_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_9) vector_reg_1073938944_9_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_10) vector_reg_1073938944_10_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_11) vector_reg_1073938944_11_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_12) vector_reg_1073938944_12_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_13) vector_reg_1073938944_13_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_14) vector_reg_1073938944_14_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_15) vector_reg_1073938944_15_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_16) vector_reg_1073938944_16_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_17) vector_reg_1073938944_17_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_18) vector_reg_1073938944_18_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_19) vector_reg_1073938944_19_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_20) vector_reg_1073938944_20_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_21) vector_reg_1073938944_21_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_22) vector_reg_1073938944_22_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_23) vector_reg_1073938944_23_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_24) vector_reg_1073938944_24_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_25) vector_reg_1073938944_25_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_26) vector_reg_1073938944_26_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_27) vector_reg_1073938944_27_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_28) vector_reg_1073938944_28_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_29) vector_reg_1073938944_29_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_30) vector_reg_1073938944_30_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_31) vector_reg_1073938944_31_read_3809 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_0) vector_reg_1073938944_0_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[135:104];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_1) vector_reg_1073938944_1_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[167:136];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_2) vector_reg_1073938944_2_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[199:168];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_3) vector_reg_1073938944_3_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[231:200];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_4) vector_reg_1073938944_4_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[263:232];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_5) vector_reg_1073938944_5_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[295:264];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_6) vector_reg_1073938944_6_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[327:296];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_7) vector_reg_1073938944_7_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[359:328];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_8) vector_reg_1073938944_8_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[391:360];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_9) vector_reg_1073938944_9_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[423:392];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_10) vector_reg_1073938944_10_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[455:424];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_11) vector_reg_1073938944_11_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[487:456];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_12) vector_reg_1073938944_12_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[519:488];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_13) vector_reg_1073938944_13_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[551:520];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_14) vector_reg_1073938944_14_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[583:552];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_15) vector_reg_1073938944_15_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[615:584];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_16) vector_reg_1073938944_16_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[647:616];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_17) vector_reg_1073938944_17_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[679:648];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_18) vector_reg_1073938944_18_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[711:680];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_19) vector_reg_1073938944_19_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[743:712];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_20) vector_reg_1073938944_20_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[775:744];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_21) vector_reg_1073938944_21_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[807:776];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_22) vector_reg_1073938944_22_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[839:808];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_23) vector_reg_1073938944_23_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[871:840];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_24) vector_reg_1073938944_24_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[903:872];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_25) vector_reg_1073938944_25_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[935:904];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_26) vector_reg_1073938944_26_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[967:936];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_27) vector_reg_1073938944_27_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[999:968];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_28) vector_reg_1073938944_28_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[1031:1000];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_29) vector_reg_1073938944_29_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[1063:1032];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_30) vector_reg_1073938944_30_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[1095:1064];
    }
  };

  RegisterAction<bit<32>, bit<32>, void>(vector_reg_1073938944_31) vector_reg_1073938944_31_write_887 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr_3.data[1127:1096];
    }
  };


  apply {
    if (hdr.cpu.isValid()) {
      hdr.cpu.setInvalid();
      fwd(hdr.cpu.out_port);
    } else if (hdr.recirc.isValid()) {
      
    } else {
      // Node 1
      // BDD node 0:expire_items_single_map(chain:(w64 1073952504), vector:(w64 1073926224), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073912440))
      // Node 4
      // BDD node 1:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 14), chunk:(w64 1074059888)[ -> (w64 1073764528)])
      if(hdr.hdr_0.isValid()) {
        // Node 11
        // BDD node 2:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // Node 12
        // BDD node 2:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // Node 20
        // BDD node 3:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 20), chunk:(w64 1074060656)[ -> (w64 1073764675)])
        if(hdr.hdr_1.isValid()) {
          // Node 39
          // BDD node 4:if ((And (Eq (w8 17) (Read w8 (w32 156) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // Node 40
          // BDD node 4:if ((And (Eq (w8 17) (Read w8 (w32 156) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // Node 52
          // BDD node 5:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 8), chunk:(w64 1074061344)[ -> (w64 1073764822)])
          if(hdr.hdr_2.isValid()) {
            // Node 83
            // BDD node 6:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 296) packet_chunks)) (Ule (w64 142) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // Node 84
            // BDD node 6:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 296) packet_chunks)) (Ule (w64 142) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // Node 100
            // BDD node 7:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 142), chunk:(w64 1074062088)[ -> (w64 1073764969)])
            if(hdr.hdr_3.isValid()) {
              // Node 129
              // BDD node 8:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              if (16w0000 != meta.port) {
                // Node 130
                // BDD node 8:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // Node 6601
                // BDD node 13:FORWARD
                fwd(0);
              } else {
                // Node 131
                // BDD node 8:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // Node 164
                // BDD node 14:map_get(map:(w64 1073912440), key:(w64 1073764970)[(ReadLSB w96 (w32 442) packet_chunks) -> (ReadLSB w96 (w32 442) packet_chunks)], value_out:(w64 1074075440)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                table_1073912440_14_key_0 = hdr.hdr_3.data[103:8];
                bool hit_0 = table_1073912440_14.apply().hit;
                // Node 277
                // BDD node 15:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                if (!hit_0) {
                  // Node 278
                  // BDD node 15:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // Node 1949
                  // BDD node 16:if ((Eq (w8 1) (Read w8 (w32 441) packet_chunks))
                  if (8w01 == hdr.hdr_3.data[7:0]) {
                    // Node 1950
                    // BDD node 16:if ((Eq (w8 1) (Read w8 (w32 441) packet_chunks))
                    // Node 2017
                    // BDD node 17:dchain_allocate_new_index(chain:(w64 1073952504), index_out:(w64 1074079288)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    hdr.cpu.hit_0 = hit_0;
                    hdr.cpu.table_1073912440_14_value_0 = table_1073912440_14_value_0;
                    send_to_controller(2017);
                    // Node 9218
                    // BDD node 150:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 14), chunk:(w64 1074059888)[ -> (w64 1073764528)])
                    // Node 9277
                    // BDD node 151:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 20), chunk:(w64 1074060656)[ -> (w64 1073764675)])
                    // Node 9337
                    // BDD node 152:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 8), chunk:(w64 1074061344)[ -> (w64 1073764822)])
                    // Node 9398
                    // BDD node 153:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 142), chunk:(w64 1074062088)[ -> (w64 1073764969)])
                    // Node 9460
                    // BDD node 17:dchain_allocate_new_index(chain:(w64 1073952504), index_out:(w64 1074079288)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    // Node 9523
                    // BDD node 18:if ((Eq false (Extract 0 (ZExt w8 (Eq false (Eq (w32 0) (And w32 (ZExt w32 (Extract 0 (ZExt w8 (Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) out_of_space_1)))))) (ZExt w32 (Eq (w32 0) (ReadLSB w32 (w32 0) number_of_freed_flows)))))))))
                    // Node 9524
                    // BDD node 18:if ((Eq false (Extract 0 (ZExt w8 (Eq false (Eq (w32 0) (And w32 (ZExt w32 (Extract 0 (ZExt w8 (Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) out_of_space_1)))))) (ZExt w32 (Eq (w32 0) (ReadLSB w32 (w32 0) number_of_freed_flows)))))))))
                    // Node 9988
                    // BDD node 20:map_put(map:(w64 1073912440), key:(w64 1073938624)[(ReadLSB w96 (w32 442) packet_chunks) -> (ReadLSB w96 (w32 442) packet_chunks)], value:(ReadLSB w32 (w32 0) new_index))
                    // Node 10127
                    // BDD node 22:vector_borrow(vector:(w64 1073938944), index:(ReadLSB w32 (w32 0) new_index), val_out:(w64 1074079344)[ -> (w64 1073951344)])
                    // Node 10342
                    // BDD node 23:vector_return(vector:(w64 1073938944), index:(ReadLSB w32 (w32 0) new_index), value:(w64 1073951344)[(ReadLSB w1024 (w32 454) packet_chunks)])
                    // Node 10641
                    // BDD node 25:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764822)[(Concat w64 (Read w8 (w32 301) packet_chunks) (Concat w56 (Read w8 (w32 300) packet_chunks) (Concat w48 (Read w8 (w32 299) packet_chunks) (Concat w40 (Read w8 (w32 298) packet_chunks) (Concat w32 (Read w8 (w32 295) packet_chunks) (Concat w24 (Read w8 (w32 294) packet_chunks) (ReadLSB w16 (w32 296) packet_chunks)))))))])
                    // Node 10796
                    // BDD node 26:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764675)[(Concat w160 (Read w8 (w32 162) packet_chunks) (Concat w152 (Read w8 (w32 161) packet_chunks) (Concat w144 (Read w8 (w32 160) packet_chunks) (Concat w136 (Read w8 (w32 159) packet_chunks) (Concat w128 (Read w8 (w32 166) packet_chunks) (Concat w120 (Read w8 (w32 165) packet_chunks) (Concat w112 (Read w8 (w32 164) packet_chunks) (Concat w104 (Read w8 (w32 163) packet_chunks) (ReadLSB w96 (w32 147) packet_chunks)))))))))])
                    // Node 10953
                    // BDD node 27:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764528)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    // Node 11114
                    // BDD node 28:FORWARD
                    // Node 9525
                    // BDD node 18:if ((Eq false (Extract 0 (ZExt w8 (Eq false (Eq (w32 0) (And w32 (ZExt w32 (Extract 0 (ZExt w8 (Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) out_of_space_1)))))) (ZExt w32 (Eq (w32 0) (ReadLSB w32 (w32 0) number_of_freed_flows)))))))))
                    // Node 9919
                    // BDD node 33:FORWARD
                  } else {
                    // Node 1951
                    // BDD node 16:if ((Eq (w8 1) (Read w8 (w32 441) packet_chunks))
                    // Node 7537
                    // BDD node 38:FORWARD
                    fwd(1);
                  }
                } else {
                  // Node 279
                  // BDD node 15:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // Node 408
                  // BDD node 39:dchain_rejuvenate_index(chain:(w64 1073952504), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  table_1073952504_39_key_0 = table_1073912440_14_value_0;
                  table_1073952504_39.apply();
                  // Node 545
                  // BDD node 40:if ((Eq false (Eq (w8 1) (Read w8 (w32 441) packet_chunks)))
                  if (8w01 != hdr.hdr_3.data[7:0]) {
                    // Node 546
                    // BDD node 40:if ((Eq false (Eq (w8 1) (Read w8 (w32 441) packet_chunks)))
                    // Node 2766
                    // BDD node 41:if ((Eq false (Eq (w8 0) (Read w8 (w32 441) packet_chunks)))
                    if (8w00 != hdr.hdr_3.data[7:0]) {
                      // Node 2767
                      // BDD node 41:if ((Eq false (Eq (w8 0) (Read w8 (w32 441) packet_chunks)))
                      // Node 3552
                      // BDD node 42:if ((Eq false (Eq (w8 2) (Read w8 (w32 441) packet_chunks)))
                      if (8w02 != hdr.hdr_3.data[7:0]) {
                        // Node 3553
                        // BDD node 42:if ((Eq false (Eq (w8 2) (Read w8 (w32 441) packet_chunks)))
                        // Node 5908
                        // BDD node 44:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764822)[(Concat w64 (Read w8 (w32 301) packet_chunks) (Concat w56 (Read w8 (w32 300) packet_chunks) (Concat w48 (Read w8 (w32 299) packet_chunks) (Concat w40 (Read w8 (w32 298) packet_chunks) (Concat w32 (Read w8 (w32 295) packet_chunks) (Concat w24 (Read w8 (w32 294) packet_chunks) (ReadLSB w16 (w32 296) packet_chunks)))))))])
                        swap(hdr.hdr_2.data[23:16], hdr.hdr_2.data[7:0]);
                        swap(hdr.hdr_2.data[31:24], hdr.hdr_2.data[15:8]);
                        // Node 6907
                        // BDD node 45:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764675)[(Concat w160 (Read w8 (w32 162) packet_chunks) (Concat w152 (Read w8 (w32 161) packet_chunks) (Concat w144 (Read w8 (w32 160) packet_chunks) (Concat w136 (Read w8 (w32 159) packet_chunks) (Concat w128 (Read w8 (w32 166) packet_chunks) (Concat w120 (Read w8 (w32 165) packet_chunks) (Concat w112 (Read w8 (w32 164) packet_chunks) (Concat w104 (Read w8 (w32 163) packet_chunks) (ReadLSB w96 (w32 147) packet_chunks)))))))))])
                        swap(hdr.hdr_1.data[135:128], hdr.hdr_1.data[103:96]);
                        swap(hdr.hdr_1.data[143:136], hdr.hdr_1.data[111:104]);
                        swap(hdr.hdr_1.data[151:144], hdr.hdr_1.data[119:112]);
                        swap(hdr.hdr_1.data[159:152], hdr.hdr_1.data[127:120]);
                        // Node 7861
                        // BDD node 46:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764528)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                        swap(hdr.hdr_0.data[55:48], hdr.hdr_0.data[7:0]);
                        swap(hdr.hdr_0.data[63:56], hdr.hdr_0.data[15:8]);
                        swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                        swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                        swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                        swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                        // Node 8527
                        // BDD node 47:FORWARD
                        fwd(0);
                      } else {
                        // Node 3554
                        // BDD node 42:if ((Eq false (Eq (w8 2) (Read w8 (w32 441) packet_chunks)))
                        // Node 4694
                        // BDD node 48:map_erase(map:(w64 1073912440), key:(w64 1073764970)[(ReadLSB w96 (w32 442) packet_chunks) -> (ReadLSB w96 (w32 442) packet_chunks)], trash:(w64 1074078824)[ -> (w64 12370169555311111083)])
                        hdr.cpu.hit_0 = hit_0;
                        hdr.cpu.table_1073952504_39_key_0 = table_1073952504_39_key_0;
                        send_to_controller(4694);
                        // Node 9920
                        // BDD node 154:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 14), chunk:(w64 1074059888)[ -> (w64 1073764528)])
                        // Node 10057
                        // BDD node 155:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 20), chunk:(w64 1074060656)[ -> (w64 1073764675)])
                        // Node 10198
                        // BDD node 156:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 8), chunk:(w64 1074061344)[ -> (w64 1073764822)])
                        // Node 10343
                        // BDD node 157:packet_borrow_next_chunk(p:(w64 1074049192), length:(w32 142), chunk:(w64 1074062088)[ -> (w64 1073764969)])
                        // Node 10491
                        // BDD node 48:map_erase(map:(w64 1073912440), key:(w64 1073764970)[(ReadLSB w96 (w32 442) packet_chunks) -> (ReadLSB w96 (w32 442) packet_chunks)], trash:(w64 1074078824)[ -> (w64 12370169555311111083)])
                        // Node 10642
                        // BDD node 49:dchain_free_index(chain:(w64 1073952504), index:(ReadLSB w32 (w32 0) allocated_index))
                        // Node 11033
                        // BDD node 51:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764822)[(Concat w64 (Read w8 (w32 301) packet_chunks) (Concat w56 (Read w8 (w32 300) packet_chunks) (Concat w48 (Read w8 (w32 299) packet_chunks) (Concat w40 (Read w8 (w32 298) packet_chunks) (Concat w32 (Read w8 (w32 295) packet_chunks) (Concat w24 (Read w8 (w32 294) packet_chunks) (ReadLSB w16 (w32 296) packet_chunks)))))))])
                        // Node 11196
                        // BDD node 52:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764675)[(Concat w160 (Read w8 (w32 162) packet_chunks) (Concat w152 (Read w8 (w32 161) packet_chunks) (Concat w144 (Read w8 (w32 160) packet_chunks) (Concat w136 (Read w8 (w32 159) packet_chunks) (Concat w128 (Read w8 (w32 166) packet_chunks) (Concat w120 (Read w8 (w32 165) packet_chunks) (Concat w112 (Read w8 (w32 164) packet_chunks) (Concat w104 (Read w8 (w32 163) packet_chunks) (ReadLSB w96 (w32 147) packet_chunks)))))))))])
                        // Node 11279
                        // BDD node 53:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764528)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                        // Node 11363
                        // BDD node 54:FORWARD
                      }
                    } else {
                      // Node 2768
                      // BDD node 41:if ((Eq false (Eq (w8 0) (Read w8 (w32 441) packet_chunks)))
                      // Node 3809
                      // BDD node 55:vector_borrow(vector:(w64 1073938944), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074078760)[ -> (w64 1073951344)])
                      bit<1024> vector_reg_value_0 = 1024w0;
                      vector_reg_value_0[31:0] = vector_reg_1073938944_0_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[63:32] = vector_reg_1073938944_1_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[95:64] = vector_reg_1073938944_2_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[127:96] = vector_reg_1073938944_3_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[159:128] = vector_reg_1073938944_4_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[191:160] = vector_reg_1073938944_5_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[223:192] = vector_reg_1073938944_6_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[255:224] = vector_reg_1073938944_7_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[287:256] = vector_reg_1073938944_8_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[319:288] = vector_reg_1073938944_9_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[351:320] = vector_reg_1073938944_10_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[383:352] = vector_reg_1073938944_11_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[415:384] = vector_reg_1073938944_12_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[447:416] = vector_reg_1073938944_13_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[479:448] = vector_reg_1073938944_14_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[511:480] = vector_reg_1073938944_15_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[543:512] = vector_reg_1073938944_16_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[575:544] = vector_reg_1073938944_17_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[607:576] = vector_reg_1073938944_18_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[639:608] = vector_reg_1073938944_19_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[671:640] = vector_reg_1073938944_20_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[703:672] = vector_reg_1073938944_21_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[735:704] = vector_reg_1073938944_22_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[767:736] = vector_reg_1073938944_23_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[799:768] = vector_reg_1073938944_24_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[831:800] = vector_reg_1073938944_25_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[863:832] = vector_reg_1073938944_26_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[895:864] = vector_reg_1073938944_27_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[927:896] = vector_reg_1073938944_28_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[959:928] = vector_reg_1073938944_29_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[991:960] = vector_reg_1073938944_30_read_3809.execute(table_1073952504_39_key_0);
                      vector_reg_value_0[1023:992] = vector_reg_1073938944_31_read_3809.execute(table_1073952504_39_key_0);
                      // Node 5105
                      // BDD node 56:vector_return(vector:(w64 1073938944), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073951344)[(ReadLSB w1024 (w32 0) vector_data_reset_1)])
                      // Node 6202
                      // BDD node 57:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764969)[(Concat w1136 (Read w8 (w32 582) packet_chunks) (Concat w1128 (Read w8 (w32 127) vector_data_reset_1) (Concat w1120 (Read w8 (w32 126) vector_data_reset_1) (Concat w1112 (Read w8 (w32 125) vector_data_reset_1) (Concat w1104 (Read w8 (w32 124) vector_data_reset_1) (Concat w1096 (Read w8 (w32 123) vector_data_reset_1) (Concat w1088 (Read w8 (w32 122) vector_data_reset_1) (Concat w1080 (Read w8 (w32 121) vector_data_reset_1) (Concat w1072 (Read w8 (w32 120) vector_data_reset_1) (Concat w1064 (Read w8 (w32 119) vector_data_reset_1) (Concat w1056 (Read w8 (w32 118) vector_data_reset_1) (Concat w1048 (Read w8 (w32 117) vector_data_reset_1) (Concat w1040 (Read w8 (w32 116) vector_data_reset_1) (Concat w1032 (Read w8 (w32 115) vector_data_reset_1) (Concat w1024 (Read w8 (w32 114) vector_data_reset_1) (Concat w1016 (Read w8 (w32 113) vector_data_reset_1) (Concat w1008 (Read w8 (w32 112) vector_data_reset_1) (Concat w1000 (Read w8 (w32 111) vector_data_reset_1) (Concat w992 (Read w8 (w32 110) vector_data_reset_1) (Concat w984 (Read w8 (w32 109) vector_data_reset_1) (Concat w976 (Read w8 (w32 108) vector_data_reset_1) (Concat w968 (Read w8 (w32 107) vector_data_reset_1) (Concat w960 (Read w8 (w32 106) vector_data_reset_1) (Concat w952 (Read w8 (w32 105) vector_data_reset_1) (Concat w944 (Read w8 (w32 104) vector_data_reset_1) (Concat w936 (Read w8 (w32 103) vector_data_reset_1) (Concat w928 (Read w8 (w32 102) vector_data_reset_1) (Concat w920 (Read w8 (w32 101) vector_data_reset_1) (Concat w912 (Read w8 (w32 100) vector_data_reset_1) (Concat w904 (Read w8 (w32 99) vector_data_reset_1) (Concat w896 (Read w8 (w32 98) vector_data_reset_1) (Concat w888 (Read w8 (w32 97) vector_data_reset_1) (Concat w880 (Read w8 (w32 96) vector_data_reset_1) (Concat w872 (Read w8 (w32 95) vector_data_reset_1) (Concat w864 (Read w8 (w32 94) vector_data_reset_1) (Concat w856 (Read w8 (w32 93) vector_data_reset_1) (Concat w848 (Read w8 (w32 92) vector_data_reset_1) (Concat w840 (Read w8 (w32 91) vector_data_reset_1) (Concat w832 (Read w8 (w32 90) vector_data_reset_1) (Concat w824 (Read w8 (w32 89) vector_data_reset_1) (Concat w816 (Read w8 (w32 88) vector_data_reset_1) (Concat w808 (Read w8 (w32 87) vector_data_reset_1) (Concat w800 (Read w8 (w32 86) vector_data_reset_1) (Concat w792 (Read w8 (w32 85) vector_data_reset_1) (Concat w784 (Read w8 (w32 84) vector_data_reset_1) (Concat w776 (Read w8 (w32 83) vector_data_reset_1) (Concat w768 (Read w8 (w32 82) vector_data_reset_1) (Concat w760 (Read w8 (w32 81) vector_data_reset_1) (Concat w752 (Read w8 (w32 80) vector_data_reset_1) (Concat w744 (Read w8 (w32 79) vector_data_reset_1) (Concat w736 (Read w8 (w32 78) vector_data_reset_1) (Concat w728 (Read w8 (w32 77) vector_data_reset_1) (Concat w720 (Read w8 (w32 76) vector_data_reset_1) (Concat w712 (Read w8 (w32 75) vector_data_reset_1) (Concat w704 (Read w8 (w32 74) vector_data_reset_1) (Concat w696 (Read w8 (w32 73) vector_data_reset_1) (Concat w688 (Read w8 (w32 72) vector_data_reset_1) (Concat w680 (Read w8 (w32 71) vector_data_reset_1) (Concat w672 (Read w8 (w32 70) vector_data_reset_1) (Concat w664 (Read w8 (w32 69) vector_data_reset_1) (Concat w656 (Read w8 (w32 68) vector_data_reset_1) (Concat w648 (Read w8 (w32 67) vector_data_reset_1) (Concat w640 (Read w8 (w32 66) vector_data_reset_1) (Concat w632 (Read w8 (w32 65) vector_data_reset_1) (Concat w624 (Read w8 (w32 64) vector_data_reset_1) (Concat w616 (Read w8 (w32 63) vector_data_reset_1) (Concat w608 (Read w8 (w32 62) vector_data_reset_1) (Concat w600 (Read w8 (w32 61) vector_data_reset_1) (Concat w592 (Read w8 (w32 60) vector_data_reset_1) (Concat w584 (Read w8 (w32 59) vector_data_reset_1) (Concat w576 (Read w8 (w32 58) vector_data_reset_1) (Concat w568 (Read w8 (w32 57) vector_data_reset_1) (Concat w560 (Read w8 (w32 56) vector_data_reset_1) (Concat w552 (Read w8 (w32 55) vector_data_reset_1) (Concat w544 (Read w8 (w32 54) vector_data_reset_1) (Concat w536 (Read w8 (w32 53) vector_data_reset_1) (Concat w528 (Read w8 (w32 52) vector_data_reset_1) (Concat w520 (Read w8 (w32 51) vector_data_reset_1) (Concat w512 (Read w8 (w32 50) vector_data_reset_1) (Concat w504 (Read w8 (w32 49) vector_data_reset_1) (Concat w496 (Read w8 (w32 48) vector_data_reset_1) (Concat w488 (Read w8 (w32 47) vector_data_reset_1) (Concat w480 (Read w8 (w32 46) vector_data_reset_1) (Concat w472 (Read w8 (w32 45) vector_data_reset_1) (Concat w464 (Read w8 (w32 44) vector_data_reset_1) (Concat w456 (Read w8 (w32 43) vector_data_reset_1) (Concat w448 (Read w8 (w32 42) vector_data_reset_1) (Concat w440 (Read w8 (w32 41) vector_data_reset_1) (Concat w432 (Read w8 (w32 40) vector_data_reset_1) (Concat w424 (Read w8 (w32 39) vector_data_reset_1) (Concat w416 (Read w8 (w32 38) vector_data_reset_1) (Concat w408 (Read w8 (w32 37) vector_data_reset_1) (Concat w400 (Read w8 (w32 36) vector_data_reset_1) (Concat w392 (Read w8 (w32 35) vector_data_reset_1) (Concat w384 (Read w8 (w32 34) vector_data_reset_1) (Concat w376 (Read w8 (w32 33) vector_data_reset_1) (Concat w368 (Read w8 (w32 32) vector_data_reset_1) (Concat w360 (Read w8 (w32 31) vector_data_reset_1) (Concat w352 (Read w8 (w32 30) vector_data_reset_1) (Concat w344 (Read w8 (w32 29) vector_data_reset_1) (Concat w336 (Read w8 (w32 28) vector_data_reset_1) (Concat w328 (Read w8 (w32 27) vector_data_reset_1) (Concat w320 (Read w8 (w32 26) vector_data_reset_1) (Concat w312 (Read w8 (w32 25) vector_data_reset_1) (Concat w304 (Read w8 (w32 24) vector_data_reset_1) (Concat w296 (Read w8 (w32 23) vector_data_reset_1) (Concat w288 (Read w8 (w32 22) vector_data_reset_1) (Concat w280 (Read w8 (w32 21) vector_data_reset_1) (Concat w272 (Read w8 (w32 20) vector_data_reset_1) (Concat w264 (Read w8 (w32 19) vector_data_reset_1) (Concat w256 (Read w8 (w32 18) vector_data_reset_1) (Concat w248 (Read w8 (w32 17) vector_data_reset_1) (Concat w240 (Read w8 (w32 16) vector_data_reset_1) (Concat w232 (Read w8 (w32 15) vector_data_reset_1) (Concat w224 (Read w8 (w32 14) vector_data_reset_1) (Concat w216 (Read w8 (w32 13) vector_data_reset_1) (Concat w208 (Read w8 (w32 12) vector_data_reset_1) (Concat w200 (Read w8 (w32 11) vector_data_reset_1) (Concat w192 (Read w8 (w32 10) vector_data_reset_1) (Concat w184 (Read w8 (w32 9) vector_data_reset_1) (Concat w176 (Read w8 (w32 8) vector_data_reset_1) (Concat w168 (Read w8 (w32 7) vector_data_reset_1) (Concat w160 (Read w8 (w32 6) vector_data_reset_1) (Concat w152 (Read w8 (w32 5) vector_data_reset_1) (Concat w144 (Read w8 (w32 4) vector_data_reset_1) (Concat w136 (Read w8 (w32 3) vector_data_reset_1) (Concat w128 (Read w8 (w32 2) vector_data_reset_1) (Concat w120 (Read w8 (w32 1) vector_data_reset_1) (Concat w112 (Read w8 (w32 0) vector_data_reset_1) (ReadLSB w104 (w32 441) packet_chunks))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))])
                      hdr.hdr_3.data[111:104] = vector_reg_value_0[7:0];
                      hdr.hdr_3.data[119:112] = vector_reg_value_0[15:8];
                      hdr.hdr_3.data[127:120] = vector_reg_value_0[23:16];
                      hdr.hdr_3.data[135:128] = vector_reg_value_0[31:24];
                      hdr.hdr_3.data[143:136] = vector_reg_value_0[39:32];
                      hdr.hdr_3.data[151:144] = vector_reg_value_0[47:40];
                      hdr.hdr_3.data[159:152] = vector_reg_value_0[55:48];
                      hdr.hdr_3.data[167:160] = vector_reg_value_0[63:56];
                      hdr.hdr_3.data[175:168] = vector_reg_value_0[71:64];
                      hdr.hdr_3.data[183:176] = vector_reg_value_0[79:72];
                      hdr.hdr_3.data[191:184] = vector_reg_value_0[87:80];
                      hdr.hdr_3.data[199:192] = vector_reg_value_0[95:88];
                      hdr.hdr_3.data[207:200] = vector_reg_value_0[103:96];
                      hdr.hdr_3.data[215:208] = vector_reg_value_0[111:104];
                      hdr.hdr_3.data[223:216] = vector_reg_value_0[119:112];
                      hdr.hdr_3.data[231:224] = vector_reg_value_0[127:120];
                      hdr.hdr_3.data[239:232] = vector_reg_value_0[135:128];
                      hdr.hdr_3.data[247:240] = vector_reg_value_0[143:136];
                      hdr.hdr_3.data[255:248] = vector_reg_value_0[151:144];
                      hdr.hdr_3.data[263:256] = vector_reg_value_0[159:152];
                      hdr.hdr_3.data[271:264] = vector_reg_value_0[167:160];
                      hdr.hdr_3.data[279:272] = vector_reg_value_0[175:168];
                      hdr.hdr_3.data[287:280] = vector_reg_value_0[183:176];
                      hdr.hdr_3.data[295:288] = vector_reg_value_0[191:184];
                      hdr.hdr_3.data[303:296] = vector_reg_value_0[199:192];
                      hdr.hdr_3.data[311:304] = vector_reg_value_0[207:200];
                      hdr.hdr_3.data[319:312] = vector_reg_value_0[215:208];
                      hdr.hdr_3.data[327:320] = vector_reg_value_0[223:216];
                      hdr.hdr_3.data[335:328] = vector_reg_value_0[231:224];
                      hdr.hdr_3.data[343:336] = vector_reg_value_0[239:232];
                      hdr.hdr_3.data[351:344] = vector_reg_value_0[247:240];
                      hdr.hdr_3.data[359:352] = vector_reg_value_0[255:248];
                      hdr.hdr_3.data[367:360] = vector_reg_value_0[263:256];
                      hdr.hdr_3.data[375:368] = vector_reg_value_0[271:264];
                      hdr.hdr_3.data[383:376] = vector_reg_value_0[279:272];
                      hdr.hdr_3.data[391:384] = vector_reg_value_0[287:280];
                      hdr.hdr_3.data[399:392] = vector_reg_value_0[295:288];
                      hdr.hdr_3.data[407:400] = vector_reg_value_0[303:296];
                      hdr.hdr_3.data[415:408] = vector_reg_value_0[311:304];
                      hdr.hdr_3.data[423:416] = vector_reg_value_0[319:312];
                      hdr.hdr_3.data[431:424] = vector_reg_value_0[327:320];
                      hdr.hdr_3.data[439:432] = vector_reg_value_0[335:328];
                      hdr.hdr_3.data[447:440] = vector_reg_value_0[343:336];
                      hdr.hdr_3.data[455:448] = vector_reg_value_0[351:344];
                      hdr.hdr_3.data[463:456] = vector_reg_value_0[359:352];
                      hdr.hdr_3.data[471:464] = vector_reg_value_0[367:360];
                      hdr.hdr_3.data[479:472] = vector_reg_value_0[375:368];
                      hdr.hdr_3.data[487:480] = vector_reg_value_0[383:376];
                      hdr.hdr_3.data[495:488] = vector_reg_value_0[391:384];
                      hdr.hdr_3.data[503:496] = vector_reg_value_0[399:392];
                      hdr.hdr_3.data[511:504] = vector_reg_value_0[407:400];
                      hdr.hdr_3.data[519:512] = vector_reg_value_0[415:408];
                      hdr.hdr_3.data[527:520] = vector_reg_value_0[423:416];
                      hdr.hdr_3.data[535:528] = vector_reg_value_0[431:424];
                      hdr.hdr_3.data[543:536] = vector_reg_value_0[439:432];
                      hdr.hdr_3.data[551:544] = vector_reg_value_0[447:440];
                      hdr.hdr_3.data[559:552] = vector_reg_value_0[455:448];
                      hdr.hdr_3.data[567:560] = vector_reg_value_0[463:456];
                      hdr.hdr_3.data[575:568] = vector_reg_value_0[471:464];
                      hdr.hdr_3.data[583:576] = vector_reg_value_0[479:472];
                      hdr.hdr_3.data[591:584] = vector_reg_value_0[487:480];
                      hdr.hdr_3.data[599:592] = vector_reg_value_0[495:488];
                      hdr.hdr_3.data[607:600] = vector_reg_value_0[503:496];
                      hdr.hdr_3.data[615:608] = vector_reg_value_0[511:504];
                      hdr.hdr_3.data[623:616] = vector_reg_value_0[519:512];
                      hdr.hdr_3.data[631:624] = vector_reg_value_0[527:520];
                      hdr.hdr_3.data[639:632] = vector_reg_value_0[535:528];
                      hdr.hdr_3.data[647:640] = vector_reg_value_0[543:536];
                      hdr.hdr_3.data[655:648] = vector_reg_value_0[551:544];
                      hdr.hdr_3.data[663:656] = vector_reg_value_0[559:552];
                      hdr.hdr_3.data[671:664] = vector_reg_value_0[567:560];
                      hdr.hdr_3.data[679:672] = vector_reg_value_0[575:568];
                      hdr.hdr_3.data[687:680] = vector_reg_value_0[583:576];
                      hdr.hdr_3.data[695:688] = vector_reg_value_0[591:584];
                      hdr.hdr_3.data[703:696] = vector_reg_value_0[599:592];
                      hdr.hdr_3.data[711:704] = vector_reg_value_0[607:600];
                      hdr.hdr_3.data[719:712] = vector_reg_value_0[615:608];
                      hdr.hdr_3.data[727:720] = vector_reg_value_0[623:616];
                      hdr.hdr_3.data[735:728] = vector_reg_value_0[631:624];
                      hdr.hdr_3.data[743:736] = vector_reg_value_0[639:632];
                      hdr.hdr_3.data[751:744] = vector_reg_value_0[647:640];
                      hdr.hdr_3.data[759:752] = vector_reg_value_0[655:648];
                      hdr.hdr_3.data[767:760] = vector_reg_value_0[663:656];
                      hdr.hdr_3.data[775:768] = vector_reg_value_0[671:664];
                      hdr.hdr_3.data[783:776] = vector_reg_value_0[679:672];
                      hdr.hdr_3.data[791:784] = vector_reg_value_0[687:680];
                      hdr.hdr_3.data[799:792] = vector_reg_value_0[695:688];
                      hdr.hdr_3.data[807:800] = vector_reg_value_0[703:696];
                      hdr.hdr_3.data[815:808] = vector_reg_value_0[711:704];
                      hdr.hdr_3.data[823:816] = vector_reg_value_0[719:712];
                      hdr.hdr_3.data[831:824] = vector_reg_value_0[727:720];
                      hdr.hdr_3.data[839:832] = vector_reg_value_0[735:728];
                      hdr.hdr_3.data[847:840] = vector_reg_value_0[743:736];
                      hdr.hdr_3.data[855:848] = vector_reg_value_0[751:744];
                      hdr.hdr_3.data[863:856] = vector_reg_value_0[759:752];
                      hdr.hdr_3.data[871:864] = vector_reg_value_0[767:760];
                      hdr.hdr_3.data[879:872] = vector_reg_value_0[775:768];
                      hdr.hdr_3.data[887:880] = vector_reg_value_0[783:776];
                      hdr.hdr_3.data[895:888] = vector_reg_value_0[791:784];
                      hdr.hdr_3.data[903:896] = vector_reg_value_0[799:792];
                      hdr.hdr_3.data[911:904] = vector_reg_value_0[807:800];
                      hdr.hdr_3.data[919:912] = vector_reg_value_0[815:808];
                      hdr.hdr_3.data[927:920] = vector_reg_value_0[823:816];
                      hdr.hdr_3.data[935:928] = vector_reg_value_0[831:824];
                      hdr.hdr_3.data[943:936] = vector_reg_value_0[839:832];
                      hdr.hdr_3.data[951:944] = vector_reg_value_0[847:840];
                      hdr.hdr_3.data[959:952] = vector_reg_value_0[855:848];
                      hdr.hdr_3.data[967:960] = vector_reg_value_0[863:856];
                      hdr.hdr_3.data[975:968] = vector_reg_value_0[871:864];
                      hdr.hdr_3.data[983:976] = vector_reg_value_0[879:872];
                      hdr.hdr_3.data[991:984] = vector_reg_value_0[887:880];
                      hdr.hdr_3.data[999:992] = vector_reg_value_0[895:888];
                      hdr.hdr_3.data[1007:1000] = vector_reg_value_0[903:896];
                      hdr.hdr_3.data[1015:1008] = vector_reg_value_0[911:904];
                      hdr.hdr_3.data[1023:1016] = vector_reg_value_0[919:912];
                      hdr.hdr_3.data[1031:1024] = vector_reg_value_0[927:920];
                      hdr.hdr_3.data[1039:1032] = vector_reg_value_0[935:928];
                      hdr.hdr_3.data[1047:1040] = vector_reg_value_0[943:936];
                      hdr.hdr_3.data[1055:1048] = vector_reg_value_0[951:944];
                      hdr.hdr_3.data[1063:1056] = vector_reg_value_0[959:952];
                      hdr.hdr_3.data[1071:1064] = vector_reg_value_0[967:960];
                      hdr.hdr_3.data[1079:1072] = vector_reg_value_0[975:968];
                      hdr.hdr_3.data[1087:1080] = vector_reg_value_0[983:976];
                      hdr.hdr_3.data[1095:1088] = vector_reg_value_0[991:984];
                      hdr.hdr_3.data[1103:1096] = vector_reg_value_0[999:992];
                      hdr.hdr_3.data[1111:1104] = vector_reg_value_0[1007:1000];
                      hdr.hdr_3.data[1119:1112] = vector_reg_value_0[1015:1008];
                      hdr.hdr_3.data[1127:1120] = vector_reg_value_0[1023:1016];
                      // Node 7219
                      // BDD node 58:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764822)[(Concat w64 (Read w8 (w32 301) packet_chunks) (Concat w56 (Read w8 (w32 300) packet_chunks) (Concat w48 (Read w8 (w32 299) packet_chunks) (Concat w40 (Read w8 (w32 298) packet_chunks) (Concat w32 (Read w8 (w32 295) packet_chunks) (Concat w24 (Read w8 (w32 294) packet_chunks) (ReadLSB w16 (w32 296) packet_chunks)))))))])
                      swap(hdr.hdr_2.data[23:16], hdr.hdr_2.data[7:0]);
                      swap(hdr.hdr_2.data[31:24], hdr.hdr_2.data[15:8]);
                      // Node 8191
                      // BDD node 59:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764675)[(Concat w160 (Read w8 (w32 162) packet_chunks) (Concat w152 (Read w8 (w32 161) packet_chunks) (Concat w144 (Read w8 (w32 160) packet_chunks) (Concat w136 (Read w8 (w32 159) packet_chunks) (Concat w128 (Read w8 (w32 166) packet_chunks) (Concat w120 (Read w8 (w32 165) packet_chunks) (Concat w112 (Read w8 (w32 164) packet_chunks) (Concat w104 (Read w8 (w32 163) packet_chunks) (ReadLSB w96 (w32 147) packet_chunks)))))))))])
                      swap(hdr.hdr_1.data[135:128], hdr.hdr_1.data[103:96]);
                      swap(hdr.hdr_1.data[143:136], hdr.hdr_1.data[111:104]);
                      swap(hdr.hdr_1.data[151:144], hdr.hdr_1.data[119:112]);
                      swap(hdr.hdr_1.data[159:152], hdr.hdr_1.data[127:120]);
                      // Node 8869
                      // BDD node 60:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764528)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr_0.data[55:48], hdr.hdr_0.data[7:0]);
                      swap(hdr.hdr_0.data[63:56], hdr.hdr_0.data[15:8]);
                      swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                      swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                      swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                      swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                      // Node 9217
                      // BDD node 61:FORWARD
                      fwd(0);
                    }
                  } else {
                    // Node 547
                    // BDD node 40:if ((Eq false (Eq (w8 1) (Read w8 (w32 441) packet_chunks)))
                    // Node 700
                    // BDD node 62:vector_borrow(vector:(w64 1073938944), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074078792)[ -> (w64 1073951344)])
                    // Node 887
                    // BDD node 63:vector_return(vector:(w64 1073938944), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073951344)[(ReadLSB w1024 (w32 454) packet_chunks)])
                    vector_reg_1073938944_0_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_1_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_2_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_3_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_4_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_5_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_6_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_7_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_8_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_9_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_10_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_11_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_12_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_13_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_14_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_15_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_16_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_17_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_18_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_19_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_20_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_21_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_22_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_23_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_24_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_25_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_26_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_27_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_28_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_29_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_30_write_887.execute(table_1073952504_39_key_0);
                    vector_reg_1073938944_31_write_887.execute(table_1073952504_39_key_0);
                    // Node 1248
                    // BDD node 65:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764822)[(Concat w64 (Read w8 (w32 301) packet_chunks) (Concat w56 (Read w8 (w32 300) packet_chunks) (Concat w48 (Read w8 (w32 299) packet_chunks) (Concat w40 (Read w8 (w32 298) packet_chunks) (Concat w32 (Read w8 (w32 295) packet_chunks) (Concat w24 (Read w8 (w32 294) packet_chunks) (ReadLSB w16 (w32 296) packet_chunks)))))))])
                    swap(hdr.hdr_2.data[23:16], hdr.hdr_2.data[7:0]);
                    swap(hdr.hdr_2.data[31:24], hdr.hdr_2.data[15:8]);
                    // Node 1422
                    // BDD node 66:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764675)[(Concat w160 (Read w8 (w32 162) packet_chunks) (Concat w152 (Read w8 (w32 161) packet_chunks) (Concat w144 (Read w8 (w32 160) packet_chunks) (Concat w136 (Read w8 (w32 159) packet_chunks) (Concat w128 (Read w8 (w32 166) packet_chunks) (Concat w120 (Read w8 (w32 165) packet_chunks) (Concat w112 (Read w8 (w32 164) packet_chunks) (Concat w104 (Read w8 (w32 163) packet_chunks) (ReadLSB w96 (w32 147) packet_chunks)))))))))])
                    swap(hdr.hdr_1.data[135:128], hdr.hdr_1.data[103:96]);
                    swap(hdr.hdr_1.data[143:136], hdr.hdr_1.data[111:104]);
                    swap(hdr.hdr_1.data[151:144], hdr.hdr_1.data[119:112]);
                    swap(hdr.hdr_1.data[159:152], hdr.hdr_1.data[127:120]);
                    // Node 1602
                    // BDD node 67:packet_return_chunk(p:(w64 1074065408), the_chunk:(w64 1073764528)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr_0.data[55:48], hdr.hdr_0.data[7:0]);
                    swap(hdr.hdr_0.data[63:56], hdr.hdr_0.data[15:8]);
                    swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                    swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                    swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                    swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                    // Node 1788
                    // BDD node 68:FORWARD
                    fwd(0);
                  }
                }
              }
            }
            // Node 85
            // BDD node 6:if ((And (Eq (w16 40450) (ReadLSB w16 (w32 296) packet_chunks)) (Ule (w64 142) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // Node 5525
            // BDD node 72:DROP
          }
          // Node 41
          // BDD node 4:if ((And (Eq (w8 17) (Read w8 (w32 156) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // Node 4202
          // BDD node 75:DROP
        }
        // Node 13
        // BDD node 2:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // Node 3114
        // BDD node 77:DROP
      }

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
