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
  DONE    = 0x04
}

enum bit<2> fwd_op_t {
  FORWARD_NF_DEV  = 0,
  FORWARD_TO_CPU  = 1,
  RECIRCULATE     = 2,
  DROP            = 3
}

header cpu_h {
  bit<16> code_path;                  // Written by the data plane
  bit<16> egress_dev;                 // Written by the control plane
  bit<8> trigger_dataplane_execution; // Written by the control plane
  // Where the packet came in, for a packet the controller hands back to be executed again: it
  // returns on the CPU port, so ingress_port_to_nf_dev no longer knows either. What the
  // recirculation header carries for the same reason. Written by the data plane.
  bit<16> ingress_dev;
  bit<16> ingress_port;
  bit<32> time; // The ingress clock at the hand-off, the controller's now for the packet.
  bit<32> cached_insert_success0;
  bit<32> dns_response_found;
  bit<32> dns_address;
  bit<32> dev;

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
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<16> data3;
}
header hdr1_h {
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<32> data3;
  bit<32> data4;
}
header hdr2_h {
  bit<32> data0;
  bit<32> data1;
}
header hdr3_h {
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
}
header dns_label_h {
  bit<8> len;
}
header dns_c1_h {
  bit<8> v;
}
header dns_c2_h {
  bit<16> v;
}
header dns_c4_h {
  bit<32> v;
}
header dns_c8_h {
  bit<64> v;
}
header dns_query_tc_h {
  bit<16> qtype;
  bit<16> qclass;
}
header dns_answer_h {
  bit<16> name;
  bit<16> rr_type;
  bit<16> rr_class;
  bit<32> ttl;
  bit<8> rd_length_hi;
  bit<8> rd_length_lo;
}
header dns_a_ip_h {
  bit<32> address;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;
  dns_label_h dns_l0;
  dns_c1_h dns_l0_c1;
  dns_c2_h dns_l0_c2;
  dns_c4_h dns_l0_c4;
  dns_c8_h dns_l0_c8;
  dns_label_h dns_l1;
  dns_c1_h dns_l1_c1;
  dns_c2_h dns_l1_c2;
  dns_c4_h dns_l1_c4;
  dns_c8_h dns_l1_c8;
  dns_label_h dns_l2;
  dns_c1_h dns_l2_c1;
  dns_c2_h dns_l2_c2;
  dns_c4_h dns_l2_c4;
  dns_c8_h dns_l2_c8;
  dns_label_h dns_l3;
  dns_c1_h dns_l3_c1;
  dns_c2_h dns_l3_c2;
  dns_c4_h dns_l3_c4;
  dns_c8_h dns_l3_c8;
  dns_label_h dns_l4;
  dns_query_tc_h dns_query_tc;
  dns_answer_h dns_cname;
  dns_answer_h dns_answer;
  dns_a_ip_h dns_a_ip;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  // What the forwarding table decided: 0 the packet stays on the switch (recirculated or
  // dropped), 1 it leaves, 2 it goes to the controller. The headers carrying data-plane state are
  // dropped on the strength of this, after the table and outside its actions, because a packet
  // that still has the egress ahead of it must keep them: the egress parser extracts them.
  bit<2> leaving;
  bit<32> fcfs_ct_1074083024_key_32b_0;
  bit<32> fcfs_ct_1074083024_key_32b_1;
  bit<32> punt_deadline; // meta.time minus the punt gate's window
  bool hit0;
  bit<32> vector_reg_value0;
  bit<32> regexec_vector_register_1074115080_0_read_592_index0;
  bit<32> reg_incr0;
  bit<8> dns_labels;
  bit<32> dns_response_found;
  bit<32> dns_address;
  bool hit1;
  bool hit2;
  bool hit3;
  bit<32> regexec_vector_register_1074115080_0_write_7983_index0;
  bit<32> regexec_vector_register_1074115080_0_write_10010_index0;
  bool hit4;
  bit<32> vector_reg_value1;
  bit<32> regexec_vector_register_1074115080_0_read_1854_index0;
  bit<32> reg_incr1;
  bit<16> pkt_len;
  bit<1> pc_45;
  bit<1> pc_47;
  bit<1> pc_49;
  bit<1> pc_67;
  bit<1> pc_68;

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
  ParserCounter() dns_counter;


  /* This is a mandatory state, required by Tofino Architecture */
  state start {
    tofino_parser.apply(pkt, ig_intr_md);

    meta.ingress_port = (bit<16>)ig_intr_md.ingress_port;
    meta.dev = 0;
    meta.leaving = 0;
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

    // A packet the controller declined re-enters the pipeline from the top, so its own headers
    // have to be parsed again -- every branch down there asks whether they are valid. It parsed
    // on the way in, or it would have been rejected before ever reaching the controller. One the
    // controller has already decided on is only forwarded, and its bytes stay payload.
    transition select(hdr.cpu.trigger_dataplane_execution) {
      8w1: parser_init;
      default: accept;
    }
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
    meta.pc_45 = 0;
    meta.pc_47 = 0;
    meta.pc_49 = 0;
    meta.pc_67 = 0;
    meta.pc_68 = 0;
    pkt.extract(hdr.hdr0);
    transition parser_45;
  }
  state parser_45 {
    transition parser_45_0;
  }
  state parser_45_0 {
    transition select (hdr.hdr0.data3) {
      16w0x0800: parser_46;
      default: parser_152;
    }
  }
  state parser_46 {
    meta.pc_45 = 1;
    pkt.extract(hdr.hdr1);
    transition parser_47;
  }
  state parser_152 {
    transition accept;
  }
  state parser_47 {
    transition parser_47_0;
  }
  state parser_47_0 {
    transition select (hdr.hdr1.data2[23:0][23:16]) {
      8w0x11: parser_48;
      default: parser_140;
    }
  }
  state parser_48 {
    meta.pc_47 = 1;
    pkt.extract(hdr.hdr2);
    transition parser_49;
  }
  state parser_140 {
    transition accept;
  }
  state parser_49 {
    transition parser_49_0;
  }
  state parser_49_0 {
    transition select (hdr.hdr2.data0[15:0]) {
      16w0x0035: parser_67;
      default: parser_49_1;
    }
  }
  state parser_49_1 {
    transition select (hdr.hdr2.data0[31:16]) {
      16w0x0035: parser_67;
      default: parser_55;
    }
  }
  state parser_55 {
    meta.pc_49 = 1;
    transition accept;
  }
  state parser_67 {
    transition parser_67_0;
  }
  state parser_67_0 {
    transition select (hdr.hdr2.data1[31:16]) {
      16w0x0000: parser_135;
      16w0x0001: parser_135;
      16w0x0002: parser_135;
      16w0x0003: parser_135;
      16w0x0004: parser_135;
      16w0x0005: parser_135;
      16w0x0006: parser_135;
      16w0x0007: parser_135;
      16w0x0008: parser_135;
      16w0x0009: parser_135;
      16w0x000a: parser_135;
      16w0x000b: parser_135;
      16w0x000c: parser_135;
      16w0x000d: parser_135;
      16w0x000e: parser_135;
      16w0x000f: parser_135;
      16w0x0010: parser_135;
      16w0x0011: parser_135;
      16w0x0012: parser_135;
      16w0x0013: parser_135;
      default: parser_68;
    }
  }
  state parser_68 {
    meta.pc_67 = 1;
    transition parser_69;
  }
  state parser_135 {
    transition accept;
  }
  state parser_69 {
    meta.pc_68 = 1;
    pkt.extract(hdr.hdr3);
    transition parser_71;
  }
  state parser_71 {
    meta.dns_labels = 0;
    meta.dns_response_found = 0;
    transition parser_71_label_0;
  }
  state parser_71_label_0 {
    pkt.extract(hdr.dns_l0);
    transition select (hdr.dns_l0.len) {
      0: parser_71_end_0;
      1: parser_71_label_0_len_1;
      2: parser_71_label_0_len_2;
      3: parser_71_label_0_len_3;
      4: parser_71_label_0_len_4;
      5: parser_71_label_0_len_5;
      6: parser_71_label_0_len_6;
      7: parser_71_label_0_len_7;
      8: parser_71_label_0_len_8;
      9: parser_71_label_0_len_9;
      10: parser_71_label_0_len_10;
      11: parser_71_label_0_len_11;
      12: parser_71_label_0_len_12;
      13: parser_71_label_0_len_13;
      14: parser_71_label_0_len_14;
      15: parser_71_label_0_len_15;
      default: accept;
    }
  }
  state parser_71_label_0_len_1 {
    pkt.extract(hdr.dns_l0_c1);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_2 {
    pkt.extract(hdr.dns_l0_c2);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_3 {
    pkt.extract(hdr.dns_l0_c1);
    pkt.extract(hdr.dns_l0_c2);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_4 {
    pkt.extract(hdr.dns_l0_c4);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_5 {
    pkt.extract(hdr.dns_l0_c1);
    pkt.extract(hdr.dns_l0_c4);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_6 {
    pkt.extract(hdr.dns_l0_c2);
    pkt.extract(hdr.dns_l0_c4);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_7 {
    pkt.extract(hdr.dns_l0_c1);
    pkt.extract(hdr.dns_l0_c2);
    pkt.extract(hdr.dns_l0_c4);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_8 {
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_9 {
    pkt.extract(hdr.dns_l0_c1);
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_10 {
    pkt.extract(hdr.dns_l0_c2);
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_11 {
    pkt.extract(hdr.dns_l0_c1);
    pkt.extract(hdr.dns_l0_c2);
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_12 {
    pkt.extract(hdr.dns_l0_c4);
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_13 {
    pkt.extract(hdr.dns_l0_c1);
    pkt.extract(hdr.dns_l0_c4);
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_14 {
    pkt.extract(hdr.dns_l0_c2);
    pkt.extract(hdr.dns_l0_c4);
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_0_len_15 {
    pkt.extract(hdr.dns_l0_c1);
    pkt.extract(hdr.dns_l0_c2);
    pkt.extract(hdr.dns_l0_c4);
    pkt.extract(hdr.dns_l0_c8);
    transition parser_71_label_1;
  }
  state parser_71_label_1 {
    pkt.extract(hdr.dns_l1);
    transition select (hdr.dns_l1.len) {
      0: parser_71_end_1;
      1: parser_71_label_1_len_1;
      2: parser_71_label_1_len_2;
      3: parser_71_label_1_len_3;
      4: parser_71_label_1_len_4;
      5: parser_71_label_1_len_5;
      6: parser_71_label_1_len_6;
      7: parser_71_label_1_len_7;
      8: parser_71_label_1_len_8;
      9: parser_71_label_1_len_9;
      10: parser_71_label_1_len_10;
      11: parser_71_label_1_len_11;
      12: parser_71_label_1_len_12;
      13: parser_71_label_1_len_13;
      14: parser_71_label_1_len_14;
      15: parser_71_label_1_len_15;
      default: accept;
    }
  }
  state parser_71_label_1_len_1 {
    pkt.extract(hdr.dns_l1_c1);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_2 {
    pkt.extract(hdr.dns_l1_c2);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_3 {
    pkt.extract(hdr.dns_l1_c1);
    pkt.extract(hdr.dns_l1_c2);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_4 {
    pkt.extract(hdr.dns_l1_c4);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_5 {
    pkt.extract(hdr.dns_l1_c1);
    pkt.extract(hdr.dns_l1_c4);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_6 {
    pkt.extract(hdr.dns_l1_c2);
    pkt.extract(hdr.dns_l1_c4);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_7 {
    pkt.extract(hdr.dns_l1_c1);
    pkt.extract(hdr.dns_l1_c2);
    pkt.extract(hdr.dns_l1_c4);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_8 {
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_9 {
    pkt.extract(hdr.dns_l1_c1);
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_10 {
    pkt.extract(hdr.dns_l1_c2);
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_11 {
    pkt.extract(hdr.dns_l1_c1);
    pkt.extract(hdr.dns_l1_c2);
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_12 {
    pkt.extract(hdr.dns_l1_c4);
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_13 {
    pkt.extract(hdr.dns_l1_c1);
    pkt.extract(hdr.dns_l1_c4);
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_14 {
    pkt.extract(hdr.dns_l1_c2);
    pkt.extract(hdr.dns_l1_c4);
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_1_len_15 {
    pkt.extract(hdr.dns_l1_c1);
    pkt.extract(hdr.dns_l1_c2);
    pkt.extract(hdr.dns_l1_c4);
    pkt.extract(hdr.dns_l1_c8);
    transition parser_71_label_2;
  }
  state parser_71_label_2 {
    pkt.extract(hdr.dns_l2);
    transition select (hdr.dns_l2.len) {
      0: parser_71_end_2;
      1: parser_71_label_2_len_1;
      2: parser_71_label_2_len_2;
      3: parser_71_label_2_len_3;
      4: parser_71_label_2_len_4;
      5: parser_71_label_2_len_5;
      6: parser_71_label_2_len_6;
      7: parser_71_label_2_len_7;
      8: parser_71_label_2_len_8;
      9: parser_71_label_2_len_9;
      10: parser_71_label_2_len_10;
      11: parser_71_label_2_len_11;
      12: parser_71_label_2_len_12;
      13: parser_71_label_2_len_13;
      14: parser_71_label_2_len_14;
      15: parser_71_label_2_len_15;
      default: accept;
    }
  }
  state parser_71_label_2_len_1 {
    pkt.extract(hdr.dns_l2_c1);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_2 {
    pkt.extract(hdr.dns_l2_c2);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_3 {
    pkt.extract(hdr.dns_l2_c1);
    pkt.extract(hdr.dns_l2_c2);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_4 {
    pkt.extract(hdr.dns_l2_c4);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_5 {
    pkt.extract(hdr.dns_l2_c1);
    pkt.extract(hdr.dns_l2_c4);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_6 {
    pkt.extract(hdr.dns_l2_c2);
    pkt.extract(hdr.dns_l2_c4);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_7 {
    pkt.extract(hdr.dns_l2_c1);
    pkt.extract(hdr.dns_l2_c2);
    pkt.extract(hdr.dns_l2_c4);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_8 {
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_9 {
    pkt.extract(hdr.dns_l2_c1);
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_10 {
    pkt.extract(hdr.dns_l2_c2);
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_11 {
    pkt.extract(hdr.dns_l2_c1);
    pkt.extract(hdr.dns_l2_c2);
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_12 {
    pkt.extract(hdr.dns_l2_c4);
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_13 {
    pkt.extract(hdr.dns_l2_c1);
    pkt.extract(hdr.dns_l2_c4);
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_14 {
    pkt.extract(hdr.dns_l2_c2);
    pkt.extract(hdr.dns_l2_c4);
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_2_len_15 {
    pkt.extract(hdr.dns_l2_c1);
    pkt.extract(hdr.dns_l2_c2);
    pkt.extract(hdr.dns_l2_c4);
    pkt.extract(hdr.dns_l2_c8);
    transition parser_71_label_3;
  }
  state parser_71_label_3 {
    pkt.extract(hdr.dns_l3);
    transition select (hdr.dns_l3.len) {
      0: parser_71_end_3;
      1: parser_71_label_3_len_1;
      2: parser_71_label_3_len_2;
      3: parser_71_label_3_len_3;
      4: parser_71_label_3_len_4;
      5: parser_71_label_3_len_5;
      6: parser_71_label_3_len_6;
      7: parser_71_label_3_len_7;
      8: parser_71_label_3_len_8;
      9: parser_71_label_3_len_9;
      10: parser_71_label_3_len_10;
      11: parser_71_label_3_len_11;
      12: parser_71_label_3_len_12;
      13: parser_71_label_3_len_13;
      14: parser_71_label_3_len_14;
      15: parser_71_label_3_len_15;
      default: accept;
    }
  }
  state parser_71_label_3_len_1 {
    pkt.extract(hdr.dns_l3_c1);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_2 {
    pkt.extract(hdr.dns_l3_c2);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_3 {
    pkt.extract(hdr.dns_l3_c1);
    pkt.extract(hdr.dns_l3_c2);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_4 {
    pkt.extract(hdr.dns_l3_c4);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_5 {
    pkt.extract(hdr.dns_l3_c1);
    pkt.extract(hdr.dns_l3_c4);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_6 {
    pkt.extract(hdr.dns_l3_c2);
    pkt.extract(hdr.dns_l3_c4);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_7 {
    pkt.extract(hdr.dns_l3_c1);
    pkt.extract(hdr.dns_l3_c2);
    pkt.extract(hdr.dns_l3_c4);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_8 {
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_9 {
    pkt.extract(hdr.dns_l3_c1);
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_10 {
    pkt.extract(hdr.dns_l3_c2);
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_11 {
    pkt.extract(hdr.dns_l3_c1);
    pkt.extract(hdr.dns_l3_c2);
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_12 {
    pkt.extract(hdr.dns_l3_c4);
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_13 {
    pkt.extract(hdr.dns_l3_c1);
    pkt.extract(hdr.dns_l3_c4);
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_14 {
    pkt.extract(hdr.dns_l3_c2);
    pkt.extract(hdr.dns_l3_c4);
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_label_3_len_15 {
    pkt.extract(hdr.dns_l3_c1);
    pkt.extract(hdr.dns_l3_c2);
    pkt.extract(hdr.dns_l3_c4);
    pkt.extract(hdr.dns_l3_c8);
    transition parser_71_terminator;
  }
  state parser_71_terminator {
    pkt.extract(hdr.dns_l4);
    transition select (hdr.dns_l4.len) {
      0: parser_71_end_4;
      default: accept;
    }
  }
  state parser_71_end_0 {
    meta.dns_labels = 0;
    transition parser_71_query_tc;
  }
  state parser_71_end_1 {
    meta.dns_labels = 1;
    transition parser_71_query_tc;
  }
  state parser_71_end_2 {
    meta.dns_labels = 2;
    transition parser_71_query_tc;
  }
  state parser_71_end_3 {
    meta.dns_labels = 3;
    transition parser_71_query_tc;
  }
  state parser_71_end_4 {
    meta.dns_labels = 4;
    transition parser_71_query_tc;
  }
  state parser_71_query_tc {
    pkt.extract(hdr.dns_query_tc);
    transition parser_71_answer;
  }
  state parser_71_answer {
    transition select (pkt.lookahead<bit<32>>()[15:0]) {
      1: parser_71_a;
      5: parser_71_cname;
      default: accept;
    }
  }
  state parser_71_a {
    pkt.extract(hdr.dns_answer);
    pkt.extract(hdr.dns_a_ip);
    meta.dns_response_found = 1;
    transition parser_77;
  }
  state parser_71_cname {
    pkt.extract(hdr.dns_cname);
    transition select (hdr.dns_cname.rd_length_lo) {
      1: parser_71_cname_skip;
      2: parser_71_cname_skip;
      3: parser_71_cname_skip;
      4: parser_71_cname_skip;
      5: parser_71_cname_skip;
      6: parser_71_cname_skip;
      7: parser_71_cname_skip;
      8: parser_71_cname_skip;
      9: parser_71_cname_skip;
      10: parser_71_cname_skip;
      11: parser_71_cname_skip;
      12: parser_71_cname_skip;
      13: parser_71_cname_skip;
      14: parser_71_cname_skip;
      15: parser_71_cname_skip;
      16: parser_71_cname_skip;
      17: parser_71_cname_skip;
      18: parser_71_cname_skip;
      19: parser_71_cname_skip;
      20: parser_71_cname_skip;
      21: parser_71_cname_skip;
      22: parser_71_cname_skip;
      23: parser_71_cname_skip;
      24: parser_71_cname_skip;
      25: parser_71_cname_skip;
      26: parser_71_cname_skip;
      27: parser_71_cname_skip;
      28: parser_71_cname_skip;
      29: parser_71_cname_skip;
      30: parser_71_cname_skip;
      31: parser_71_cname_skip;
      32: parser_71_cname_skip;
      33: parser_71_cname_skip;
      34: parser_71_cname_skip;
      35: parser_71_cname_skip;
      36: parser_71_cname_skip;
      37: parser_71_cname_skip;
      38: parser_71_cname_skip;
      39: parser_71_cname_skip;
      40: parser_71_cname_skip;
      41: parser_71_cname_skip;
      42: parser_71_cname_skip;
      43: parser_71_cname_skip;
      44: parser_71_cname_skip;
      45: parser_71_cname_skip;
      46: parser_71_cname_skip;
      47: parser_71_cname_skip;
      48: parser_71_cname_skip;
      49: parser_71_cname_skip;
      50: parser_71_cname_skip;
      default: parser_71_cname_skip_cut;
    }
  }
  state parser_71_cname_skip {
    dns_counter.set(hdr.dns_cname.rd_length_lo);
    transition select (dns_counter.is_zero()) {
      true: parser_71_answer;
      false: parser_71_cname_byte;
    }
  }
  state parser_71_cname_skip_cut {
    dns_counter.set(hdr.dns_cname.rd_length_lo);
    pkt.advance(400);
    dns_counter.decrement(8w50);
    transition select (dns_counter.is_zero()) {
      true: parser_71_answer;
      false: parser_71_cname_byte;
    }
  }
  state parser_71_cname_byte {
    pkt.advance(8);
    dns_counter.decrement(8w1);
    transition select (dns_counter.is_zero()) {
      true: parser_71_answer;
      false: parser_71_cname_byte;
    }
  }
  state parser_77 {
    transition accept;
  }

}


@pa_no_overlay("ingress", "meta.regexec_vector_register_1074115080_0_read_592_index0")
@pa_no_overlay("ingress", "meta.regexec_vector_register_1074115080_0_write_7983_index0")
@pa_no_overlay("ingress", "meta.regexec_vector_register_1074115080_0_write_10010_index0")
@pa_no_overlay("ingress", "meta.regexec_vector_register_1074115080_0_read_1854_index0")
@pa_solitary("ingress", "meta.pc_45")
@pa_solitary("ingress", "meta.pc_47")
@pa_solitary("ingress", "meta.pc_49")
@pa_solitary("ingress", "meta.pc_67")
@pa_solitary("ingress", "meta.pc_68")


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
    hdr.cuckoo.setInvalid();
    meta.leaving = 2;
    fwd(CPU_PCIE_PORT);
  }

  action fwd_nf_dev(bit<16> port) {
    hdr.cpu.setInvalid();
    hdr.cuckoo.setInvalid();
    meta.leaving = 1;
    fwd(port);
  }

  action set_ingress_dev(bit<32> nf_dev) {
    meta.dev = nf_dev;
  }

  action set_ingress_dev_from_recirculation() {
    meta.ingress_port = hdr.recirc.ingress_port;
    meta.dev = hdr.recirc.dev;
  }

  action set_ingress_dev_from_cpu() {
    meta.ingress_port = hdr.cpu.ingress_port;
    meta.dev = (bit<32>)hdr.cpu.ingress_dev;
  }

  table ingress_port_to_nf_dev {
    key = {
      meta.ingress_port: exact;
    }
    actions = {
      set_ingress_dev;
      set_ingress_dev_from_recirculation;
      set_ingress_dev_from_cpu;
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

  // Swapping two fields a byte at a time forces byte-granular PHV slicing on both of them, and a
  // field sliced that way drags its neighbours into the same container group; bf-p4c then cannot
  // satisfy the action constraints. Swap whole fields where the byte pairs make one up.
  action swap16(inout bit<16> a, inout bit<16> b) {
    bit<16> tmp = a;
    a = b;
    b = tmp;
  }

  action swap24(inout bit<24> a, inout bit<24> b) {
    bit<24> tmp = a;
    a = b;
    b = tmp;
  }

  action swap32(inout bit<32> a, inout bit<32> b) {
    bit<32> tmp = a;
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
    hdr.cpu.ingress_dev = meta.dev[15:0];
    hdr.cpu.ingress_port = meta.ingress_port;
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

  Hash<bit<3>>(HashAlgorithm_t.CRC32) fcfs_ct_1074083024_hash_50;
  Hash<bit<3>>(HashAlgorithm_t.CRC32) fcfs_ct_1074083024_hash_136;
  Hash<bit<3>>(HashAlgorithm_t.CRC32) fcfs_ct_1074083024_hash_89;
  Register<bit<32>,_>(65536, 0) fcfs_ct_1074083024_reg_liveness;
  RegisterAction<bit<32>, bit<32>, bool>(fcfs_ct_1074083024_reg_liveness) fcfs_ct_1074083024_reg_liveness_query_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
      } else {
        was_alive = true;
      }
    }
  };

  RegisterAction<bit<32>, bit<32>, bool>(fcfs_ct_1074083024_reg_liveness) fcfs_ct_1074083024_reg_liveness_query_and_refresh_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
      } else {
        was_alive = true;
      }
      alarm = meta.time + 16384;
    }
  };

  Register<bit<32>,_>(8, 0) fcfs_ct_1074083024_reg_key_0;
  RegisterAction<bit<32>, bit<3>, void>(fcfs_ct_1074083024_reg_key_0) fcfs_ct_1074083024_reg_key_0_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_ct_1074083024_key_32b_0;
    }
  };

  RegisterAction<bit<32>, bit<3>, bit<8>>(fcfs_ct_1074083024_reg_key_0) fcfs_ct_1074083024_reg_key_0_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_ct_1074083024_key_32b_0) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<32>,_>(8, 0) fcfs_ct_1074083024_reg_key_1;
  RegisterAction<bit<32>, bit<3>, void>(fcfs_ct_1074083024_reg_key_1) fcfs_ct_1074083024_reg_key_1_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_ct_1074083024_key_32b_1;
    }
  };

  RegisterAction<bit<32>, bit<3>, bit<8>>(fcfs_ct_1074083024_reg_key_1) fcfs_ct_1074083024_reg_key_1_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_ct_1074083024_key_32b_1) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<32>,_>(8, 0) fcfs_ct_1074083024_reg_punt_gate;
  RegisterAction<bit<32>, bit<3>, bool>(fcfs_ct_1074083024_reg_punt_gate) fcfs_ct_1074083024_reg_punt_gate_claim_if_stale = {
    void apply(inout bit<32> stamp, out bool claimed) {
      if (stamp < meta.punt_deadline) {
        claimed = true;
        stamp = meta.time;
      } else {
        claimed = false;
      }
    }
  };

  bit<32> fcfs_ct_1074083024_table_50_get_value_param0 = 32w0;
  action fcfs_ct_1074083024_table_50_get_value(bit<32> _fcfs_ct_1074083024_table_50_get_value_param0) {
    fcfs_ct_1074083024_table_50_get_value_param0 = _fcfs_ct_1074083024_table_50_get_value_param0;
  }

  table fcfs_ct_1074083024_table_50 {
    key = {
      meta.fcfs_ct_1074083024_key_32b_0: exact;
      meta.fcfs_ct_1074083024_key_32b_1: exact;
    }
    actions = {
      fcfs_ct_1074083024_table_50_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<3> fcfs_ct_1074083024_hash_50_value;
  action fcfs_ct_1074083024_hash_50_calc() {
    fcfs_ct_1074083024_hash_50_value = fcfs_ct_1074083024_hash_50.get({
      meta.fcfs_ct_1074083024_key_32b_0,
      meta.fcfs_ct_1074083024_key_32b_1
      });
      fcfs_ct_1074083024_table_50_get_value_param0[2:0] = fcfs_ct_1074083024_hash_50_value;
  }
  bit<8> match_counter0 = 0;
  action fcfs_ct_1074083024_check_key_0_50() {
    match_counter0 = match_counter0 + fcfs_ct_1074083024_reg_key_0_check_value.execute(fcfs_ct_1074083024_hash_50_value);
  }
  action fcfs_ct_1074083024_check_key_1_50() {
    match_counter0 = match_counter0 + fcfs_ct_1074083024_reg_key_1_check_value.execute(fcfs_ct_1074083024_hash_50_value);
  }
  Register<bit<32>,_>(65536, 0) vector_register_1074115080_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074115080_0) vector_register_1074115080_0_read_592 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action regexec_vector_register_1074115080_0_read_592() {
    meta.vector_reg_value0 = vector_register_1074115080_0_read_592.execute(meta.regexec_vector_register_1074115080_0_read_592_index0);
  }
  Register<bit<32>,_>(2048, 0) vector_register_1074167072_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074167072_0) vector_register_1074167072_0_add_value_977 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 32w0x00000001;
      out_value = value;
    }
  };

  Register<bit<32>,_>(2048, 0) vector_register_1074184288_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074184288_0) vector_register_1074184288_0_add_value_1153 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + meta.reg_incr0;
      out_value = value;
    }
  };

  bit<8> dns_name_l0_len;
  bit<8> dns_name_l0_c1;
  bit<16> dns_name_l0_c2;
  bit<32> dns_name_l0_c4;
  bit<64> dns_name_l0_c8;
  bit<8> dns_name_l1_len;
  bit<8> dns_name_l1_c1;
  bit<16> dns_name_l1_c2;
  bit<32> dns_name_l1_c4;
  bit<64> dns_name_l1_c8;
  bit<8> dns_name_l2_len;
  bit<8> dns_name_l2_c1;
  bit<16> dns_name_l2_c2;
  bit<32> dns_name_l2_c4;
  bit<64> dns_name_l2_c8;
  bit<8> dns_name_l3_len;
  bit<8> dns_name_l3_c1;
  bit<16> dns_name_l3_c2;
  bit<32> dns_name_l3_c4;
  bit<64> dns_name_l3_c8;
  action dns_name_build_1() {
    dns_name_l0_len = hdr.dns_l0.len;
    dns_name_l0_c1 = hdr.dns_l0_c1.v;
    dns_name_l0_c2 = hdr.dns_l0_c2.v;
    dns_name_l0_c4 = hdr.dns_l0_c4.v;
    dns_name_l0_c8 = hdr.dns_l0_c8.v;
  }
  action dns_name_build_2() {
    dns_name_l1_len = hdr.dns_l0.len;
    dns_name_l1_c1 = hdr.dns_l0_c1.v;
    dns_name_l1_c2 = hdr.dns_l0_c2.v;
    dns_name_l1_c4 = hdr.dns_l0_c4.v;
    dns_name_l1_c8 = hdr.dns_l0_c8.v;
    dns_name_l0_len = hdr.dns_l1.len;
    dns_name_l0_c1 = hdr.dns_l1_c1.v;
    dns_name_l0_c2 = hdr.dns_l1_c2.v;
    dns_name_l0_c4 = hdr.dns_l1_c4.v;
    dns_name_l0_c8 = hdr.dns_l1_c8.v;
  }
  action dns_name_build_3() {
    dns_name_l2_len = hdr.dns_l0.len;
    dns_name_l2_c1 = hdr.dns_l0_c1.v;
    dns_name_l2_c2 = hdr.dns_l0_c2.v;
    dns_name_l2_c4 = hdr.dns_l0_c4.v;
    dns_name_l2_c8 = hdr.dns_l0_c8.v;
    dns_name_l1_len = hdr.dns_l1.len;
    dns_name_l1_c1 = hdr.dns_l1_c1.v;
    dns_name_l1_c2 = hdr.dns_l1_c2.v;
    dns_name_l1_c4 = hdr.dns_l1_c4.v;
    dns_name_l1_c8 = hdr.dns_l1_c8.v;
    dns_name_l0_len = hdr.dns_l2.len;
    dns_name_l0_c1 = hdr.dns_l2_c1.v;
    dns_name_l0_c2 = hdr.dns_l2_c2.v;
    dns_name_l0_c4 = hdr.dns_l2_c4.v;
    dns_name_l0_c8 = hdr.dns_l2_c8.v;
  }
  action dns_name_build_4() {
    dns_name_l3_len = hdr.dns_l0.len;
    dns_name_l3_c1 = hdr.dns_l0_c1.v;
    dns_name_l3_c2 = hdr.dns_l0_c2.v;
    dns_name_l3_c4 = hdr.dns_l0_c4.v;
    dns_name_l3_c8 = hdr.dns_l0_c8.v;
    dns_name_l2_len = hdr.dns_l1.len;
    dns_name_l2_c1 = hdr.dns_l1_c1.v;
    dns_name_l2_c2 = hdr.dns_l1_c2.v;
    dns_name_l2_c4 = hdr.dns_l1_c4.v;
    dns_name_l2_c8 = hdr.dns_l1_c8.v;
    dns_name_l1_len = hdr.dns_l2.len;
    dns_name_l1_c1 = hdr.dns_l2_c1.v;
    dns_name_l1_c2 = hdr.dns_l2_c2.v;
    dns_name_l1_c4 = hdr.dns_l2_c4.v;
    dns_name_l1_c8 = hdr.dns_l2_c8.v;
    dns_name_l0_len = hdr.dns_l3.len;
    dns_name_l0_c1 = hdr.dns_l3_c1.v;
    dns_name_l0_c2 = hdr.dns_l3_c2.v;
    dns_name_l0_c4 = hdr.dns_l3_c4.v;
    dns_name_l0_c8 = hdr.dns_l3_c8.v;
  }
  table dns_name_build {
    key = { meta.dns_labels: exact; }
    actions = { dns_name_build_1; dns_name_build_2; dns_name_build_3; dns_name_build_4; }
    const entries = {
      1: dns_name_build_1();
      2: dns_name_build_2();
      3: dns_name_build_3();
      4: dns_name_build_4();
    }
    size = 4;
  }
  bit<32> lpm_1074082560_set_value_param0 = 32w0;
  action lpm_1074082560_set_value(bit<32> _lpm_1074082560_set_value_param0) {
    lpm_1074082560_set_value_param0 = _lpm_1074082560_set_value_param0;
  }

  table lpm_1074082560 {
    key = {
      meta.dns_labels: ternary;
      dns_name_l0_len: ternary;
      dns_name_l0_c1: ternary;
      dns_name_l0_c2: ternary;
      dns_name_l0_c4: ternary;
      dns_name_l0_c8: ternary;
      dns_name_l1_len: ternary;
      dns_name_l1_c1: ternary;
      dns_name_l1_c2: ternary;
      dns_name_l1_c4: ternary;
      dns_name_l1_c8: ternary;
      dns_name_l2_len: ternary;
      dns_name_l2_c1: ternary;
      dns_name_l2_c2: ternary;
      dns_name_l2_c4: ternary;
      dns_name_l2_c8: ternary;
      dns_name_l3_len: ternary;
      dns_name_l3_c1: ternary;
      dns_name_l3_c2: ternary;
      dns_name_l3_c4: ternary;
      dns_name_l3_c8: ternary;
    }
    actions = { lpm_1074082560_set_value; }
    size = 2048;
  }
  bit<32> lpm_1074082784_set_value_param0 = 32w0;
  action lpm_1074082784_set_value(bit<32> _lpm_1074082784_set_value_param0) {
    lpm_1074082784_set_value_param0 = _lpm_1074082784_set_value_param0;
  }

  table lpm_1074082784 {
    key = {
      hdr.hdr1.data4: ternary;
    }
    actions = { lpm_1074082784_set_value; }
    size = 2048;
  }
  Register<bit<32>,_>(2048, 0) vector_register_1074132640_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074132640_0) vector_register_1074132640_0_add_value_6147 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 32w0x00000001;
      out_value = value;
    }
  };

  bit<32> fcfs_ct_1074083024_table_89_get_value_param0 = 32w0;
  action fcfs_ct_1074083024_table_89_get_value(bit<32> _fcfs_ct_1074083024_table_89_get_value_param0) {
    fcfs_ct_1074083024_table_89_get_value_param0 = _fcfs_ct_1074083024_table_89_get_value_param0;
  }

  table fcfs_ct_1074083024_table_89 {
    key = {
      meta.fcfs_ct_1074083024_key_32b_0: exact;
      meta.fcfs_ct_1074083024_key_32b_1: exact;
    }
    actions = {
      fcfs_ct_1074083024_table_89_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<3> fcfs_ct_1074083024_hash_89_value;
  action fcfs_ct_1074083024_hash_89_calc() {
    fcfs_ct_1074083024_hash_89_value = fcfs_ct_1074083024_hash_89.get({
      meta.fcfs_ct_1074083024_key_32b_0,
      meta.fcfs_ct_1074083024_key_32b_1
      });
      fcfs_ct_1074083024_table_89_get_value_param0[2:0] = fcfs_ct_1074083024_hash_89_value;
  }
  bool punt_allowed0 = true;
  bit<8> match_counter1 = 0;
  action fcfs_ct_1074083024_check_key_0_89() {
    match_counter1 = match_counter1 + fcfs_ct_1074083024_reg_key_0_check_value.execute(fcfs_ct_1074083024_hash_89_value);
  }
  action fcfs_ct_1074083024_check_key_1_89() {
    match_counter1 = match_counter1 + fcfs_ct_1074083024_reg_key_1_check_value.execute(fcfs_ct_1074083024_hash_89_value);
  }
  action punt_gate_64770() {
    punt_allowed0 = fcfs_ct_1074083024_reg_punt_gate_claim_if_stale.execute(fcfs_ct_1074083024_hash_89_value);
  }

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1074115080_0) vector_register_1074115080_0_write_7983 = {
    void apply(inout bit<32> value) {
      value = lpm_1074082560_set_value_param0;
    }
  };

  action regexec_vector_register_1074115080_0_write_7983() {
    vector_register_1074115080_0_write_7983.execute(meta.regexec_vector_register_1074115080_0_write_7983_index0);
  }

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1074115080_0) vector_register_1074115080_0_write_10010 = {
    void apply(inout bit<32> value) {
      value = lpm_1074082560_set_value_param0;
    }
  };

  action regexec_vector_register_1074115080_0_write_10010() {
    vector_register_1074115080_0_write_10010.execute(meta.regexec_vector_register_1074115080_0_write_10010_index0);
  }
  bit<32> fcfs_ct_1074083024_table_136_get_value_param0 = 32w0;
  action fcfs_ct_1074083024_table_136_get_value(bit<32> _fcfs_ct_1074083024_table_136_get_value_param0) {
    fcfs_ct_1074083024_table_136_get_value_param0 = _fcfs_ct_1074083024_table_136_get_value_param0;
  }

  table fcfs_ct_1074083024_table_136 {
    key = {
      meta.fcfs_ct_1074083024_key_32b_0: exact;
      meta.fcfs_ct_1074083024_key_32b_1: exact;
    }
    actions = {
      fcfs_ct_1074083024_table_136_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<3> fcfs_ct_1074083024_hash_136_value;
  action fcfs_ct_1074083024_hash_136_calc() {
    fcfs_ct_1074083024_hash_136_value = fcfs_ct_1074083024_hash_136.get({
      meta.fcfs_ct_1074083024_key_32b_0,
      meta.fcfs_ct_1074083024_key_32b_1
      });
      fcfs_ct_1074083024_table_136_get_value_param0[2:0] = fcfs_ct_1074083024_hash_136_value;
  }
  bit<8> match_counter2 = 0;
  action fcfs_ct_1074083024_check_key_0_136() {
    match_counter2 = match_counter2 + fcfs_ct_1074083024_reg_key_0_check_value.execute(fcfs_ct_1074083024_hash_136_value);
  }
  action fcfs_ct_1074083024_check_key_1_136() {
    match_counter2 = match_counter2 + fcfs_ct_1074083024_reg_key_1_check_value.execute(fcfs_ct_1074083024_hash_136_value);
  }

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074115080_0) vector_register_1074115080_0_read_1854 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action regexec_vector_register_1074115080_0_read_1854() {
    meta.vector_reg_value1 = vector_register_1074115080_0_read_1854.execute(meta.regexec_vector_register_1074115080_0_read_1854_index0);
  }

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074167072_0) vector_register_1074167072_0_add_value_2359 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 32w0x00000001;
      out_value = value;
    }
  };


  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074184288_0) vector_register_1074184288_0_add_value_2617 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + meta.reg_incr1;
      out_value = value;
    }
  };


  apply {
    meta.punt_deadline = meta.time - 128;
    meta.pkt_len = 0;
    if (hdr.hdr1.isValid()) {
      meta.pkt_len = hdr.hdr1.data0[15:0] + 14;
    }

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:Ignore
      // BDD node 43:expire_items_single_map
      // EP node  4:ParserExtraction
      // BDD node 44:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 45:if
        if (meta.pc_45 == 1) {
          // EP node  14:Then
          // BDD node 45:if
          // EP node  26:ParserExtraction
          // BDD node 46:packet_borrow_next_chunk
          if(hdr.hdr1.isValid()) {
            // EP node  58:ParserCondition
            // BDD node 47:if
            if (meta.pc_47 == 1) {
              // EP node  59:Then
              // BDD node 47:if
              // EP node  98:ParserExtraction
              // BDD node 48:packet_borrow_next_chunk
              if(hdr.hdr2.isValid()) {
                // EP node  171:ParserCondition
                // BDD node 49:if
                if (meta.pc_49 == 1) {
                  // EP node  172:Then
                  // BDD node 49:if
                  // EP node  243:FCFSCachedTableRead
                  // BDD node 50:map_get
                  meta.fcfs_ct_1074083024_key_32b_0 = hdr.hdr1.data4;
                  meta.fcfs_ct_1074083024_key_32b_1 = hdr.hdr1.data3;
                  meta.hit0 = fcfs_ct_1074083024_table_50.apply().hit;
                  if (!meta.hit0) {
                    fcfs_ct_1074083024_hash_50_calc();
                    bool fcfs_ct_is_alive0 = fcfs_ct_1074083024_reg_liveness_query_timestamp.execute(fcfs_ct_1074083024_table_50_get_value_param0);
                    if (fcfs_ct_is_alive0) {
                      fcfs_ct_1074083024_check_key_0_50();
                      fcfs_ct_1074083024_check_key_1_50();
                      if (match_counter0 == 2) {
                        meta.hit0 = true;
                      }
                    }
                  }
                  // EP node  473:If
                  // BDD node 51:if
                  if (!meta.hit0){
                    // EP node  474:Then
                    // BDD node 51:if
                    // EP node  20681:Forward
                    // BDD node 55:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  } else {
                    // EP node  475:Else
                    // BDD node 51:if
                    // EP node  529:Ignore
                    // BDD node 56:dchain_rejuvenate_index
                    // EP node  592:VectorRegisterLookup
                    // BDD node 57:vector_borrow
                    meta.regexec_vector_register_1074115080_0_read_592_index0 = fcfs_ct_1074083024_table_50_get_value_param0;
                    regexec_vector_register_1074115080_0_read_592();
                    // EP node  753:Ignore
                    // BDD node 58:vector_return
                    // EP node  842:Ignore
                    // BDD node 59:vector_borrow
                    // EP node  977:VectorRegisterUpdate
                    // BDD node 60:vector_return
                    bit<32> reg_new0 = vector_register_1074167072_0_add_value_977.execute(meta.vector_reg_value0);
                    // EP node  1052:Ignore
                    // BDD node 61:vector_borrow
                    // EP node  1153:VectorRegisterUpdate
                    // BDD node 62:vector_return
                    meta.reg_incr0 = (bit<32>)(meta.pkt_len);
                    bit<32> reg_new1 = vector_register_1074184288_0_add_value_1153.execute(meta.vector_reg_value0);
                    // EP node  1504:Forward
                    // BDD node 66:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  }
                } else {
                  // EP node  173:Else
                  // BDD node 49:if
                  // EP node  3153:ParserCondition
                  // BDD node 67:if
                  if (meta.pc_67 == 1) {
                    // EP node  3154:Then
                    // BDD node 67:if
                    // EP node  3358:ParserCondition
                    // BDD node 68:if
                    if (meta.pc_68 == 1) {
                      // EP node  3359:Then
                      // BDD node 68:if
                      // EP node  3535:ParserExtraction
                      // BDD node 69:packet_borrow_next_chunk
                      if(hdr.hdr3.isValid()) {
                        // EP node  3848:If
                        // BDD node 70:if
                        if ((32w0x00000000) != (((bit<32>)(hdr.hdr3.data0[15:8])) & (32w0x00000080))){
                          // EP node  3849:Then
                          // BDD node 70:if
                          // EP node  4043:DnsGetResponse
                          // BDD node 71:dns_get_response
                          meta.dns_address = hdr.dns_a_ip.address;
                          dns_name_build.apply();
                          // EP node  4437:If
                          // BDD node 72:if
                          if ((32w0x00000000) == (meta.dns_response_found)){
                            // EP node  4438:Then
                            // BDD node 72:if
                            // EP node  23015:Forward
                            // BDD node 77:FORWARD
                            nf_dev[15:0] = meta.dev[15:0];
                          } else {
                            // EP node  4439:Else
                            // BDD node 72:if
                            // EP node  4651:LPMLookup
                            // BDD node 78:lpm_lookup
                            meta.hit1 = lpm_1074082560.apply().hit;
                            // EP node  5030:If
                            // BDD node 79:if
                            if (!meta.hit1){
                              // EP node  5031:Then
                              // BDD node 79:if
                              // EP node  23247:Forward
                              // BDD node 84:FORWARD
                              nf_dev[15:0] = meta.dev[15:0];
                            } else {
                              // EP node  5032:Else
                              // BDD node 79:if
                              // EP node  5210:LPMLookup
                              // BDD node 85:lpm_lookup
                              meta.hit2 = lpm_1074082784.apply().hit;
                              // EP node  5566:If
                              // BDD node 86:if
                              if (!meta.hit2){
                                // EP node  5567:Then
                                // BDD node 86:if
                                // EP node  5763:Ignore
                                // BDD node 87:vector_borrow
                                // EP node  6147:VectorRegisterUpdate
                                // BDD node 88:vector_return
                                bit<32> reg_new2 = vector_register_1074132640_0_add_value_6147.execute(lpm_1074082560_set_value_param0);
                                // EP node  6477:FCFSCachedTableReadInsert
                                // BDD node 89:map_get
                                meta.fcfs_ct_1074083024_key_32b_0 = hdr.hdr1.data4;
                                meta.fcfs_ct_1074083024_key_32b_1 = meta.dns_address;
                                meta.hit3 = fcfs_ct_1074083024_table_89.apply().hit;
                                bit<32> cached_insert_success0 = 0;
                                if (!meta.hit3) {
                                  fcfs_ct_1074083024_hash_89_calc();
                                  bool fcfs_ct_is_alive1 = fcfs_ct_1074083024_reg_liveness_query_and_refresh_timestamp.execute(fcfs_ct_1074083024_table_89_get_value_param0);
                                  if (fcfs_ct_is_alive1) {
                                    fcfs_ct_1074083024_check_key_0_89();
                                    fcfs_ct_1074083024_check_key_1_89();
                                    if (match_counter1 == 2) {
                                      meta.hit3 = true;
                                    }
                                    else {
                                      punt_gate_64770();
                                    }
                                  } else {
                                    fcfs_ct_1074083024_reg_key_0_write.execute(fcfs_ct_1074083024_hash_89_value);
                                    fcfs_ct_1074083024_reg_key_1_write.execute(fcfs_ct_1074083024_hash_89_value);
                                    cached_insert_success0 = 1;
                                  }
                                }
                                // EP node  6478:If
                                // BDD node 89:map_get
                                if (meta.hit3){
                                  // EP node  6479:Then
                                  // BDD node 89:map_get
                                  // EP node  7438:Ignore
                                  // BDD node 110:dchain_rejuvenate_index
                                  // EP node  7674:Ignore
                                  // BDD node 111:vector_borrow
                                  // EP node  7983:VectorRegisterUpdate
                                  // BDD node 112:vector_return
                                  meta.regexec_vector_register_1074115080_0_write_7983_index0 = fcfs_ct_1074083024_table_89_get_value_param0;
                                  regexec_vector_register_1074115080_0_write_7983();
                                  // EP node  9268:Forward
                                  // BDD node 117:FORWARD
                                  nf_dev[15:0] = meta.dev[15:0];
                                } else {
                                  // EP node  6480:Else
                                  // BDD node 89:map_get
                                  // EP node  6481:If
                                  // BDD node 89:map_get
                                  if ((cached_insert_success0) != (32w0x00000000)){
                                    // EP node  6482:Then
                                    // BDD node 89:map_get
                                    // EP node  9553:Ignore
                                    // BDD node 103:vector_borrow
                                    // EP node  10010:VectorRegisterUpdate
                                    // BDD node 104:vector_return
                                    meta.regexec_vector_register_1074115080_0_write_10010_index0 = fcfs_ct_1074083024_table_89_get_value_param0;
                                    regexec_vector_register_1074115080_0_write_10010();
                                    // EP node  11891:Forward
                                    // BDD node 109:FORWARD
                                    nf_dev[15:0] = meta.dev[15:0];
                                  } else {
                                    // EP node  6483:Else
                                    // BDD node 89:map_get
                                    // EP node  9446:SendToController
                                    // BDD node 291:tofino_force_send_to_controller
                                    if (punt_allowed0) {
                                      fwd_op = fwd_op_t.FORWARD_TO_CPU;
                                      build_cpu_hdr(0);
                                      hdr.cpu.time = meta.time;
                                      hdr.cpu.cached_insert_success0 = cached_insert_success0;
                                      hdr.cpu.dns_response_found = meta.dns_response_found;
                                      hdr.cpu.dns_address = meta.dns_address;
                                      hdr.cpu.dev = meta.dev;
                                    } else {
                                      fwd_op = fwd_op_t.DROP;
                                    }
                                  }
                                }
                              } else {
                                // EP node  5568:Else
                                // BDD node 86:if
                                // EP node  23481:Forward
                                // BDD node 122:FORWARD
                                nf_dev[15:0] = meta.dev[15:0];
                              }
                            }
                          }
                        } else {
                          // EP node  3850:Else
                          // BDD node 70:if
                          // EP node  22785:Forward
                          // BDD node 127:FORWARD
                          nf_dev[15:0] = meta.dev[15:0];
                        }
                      }
                    } else {
                      // EP node  3360:Else
                      // BDD node 68:if
                      // EP node  21131:Forward
                      // BDD node 131:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                    }
                  } else {
                    // EP node  3155:Else
                    // BDD node 67:if
                    // EP node  20905:Forward
                    // BDD node 135:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  }
                }
              }
            } else {
              // EP node  60:Else
              // BDD node 47:if
              // EP node  1556:FCFSCachedTableRead
              // BDD node 136:map_get
              meta.fcfs_ct_1074083024_key_32b_0 = hdr.hdr1.data4;
              meta.fcfs_ct_1074083024_key_32b_1 = hdr.hdr1.data3;
              meta.hit4 = fcfs_ct_1074083024_table_136.apply().hit;
              if (!meta.hit4) {
                fcfs_ct_1074083024_hash_136_calc();
                bool fcfs_ct_is_alive2 = fcfs_ct_1074083024_reg_liveness_query_timestamp.execute(fcfs_ct_1074083024_table_136_get_value_param0);
                if (fcfs_ct_is_alive2) {
                  fcfs_ct_1074083024_check_key_0_136();
                  fcfs_ct_1074083024_check_key_1_136();
                  if (match_counter2 == 2) {
                    meta.hit4 = true;
                  }
                }
              }
              // EP node  1665:If
              // BDD node 137:if
              if (!meta.hit4){
                // EP node  1666:Then
                // BDD node 137:if
                // EP node  18399:Forward
                // BDD node 140:FORWARD
                nf_dev[15:0] = meta.dev[15:0];
              } else {
                // EP node  1667:Else
                // BDD node 137:if
                // EP node  1756:Ignore
                // BDD node 141:dchain_rejuvenate_index
                // EP node  1854:VectorRegisterLookup
                // BDD node 142:vector_borrow
                meta.regexec_vector_register_1074115080_0_read_1854_index0 = fcfs_ct_1074083024_table_136_get_value_param0;
                regexec_vector_register_1074115080_0_read_1854();
                // EP node  2017:Ignore
                // BDD node 143:vector_return
                // EP node  2153:Ignore
                // BDD node 144:vector_borrow
                // EP node  2359:VectorRegisterUpdate
                // BDD node 145:vector_return
                bit<32> reg_new3 = vector_register_1074167072_0_add_value_2359.execute(meta.vector_reg_value1);
                // EP node  2469:Ignore
                // BDD node 146:vector_borrow
                // EP node  2617:VectorRegisterUpdate
                // BDD node 147:vector_return
                meta.reg_incr1 = (bit<32>)(meta.pkt_len);
                bit<32> reg_new4 = vector_register_1074184288_0_add_value_2617.execute(meta.vector_reg_value1);
                // EP node  3003:Forward
                // BDD node 150:FORWARD
                nf_dev[15:0] = meta.dev[15:0];
              }
            }
          }
        } else {
          // EP node  15:Else
          // BDD node 45:if
          // EP node  15127:Forward
          // BDD node 152:FORWARD
          nf_dev[15:0] = meta.dev[15:0];
        }
      }

    }

    forwarding_tbl.apply();
    if (meta.leaving != 0) {
      hdr.recirc.setInvalid();
    }
    if (meta.leaving == 0) {
      hdr.cpu.setInvalid();
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


  apply {

  }
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
