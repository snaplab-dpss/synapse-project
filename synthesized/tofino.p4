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
      default: parser_init;
    }
  }

  state parse_cpu {
    pkt.extract(hdr.cpu);
    transition accept;
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
    transition parser_63;
  }
  state parser_72 {
    transition reject;
  }
  state parser_63 {
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

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_0) vector_reg_1073938944_0_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_1) vector_reg_1073938944_1_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_2) vector_reg_1073938944_2_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_3) vector_reg_1073938944_3_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_4) vector_reg_1073938944_4_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_5) vector_reg_1073938944_5_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_6) vector_reg_1073938944_6_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_7) vector_reg_1073938944_7_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_8) vector_reg_1073938944_8_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_9) vector_reg_1073938944_9_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_10) vector_reg_1073938944_10_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_11) vector_reg_1073938944_11_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_12) vector_reg_1073938944_12_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_13) vector_reg_1073938944_13_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_14) vector_reg_1073938944_14_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_15) vector_reg_1073938944_15_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_16) vector_reg_1073938944_16_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_17) vector_reg_1073938944_17_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_18) vector_reg_1073938944_18_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_19) vector_reg_1073938944_19_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_20) vector_reg_1073938944_20_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_21) vector_reg_1073938944_21_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_22) vector_reg_1073938944_22_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_23) vector_reg_1073938944_23_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_24) vector_reg_1073938944_24_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_25) vector_reg_1073938944_25_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_26) vector_reg_1073938944_26_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_27) vector_reg_1073938944_27_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_28) vector_reg_1073938944_28_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_29) vector_reg_1073938944_29_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_30) vector_reg_1073938944_30_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_reg_1073938944_31) vector_reg_1073938944_31_read_267 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> table_1073952504_39_key_0 = 32w0;
  table table_1073952504_39 {
    key = {
      table_1073952504_39_key_0: exact;
    }
    actions = {}
    size = 8192;
  }


  apply {
    // Node 1
    // Node 4
    if(hdr.hdr_0.isValid()) {
      // Node 14
      // Node 15
      // Node 23
      if(hdr.hdr_1.isValid()) {
        // Node 42
        // Node 43
        // Node 55
        if(hdr.hdr_2.isValid()) {
          // Node 86
          // Node 87
          // Node 116
          if(hdr.hdr_3.isValid()) {
            // Node 159
            if (16w0000 != meta.port) {
              // Node 160
              // Node 2588
              fwd(0);
            } else {
              // Node 161
              // Node 194
              table_1073912440_14_key_0 = hdr.hdr_3.data[103:8];
              bool hit_0 = table_1073912440_14.apply().hit;
              // Node 267
              bit<32> vector_reg_entry_0_0 = vector_reg_1073938944_0_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_1_0 = vector_reg_1073938944_1_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_2_0 = vector_reg_1073938944_2_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_3_0 = vector_reg_1073938944_3_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_4_0 = vector_reg_1073938944_4_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_5_0 = vector_reg_1073938944_5_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_6_0 = vector_reg_1073938944_6_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_7_0 = vector_reg_1073938944_7_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_8_0 = vector_reg_1073938944_8_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_9_0 = vector_reg_1073938944_9_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_10_0 = vector_reg_1073938944_10_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_11_0 = vector_reg_1073938944_11_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_12_0 = vector_reg_1073938944_12_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_13_0 = vector_reg_1073938944_13_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_14_0 = vector_reg_1073938944_14_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_15_0 = vector_reg_1073938944_15_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_16_0 = vector_reg_1073938944_16_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_17_0 = vector_reg_1073938944_17_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_18_0 = vector_reg_1073938944_18_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_19_0 = vector_reg_1073938944_19_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_20_0 = vector_reg_1073938944_20_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_21_0 = vector_reg_1073938944_21_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_22_0 = vector_reg_1073938944_22_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_23_0 = vector_reg_1073938944_23_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_24_0 = vector_reg_1073938944_24_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_25_0 = vector_reg_1073938944_25_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_26_0 = vector_reg_1073938944_26_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_27_0 = vector_reg_1073938944_27_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_28_0 = vector_reg_1073938944_28_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_29_0 = vector_reg_1073938944_29_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_30_0 = vector_reg_1073938944_30_read_267.execute(table_1073912440_14_value_0);
              bit<32> vector_reg_entry_31_0 = vector_reg_1073938944_31_read_267.execute(table_1073912440_14_value_0);
              // Node 306
              if (!hit_0) {
                // Node 307
                // Node 479
                if (8w01 == hdr.hdr_3.data[7:0]) {
                  // Node 480
                  // Node 539
                  hdr.cpu.hit_0 = hit_0;
                  hdr.cpu.table_1073912440_14_value_0 = table_1073912440_14_value_0;
                  send_to_controller(539);
                  // Node 4037
                  // Node 4163
                  // Node 4291
                  // Node 4421
                  // Node 4487
                  // Node 4621
                  // Node 4689
                  // Node 4690
                  // Node 5406
                  // Node 5704
                  // Node 6015
                  // Node 6176
                  // Node 6341
                  // Node 6508
                  // Node 4691
                  // Node 5115
                } else {
                  // Node 481
                  // Node 2876
                  fwd(1);
                }
              } else {
                // Node 308
                // Node 351
                table_1073952504_39_key_0 = table_1073912440_14_value_0;
                table_1073952504_39.apply();
                // Node 398
                if (8w01 != hdr.hdr_3.data[7:0]) {
                  // Node 399
                  // Node 824
                  if (8w00 != hdr.hdr_3.data[7:0]) {
                    // Node 825
                    // Node 1246
                    if (8w02 != hdr.hdr_3.data[7:0]) {
                      // Node 1247
                      // Node 2315
                      swap(hdr.hdr_2.data[23:16], hdr.hdr_2.data[7:0]);
                      swap(hdr.hdr_2.data[31:24], hdr.hdr_2.data[15:8]);
                      // Node 2682
                      swap(hdr.hdr_1.data[135:128], hdr.hdr_1.data[103:96]);
                      swap(hdr.hdr_1.data[143:136], hdr.hdr_1.data[111:104]);
                      swap(hdr.hdr_1.data[151:144], hdr.hdr_1.data[119:112]);
                      swap(hdr.hdr_1.data[159:152], hdr.hdr_1.data[127:120]);
                      // Node 2976
                      swap(hdr.hdr_0.data[55:48], hdr.hdr_0.data[7:0]);
                      swap(hdr.hdr_0.data[63:56], hdr.hdr_0.data[15:8]);
                      swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                      swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                      swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                      swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                      // Node 3182
                      fwd(0);
                    } else {
                      // Node 1248
                      // Node 1885
                      hdr.cpu.hit_0 = hit_0;
                      hdr.cpu.table_1073952504_39_key_0 = table_1073952504_39_key_0;
                      send_to_controller(1885);
                      // Node 5260
                      // Node 5554
                      // Node 5705
                      // Node 5859
                      // Node 6016
                      // Node 6177
                      // Node 6593
                      // Node 6679
                      // Node 6766
                      // Node 6854
                    }
                  } else {
                    // Node 826
                    // Node 1323
                    // Node 1969
                    hdr.hdr_3.data[111:104] = vector_reg_entry_0_0[7:0];
                    hdr.hdr_3.data[119:112] = vector_reg_entry_0_0[15:8];
                    hdr.hdr_3.data[127:120] = vector_reg_entry_0_0[23:16];
                    hdr.hdr_3.data[135:128] = vector_reg_entry_0_0[31:24];
                    hdr.hdr_3.data[143:136] = vector_reg_entry_1_0[7:0];
                    hdr.hdr_3.data[151:144] = vector_reg_entry_1_0[15:8];
                    hdr.hdr_3.data[159:152] = vector_reg_entry_1_0[23:16];
                    hdr.hdr_3.data[167:160] = vector_reg_entry_1_0[31:24];
                    hdr.hdr_3.data[175:168] = vector_reg_entry_2_0[7:0];
                    hdr.hdr_3.data[183:176] = vector_reg_entry_2_0[15:8];
                    hdr.hdr_3.data[191:184] = vector_reg_entry_2_0[23:16];
                    hdr.hdr_3.data[199:192] = vector_reg_entry_2_0[31:24];
                    hdr.hdr_3.data[207:200] = vector_reg_entry_3_0[7:0];
                    hdr.hdr_3.data[215:208] = vector_reg_entry_3_0[15:8];
                    hdr.hdr_3.data[223:216] = vector_reg_entry_3_0[23:16];
                    hdr.hdr_3.data[231:224] = vector_reg_entry_3_0[31:24];
                    hdr.hdr_3.data[239:232] = vector_reg_entry_4_0[7:0];
                    hdr.hdr_3.data[247:240] = vector_reg_entry_4_0[15:8];
                    hdr.hdr_3.data[255:248] = vector_reg_entry_4_0[23:16];
                    hdr.hdr_3.data[263:256] = vector_reg_entry_4_0[31:24];
                    hdr.hdr_3.data[271:264] = vector_reg_entry_5_0[7:0];
                    hdr.hdr_3.data[279:272] = vector_reg_entry_5_0[15:8];
                    hdr.hdr_3.data[287:280] = vector_reg_entry_5_0[23:16];
                    hdr.hdr_3.data[295:288] = vector_reg_entry_5_0[31:24];
                    hdr.hdr_3.data[303:296] = vector_reg_entry_6_0[7:0];
                    hdr.hdr_3.data[311:304] = vector_reg_entry_6_0[15:8];
                    hdr.hdr_3.data[319:312] = vector_reg_entry_6_0[23:16];
                    hdr.hdr_3.data[327:320] = vector_reg_entry_6_0[31:24];
                    hdr.hdr_3.data[335:328] = vector_reg_entry_7_0[7:0];
                    hdr.hdr_3.data[343:336] = vector_reg_entry_7_0[15:8];
                    hdr.hdr_3.data[351:344] = vector_reg_entry_7_0[23:16];
                    hdr.hdr_3.data[359:352] = vector_reg_entry_7_0[31:24];
                    hdr.hdr_3.data[367:360] = vector_reg_entry_8_0[7:0];
                    hdr.hdr_3.data[375:368] = vector_reg_entry_8_0[15:8];
                    hdr.hdr_3.data[383:376] = vector_reg_entry_8_0[23:16];
                    hdr.hdr_3.data[391:384] = vector_reg_entry_8_0[31:24];
                    hdr.hdr_3.data[399:392] = vector_reg_entry_9_0[7:0];
                    hdr.hdr_3.data[407:400] = vector_reg_entry_9_0[15:8];
                    hdr.hdr_3.data[415:408] = vector_reg_entry_9_0[23:16];
                    hdr.hdr_3.data[423:416] = vector_reg_entry_9_0[31:24];
                    hdr.hdr_3.data[431:424] = vector_reg_entry_10_0[7:0];
                    hdr.hdr_3.data[439:432] = vector_reg_entry_10_0[15:8];
                    hdr.hdr_3.data[447:440] = vector_reg_entry_10_0[23:16];
                    hdr.hdr_3.data[455:448] = vector_reg_entry_10_0[31:24];
                    hdr.hdr_3.data[463:456] = vector_reg_entry_11_0[7:0];
                    hdr.hdr_3.data[471:464] = vector_reg_entry_11_0[15:8];
                    hdr.hdr_3.data[479:472] = vector_reg_entry_11_0[23:16];
                    hdr.hdr_3.data[487:480] = vector_reg_entry_11_0[31:24];
                    hdr.hdr_3.data[495:488] = vector_reg_entry_12_0[7:0];
                    hdr.hdr_3.data[503:496] = vector_reg_entry_12_0[15:8];
                    hdr.hdr_3.data[511:504] = vector_reg_entry_12_0[23:16];
                    hdr.hdr_3.data[519:512] = vector_reg_entry_12_0[31:24];
                    hdr.hdr_3.data[527:520] = vector_reg_entry_13_0[7:0];
                    hdr.hdr_3.data[535:528] = vector_reg_entry_13_0[15:8];
                    hdr.hdr_3.data[543:536] = vector_reg_entry_13_0[23:16];
                    hdr.hdr_3.data[551:544] = vector_reg_entry_13_0[31:24];
                    hdr.hdr_3.data[559:552] = vector_reg_entry_14_0[7:0];
                    hdr.hdr_3.data[567:560] = vector_reg_entry_14_0[15:8];
                    hdr.hdr_3.data[575:568] = vector_reg_entry_14_0[23:16];
                    hdr.hdr_3.data[583:576] = vector_reg_entry_14_0[31:24];
                    hdr.hdr_3.data[591:584] = vector_reg_entry_15_0[7:0];
                    hdr.hdr_3.data[599:592] = vector_reg_entry_15_0[15:8];
                    hdr.hdr_3.data[607:600] = vector_reg_entry_15_0[23:16];
                    hdr.hdr_3.data[615:608] = vector_reg_entry_15_0[31:24];
                    hdr.hdr_3.data[623:616] = vector_reg_entry_16_0[7:0];
                    hdr.hdr_3.data[631:624] = vector_reg_entry_16_0[15:8];
                    hdr.hdr_3.data[639:632] = vector_reg_entry_16_0[23:16];
                    hdr.hdr_3.data[647:640] = vector_reg_entry_16_0[31:24];
                    hdr.hdr_3.data[655:648] = vector_reg_entry_17_0[7:0];
                    hdr.hdr_3.data[663:656] = vector_reg_entry_17_0[15:8];
                    hdr.hdr_3.data[671:664] = vector_reg_entry_17_0[23:16];
                    hdr.hdr_3.data[679:672] = vector_reg_entry_17_0[31:24];
                    hdr.hdr_3.data[687:680] = vector_reg_entry_18_0[7:0];
                    hdr.hdr_3.data[695:688] = vector_reg_entry_18_0[15:8];
                    hdr.hdr_3.data[703:696] = vector_reg_entry_18_0[23:16];
                    hdr.hdr_3.data[711:704] = vector_reg_entry_18_0[31:24];
                    hdr.hdr_3.data[719:712] = vector_reg_entry_19_0[7:0];
                    hdr.hdr_3.data[727:720] = vector_reg_entry_19_0[15:8];
                    hdr.hdr_3.data[735:728] = vector_reg_entry_19_0[23:16];
                    hdr.hdr_3.data[743:736] = vector_reg_entry_19_0[31:24];
                    hdr.hdr_3.data[751:744] = vector_reg_entry_20_0[7:0];
                    hdr.hdr_3.data[759:752] = vector_reg_entry_20_0[15:8];
                    hdr.hdr_3.data[767:760] = vector_reg_entry_20_0[23:16];
                    hdr.hdr_3.data[775:768] = vector_reg_entry_20_0[31:24];
                    hdr.hdr_3.data[783:776] = vector_reg_entry_21_0[7:0];
                    hdr.hdr_3.data[791:784] = vector_reg_entry_21_0[15:8];
                    hdr.hdr_3.data[799:792] = vector_reg_entry_21_0[23:16];
                    hdr.hdr_3.data[807:800] = vector_reg_entry_21_0[31:24];
                    hdr.hdr_3.data[815:808] = vector_reg_entry_22_0[7:0];
                    hdr.hdr_3.data[823:816] = vector_reg_entry_22_0[15:8];
                    hdr.hdr_3.data[831:824] = vector_reg_entry_22_0[23:16];
                    hdr.hdr_3.data[839:832] = vector_reg_entry_22_0[31:24];
                    hdr.hdr_3.data[847:840] = vector_reg_entry_23_0[7:0];
                    hdr.hdr_3.data[855:848] = vector_reg_entry_23_0[15:8];
                    hdr.hdr_3.data[863:856] = vector_reg_entry_23_0[23:16];
                    hdr.hdr_3.data[871:864] = vector_reg_entry_23_0[31:24];
                    hdr.hdr_3.data[879:872] = vector_reg_entry_24_0[7:0];
                    hdr.hdr_3.data[887:880] = vector_reg_entry_24_0[15:8];
                    hdr.hdr_3.data[895:888] = vector_reg_entry_24_0[23:16];
                    hdr.hdr_3.data[903:896] = vector_reg_entry_24_0[31:24];
                    hdr.hdr_3.data[911:904] = vector_reg_entry_25_0[7:0];
                    hdr.hdr_3.data[919:912] = vector_reg_entry_25_0[15:8];
                    hdr.hdr_3.data[927:920] = vector_reg_entry_25_0[23:16];
                    hdr.hdr_3.data[935:928] = vector_reg_entry_25_0[31:24];
                    hdr.hdr_3.data[943:936] = vector_reg_entry_26_0[7:0];
                    hdr.hdr_3.data[951:944] = vector_reg_entry_26_0[15:8];
                    hdr.hdr_3.data[959:952] = vector_reg_entry_26_0[23:16];
                    hdr.hdr_3.data[967:960] = vector_reg_entry_26_0[31:24];
                    hdr.hdr_3.data[975:968] = vector_reg_entry_27_0[7:0];
                    hdr.hdr_3.data[983:976] = vector_reg_entry_27_0[15:8];
                    hdr.hdr_3.data[991:984] = vector_reg_entry_27_0[23:16];
                    hdr.hdr_3.data[999:992] = vector_reg_entry_27_0[31:24];
                    hdr.hdr_3.data[1007:1000] = vector_reg_entry_28_0[7:0];
                    hdr.hdr_3.data[1015:1008] = vector_reg_entry_28_0[15:8];
                    hdr.hdr_3.data[1023:1016] = vector_reg_entry_28_0[23:16];
                    hdr.hdr_3.data[1031:1024] = vector_reg_entry_28_0[31:24];
                    hdr.hdr_3.data[1039:1032] = vector_reg_entry_29_0[7:0];
                    hdr.hdr_3.data[1047:1040] = vector_reg_entry_29_0[15:8];
                    hdr.hdr_3.data[1055:1048] = vector_reg_entry_29_0[23:16];
                    hdr.hdr_3.data[1063:1056] = vector_reg_entry_29_0[31:24];
                    hdr.hdr_3.data[1071:1064] = vector_reg_entry_30_0[7:0];
                    hdr.hdr_3.data[1079:1072] = vector_reg_entry_30_0[15:8];
                    hdr.hdr_3.data[1087:1080] = vector_reg_entry_30_0[23:16];
                    hdr.hdr_3.data[1095:1088] = vector_reg_entry_30_0[31:24];
                    hdr.hdr_3.data[1103:1096] = vector_reg_entry_31_0[7:0];
                    hdr.hdr_3.data[1111:1104] = vector_reg_entry_31_0[15:8];
                    hdr.hdr_3.data[1119:1112] = vector_reg_entry_31_0[23:16];
                    hdr.hdr_3.data[1127:1120] = vector_reg_entry_31_0[31:24];
                    // Node 2405
                    swap(hdr.hdr_2.data[23:16], hdr.hdr_2.data[7:0]);
                    swap(hdr.hdr_2.data[31:24], hdr.hdr_2.data[15:8]);
                    // Node 2778
                    swap(hdr.hdr_1.data[135:128], hdr.hdr_1.data[103:96]);
                    swap(hdr.hdr_1.data[143:136], hdr.hdr_1.data[111:104]);
                    swap(hdr.hdr_1.data[151:144], hdr.hdr_1.data[119:112]);
                    swap(hdr.hdr_1.data[159:152], hdr.hdr_1.data[127:120]);
                    // Node 3078
                    swap(hdr.hdr_0.data[55:48], hdr.hdr_0.data[7:0]);
                    swap(hdr.hdr_0.data[63:56], hdr.hdr_0.data[15:8]);
                    swap(hdr.hdr_0.data[71:64], hdr.hdr_0.data[23:16]);
                    swap(hdr.hdr_0.data[79:72], hdr.hdr_0.data[31:24]);
                    swap(hdr.hdr_0.data[87:80], hdr.hdr_0.data[39:32]);
                    swap(hdr.hdr_0.data[95:88], hdr.hdr_0.data[47:40]);
                    // Node 3341
                    fwd(0);
                  }
                } else {
                  // Node 400
                  // Node 450
                  hdr.cpu.hit_0 = hit_0;
                  hdr.cpu.table_1073952504_39_key_0 = table_1073952504_39_key_0;
                  send_to_controller(450);
                  // Node 3448
                  // Node 3449
                  // Node 3504
                  // Node 3560
                  // Node 3617
                  // Node 3791
                  // Node 3851
                  // Node 3912
                  // Node 3974
                }
              }
            }
          }
          // Node 88
          // Node 2140
        }
        // Node 44
        // Node 1520
      }
      // Node 16
      // Node 996
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
