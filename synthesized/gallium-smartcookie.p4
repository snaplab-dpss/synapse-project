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
  bit<32> time; // The ingress clock at the hand-off, the controller's now for the packet.
  bit<32> bf_1073927040_estimate;
  bit<32> dev;
  bit<32> vector_reg_value0;
  bit<64> op_lshr_457_out;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;


};

// The computed values' home: slots reused by live range. A real header -- valid from the start,
// carried with the packet across every pass, dropped where it leaves -- so its fields are exact
// containers: the allocator then keeps each one's slices in one container, as it does for the
// ground truth's recirc_state, instead of spreading them and running out of PHV sources.
header state_h {
  bit<32> s32_1;
  bit<32> s32_2;
  bit<32> s32_3;
  bit<32> s32_4;
  bit<32> s32_6;
  bit<32> s32_7;
}

// The chain's temporaries: header fields for the same reason, but nothing crosses a cut in
// them, so the header is valid inside a pass only and invalidated before every deparser: it
// never travels with the packet, whose bytes on the recirculation ports are the throughput.
header scratch_h {
  bit<32> s32_0;
  bit<32> s32_5;
  bit<32> s32_8;
}



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
  bit<32> data2;
  bit<16> data3;
  bit<16> data4;
  bit<32> data5;
}
header hdr3_h {
  bit<16> data0;
  bit<16> data1;
  bit<32> data2;
}
header hdr4_h {
  bit<32> data0;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  scratch_h sc;
  state_h st;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;
  hdr4_h hdr4;

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
  bit<1> shared_run_0;
  bit<32> bf_1073927040_estimate;
  bit<32> vector_reg_value0;
  bit<32> op_lshr_457_out;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  scratch_h sc;
  state_h st;


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
    pkt.extract(hdr.st);

    transition accept;
  }

  state parse_recirc {
    pkt.extract(hdr.recirc);
    pkt.extract(hdr.st);

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
    transition parser_3;
  }
  state parser_3 {
    transition parser_3_0;
  }
  state parser_3_0 {
    transition select (hdr.hdr0.data3) {
      16w0x0800: parser_4;
      default: parser_230;
    }
  }
  state parser_4 {
    pkt.extract(hdr.hdr1);
    transition parser_5;
  }
  state parser_230 {
    transition reject;
  }
  state parser_5 {
    transition parser_5_0;
  }
  state parser_5_0 {
    transition select (hdr.hdr1.data2[23:0][23:16]) {
      8w0x11: parser_201;
      default: parser_6;
    }
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr1.data2[23:0][23:16]) {
      8w0x06: parser_10;
      default: parser_9;
    }
  }
  state parser_201 {
    transition parser_202;
  }
  state parser_9 {
    transition accept;
  }
  state parser_10 {
    transition parser_11;
  }
  state parser_202 {
    pkt.extract(hdr.hdr3);
    transition parser_204;
  }
  state parser_11 {
    pkt.extract(hdr.hdr2);
    transition parser_192;
  }
  state parser_204 {
    transition parser_204_0;
  }
  state parser_204_0 {
    transition select (hdr.hdr3.data1) {
      16w0x15b3: parser_205;
      default: parser_221;
    }
  }
  state parser_192 {
    transition accept;
  }
  state parser_205 {
    transition parser_210;
  }
  state parser_221 {
    transition accept;
  }
  state parser_210 {
    pkt.extract(hdr.hdr4);
    transition parser_217;
  }
  state parser_217 {
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

  Register<bit<1>,_>(1048576, 0) bf_1073927040_row_0;
  Register<bit<1>,_>(1048576, 0) bf_1073927040_row_1;

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_0_583;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_1_583;

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073927040_row_0) bf_1073927040_row_0_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073927040_row_0_read_value;
  action bf_1073927040_row_0_read_execute() {
    bf_1073927040_row_0_read_value = bf_1073927040_row_0_read.execute(bf_1073927040_hash_0_583.get({
      hdr.hdr1.data3,
      hdr.hdr1.data4,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0xfbc31fc7
    }));
    meta.bf_1073927040_estimate[0:0] = bf_1073927040_row_0_read_value[0:0];
  }

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073927040_row_1) bf_1073927040_row_1_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073927040_row_1_read_value;
  action bf_1073927040_row_1_read_execute() {
    bf_1073927040_row_1_read_value = bf_1073927040_row_1_read.execute(bf_1073927040_hash_1_583.get({
      hdr.hdr1.data3,
      hdr.hdr1.data4,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0x2681580b
    }));
    meta.bf_1073927040_estimate[1:1] = bf_1073927040_row_1_read_value[0:0];
  }

  Register<bit<32>,_>(1, 0) vector_register_1073939616_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073939616_0) vector_register_1073939616_0_read_1682 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action compute_rotate_left_107() {
    hdr.sc.s32_0 = 32w2225785509;
  }

  action compute_rotate_left_108_x() {
    hdr.st.s32_1 = (32w0x3b355c4b) ^ (hdr.hdr1.data3);
  }

  action compute_rotate_left_108_x_k() {
    hdr.st.s32_2 = 32w2862247782;
  }

  action compute_rotate_left_108() {
    hdr.st.s32_3 = hdr.st.s32_1[23:0] ++ hdr.st.s32_1[31:24];
    hdr.sc.s32_0 = (32w0x6f66aa9a) ^ (hdr.sc.s32_0);
    hdr.st.s32_4 = 32w0x5d574351 + hdr.st.s32_1;
  }

  action compute_rotate_left_110() {
    @in_hash { hdr.sc.s32_5 = hdr.sc.s32_0[18:0] ++ hdr.sc.s32_0[31:19]; }
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_4);
    hdr.st.s32_1 = hdr.st.s32_1 + hdr.sc.s32_0;
  }

  action compute_rotate_left_111() {
    @in_hash { hdr.sc.s32_0 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_2 = hdr.st.s32_2 + hdr.st.s32_3;
  }

  action compute_rotate_left_111_k() {
    hdr.st.s32_1 = (32w0x5d574351) + (hdr.st.s32_1);
  }

  action compute_rotate_left_112() {
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_4 = hdr.sc.s32_5 ^ hdr.st.s32_1;
    hdr.st.s32_3 = hdr.sc.s32_0 ^ hdr.st.s32_2;
  }

  action compute_rotate_left_113() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.sc.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_2 = (hdr.st.s32_2) + (hdr.st.s32_4);
    hdr.st.s32_6 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_115() {
    hdr.st.s32_4 = hdr.st.s32_2[15:0] ++ hdr.st.s32_2[31:16];
    hdr.st.s32_3 = (hdr.st.s32_7) ^ (hdr.st.s32_2);
    hdr.sc.s32_0 = (hdr.sc.s32_8) ^ (hdr.st.s32_6);
  }

  action compute_rotate_left_116() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_3[18:0] ++ hdr.st.s32_3[31:19]; }
  }

  action compute_rotate_left_117() {
    @in_hash { hdr.sc.s32_8 = hdr.sc.s32_0[24:0] ++ hdr.sc.s32_0[31:25]; }
    hdr.st.s32_1 = (hdr.st.s32_6) + (hdr.st.s32_3);
    hdr.st.s32_2 = hdr.st.s32_4 + hdr.sc.s32_0;
  }

  action compute_rotate_left_118() {
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_3 = hdr.sc.s32_8 ^ hdr.st.s32_2;
    hdr.st.s32_4 = hdr.st.s32_7 ^ hdr.st.s32_1;
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_0_54319;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_1_54319;

  RegisterAction<bit<1>, bit<20>, void>(bf_1073927040_row_0) bf_1073927040_row_0_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073927040_row_0_set_to_one_execute() {
    bf_1073927040_row_0_set_to_one.execute(bf_1073927040_hash_0_54319.get({
      hdr.hdr1.data3,
      hdr.hdr1.data4,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0xfbc31fc7
    }));
  }

  RegisterAction<bit<1>, bit<20>, void>(bf_1073927040_row_1) bf_1073927040_row_1_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073927040_row_1_set_to_one_execute() {
    bf_1073927040_row_1_set_to_one.execute(bf_1073927040_hash_1_54319.get({
      hdr.hdr1.data3,
      hdr.hdr1.data4,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0x2681580b
    }));
  }

  action compute_op_lshr_457() {
    meta.op_lshr_457_out = meta.time;
  }


  apply {
    meta.shared_run_0 = 0;
    hdr.sc.setValid();
    hdr.st.setValid();

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:ParserExtraction
      // BDD node 2:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  5:ParserCondition
        // BDD node 3:if
        // EP node  6:Then
        // BDD node 3:if
        // EP node  16:ParserExtraction
        // BDD node 4:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  38:ParserCondition
          // BDD node 5:if
          // EP node  39:Then
          // BDD node 5:if
          // EP node  74:ParserCondition
          // BDD node 6:if
          // EP node  75:Then
          // BDD node 6:if
          // EP node  61251:Forward
          // BDD node 9:FORWARD
          @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:24]); }
          // EP node  76:Else
          // BDD node 6:if
          // EP node  122:ParserCondition
          // BDD node 10:if
          // EP node  123:Then
          // BDD node 10:if
          // EP node  168:ParserExtraction
          // BDD node 11:packet_borrow_next_chunk
          if(hdr.hdr2.isValid()) {
            // EP node  276:If
            // BDD node 12:if
            if ((16w0x0000) != (meta.dev[15:0])){
              // EP node  277:Then
              // BDD node 12:if
              // EP node  389:If
              // BDD node 13:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000002))){
                // EP node  390:Then
                // BDD node 13:if
                // EP node  583:BloomFilterQuery
                // BDD node 14:bf_query
                meta.bf_1073927040_estimate = 0;
                bf_1073927040_row_0_read_execute();
                bf_1073927040_row_1_read_execute();
                // EP node  856:If
                // BDD node 15:if
                if ((32w0x00000000) == (meta.bf_1073927040_estimate)){
                  // EP node  857:Then
                  // BDD node 15:if
                  // EP node  28656:RotateLeft
                  // BDD node 16:rotate_left
                  meta.shared_run_0 = 1;
                  // EP node  29472:RotateLeft
                  // BDD node 17:rotate_left
                  // EP node  30225:RotateLeft
                  // BDD node 18:rotate_left
                  // EP node  30913:RotateLeft
                  // BDD node 19:rotate_left
                  // EP node  31609:ArithmeticOp
                  // BDD node 231:op_xor
                  // EP node  32238:ArithmeticOp
                  // BDD node 232:op_add
                  // EP node  32953:RotateLeft
                  // BDD node 20:rotate_left
                  // EP node  33676:ArithmeticOp
                  // BDD node 233:op_xor
                  // EP node  34329:ArithmeticOp
                  // BDD node 234:op_add
                  // EP node  35071:RotateLeft
                  // BDD node 21:rotate_left
                  // EP node  35740:ArithmeticOp
                  // BDD node 235:op_add
                  // EP node  36500:ArithmeticOp
                  // BDD node 236:op_xor
                  // EP node  37269:ArithmeticOp
                  // BDD node 237:op_add
                  // EP node  38132:ArithmeticOp
                  // BDD node 238:op_xor
                  // EP node  39091:ArithmeticOp
                  // BDD node 240:op_xor
                  // EP node  40061:RotateLeft
                  // BDD node 22:rotate_left
                  // EP node  41041:RotateLeft
                  // BDD node 23:rotate_left
                  // EP node  41855:RotateLeft
                  // BDD node 24:rotate_left
                  // EP node  42588:ArithmeticOp
                  // BDD node 239:op_add
                  // EP node  43420:RotateLeft
                  // BDD node 25:rotate_left
                  // EP node  44260:ArithmeticOp
                  // BDD node 241:op_add
                  // EP node  45110:RotateLeft
                  // BDD node 26:rotate_left
                  // EP node  45968:ArithmeticOp
                  // BDD node 242:op_xor
                  // EP node  46836:RotateLeft
                  // BDD node 27:rotate_left
                  // EP node  47617:ArithmeticOp
                  // BDD node 243:op_add
                  // EP node  48503:ArithmeticOp
                  // BDD node 244:op_xor
                  // EP node  49398:ArithmeticOp
                  // BDD node 245:op_add
                  // EP node  50401:ArithmeticOp
                  // BDD node 246:op_xor
                  // EP node  51514:ArithmeticOp
                  // BDD node 248:op_xor
                  // EP node  52548:SendToController
                  // BDD node 247:op_xor
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(1);
                  hdr.cpu.time = meta.time;
                  hdr.cpu.bf_1073927040_estimate = meta.bf_1073927040_estimate;
                } else {
                  // EP node  858:Else
                  // BDD node 15:if
                  // EP node  1204:Forward
                  // BDD node 103:FORWARD
                  nf_dev[15:0] = 16w0x0000;
                }
              } else {
                // EP node  391:Else
                // BDD node 13:if
                // EP node  1441:If
                // BDD node 104:if
                if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000010))){
                  // EP node  1442:Then
                  // BDD node 104:if
                  // EP node  1682:VectorRegisterLookup
                  // BDD node 105:vector_borrow
                  meta.vector_reg_value0 = vector_register_1073939616_0_read_1682.execute(32w0x00000000);
                  // EP node  2205:Ignore
                  // BDD node 106:vector_return
                  // EP node  2528:RotateLeft
                  // BDD node 107:rotate_left
                  meta.shared_run_0 = 1;
                  // EP node  2862:RotateLeft
                  // BDD node 108:rotate_left
                  // EP node  3173:RotateLeft
                  // BDD node 109:rotate_left
                  // EP node  3459:RotateLeft
                  // BDD node 110:rotate_left
                  // EP node  3752:ArithmeticOp
                  // BDD node 345:op_xor
                  // EP node  4018:ArithmeticOp
                  // BDD node 346:op_add
                  // EP node  4328:RotateLeft
                  // BDD node 111:rotate_left
                  // EP node  4645:ArithmeticOp
                  // BDD node 347:op_xor
                  // EP node  4932:ArithmeticOp
                  // BDD node 348:op_add
                  // EP node  5266:RotateLeft
                  // BDD node 112:rotate_left
                  // EP node  5567:ArithmeticOp
                  // BDD node 349:op_add
                  // EP node  5917:ArithmeticOp
                  // BDD node 350:op_xor
                  // EP node  6275:ArithmeticOp
                  // BDD node 351:op_add
                  // EP node  6685:ArithmeticOp
                  // BDD node 352:op_xor
                  // EP node  7149:ArithmeticOp
                  // BDD node 354:op_xor
                  // EP node  7623:RotateLeft
                  // BDD node 113:rotate_left
                  // EP node  8106:RotateLeft
                  // BDD node 114:rotate_left
                  // EP node  8504:RotateLeft
                  // BDD node 115:rotate_left
                  // EP node  8861:ArithmeticOp
                  // BDD node 353:op_add
                  // EP node  9275:RotateLeft
                  // BDD node 116:rotate_left
                  // EP node  9696:ArithmeticOp
                  // BDD node 355:op_add
                  // EP node  10126:RotateLeft
                  // BDD node 117:rotate_left
                  // EP node  10563:ArithmeticOp
                  // BDD node 356:op_xor
                  // EP node  11009:RotateLeft
                  // BDD node 118:rotate_left
                  // EP node  11408:ArithmeticOp
                  // BDD node 357:op_add
                  // EP node  11870:ArithmeticOp
                  // BDD node 358:op_xor
                  // EP node  12340:ArithmeticOp
                  // BDD node 359:op_add
                  // EP node  12876:ArithmeticOp
                  // BDD node 360:op_xor
                  // EP node  13480:ArithmeticOp
                  // BDD node 362:op_xor
                  // EP node  14045:SendToController
                  // BDD node 361:op_xor
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(0);
                  hdr.cpu.time = meta.time;
                  hdr.cpu.dev = meta.dev;
                  hdr.cpu.vector_reg_value0 = meta.vector_reg_value0;
                } else {
                  // EP node  1443:Else
                  // BDD node 104:if
                  // EP node  63368:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
              if (meta.shared_run_0 == 1) {
                compute_rotate_left_107();
                compute_rotate_left_108_x();
                compute_rotate_left_108_x_k();
                compute_rotate_left_108();
                compute_rotate_left_110();
                compute_rotate_left_111();
                compute_rotate_left_111_k();
                compute_rotate_left_112();
                compute_rotate_left_113();
                compute_rotate_left_115();
                compute_rotate_left_116();
                compute_rotate_left_117();
                compute_rotate_left_118();
              }
            } else {
              // EP node  278:Else
              // BDD node 12:if
              // EP node  52776:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000040))){
                // EP node  52777:Then
                // BDD node 188:if
                // EP node  54200:Forward
                // BDD node 192:FORWARD
                @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:24]); }
              } else {
                // EP node  52778:Else
                // BDD node 188:if
                // EP node  54319:BloomFilterSet
                // BDD node 193:bf_set
                bf_1073927040_row_0_set_to_one_execute();
                bf_1073927040_row_1_set_to_one_execute();
                // EP node  55888:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  124:Else
          // BDD node 10:if
          // EP node  61509:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  56607:ParserCondition
          // BDD node 201:if
          // EP node  56608:Then
          // BDD node 201:if
          // EP node  58344:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  60480:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev[15:0])){
              // EP node  60481:Then
              // BDD node 203:if
              // EP node  62298:ParserCondition
              // BDD node 204:if
              // EP node  62299:Then
              // BDD node 204:if
              // EP node  63772:ParserCondition
              // BDD node 205:if
              // EP node  63773:Then
              // BDD node 205:if
              // EP node  69930:Forward
              // BDD node 209:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:24]); }
              // EP node  63774:Else
              // BDD node 205:if
              // EP node  65573:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  67386:Ignore
                // BDD node 211:vector_borrow
                // EP node  69215:ArithmeticOp
                // BDD node 457:op_lshr
                compute_op_lshr_457();
                // EP node  70222:SendToController
                // BDD node 212:vector_return
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(2);
                hdr.cpu.time = meta.time;
                hdr.cpu.op_lshr_457_out = (bit<64>)meta.op_lshr_457_out;
              }
              // EP node  62300:Else
              // BDD node 204:if
              // EP node  69644:Forward
              // BDD node 221:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:24]); }
            } else {
              // EP node  60482:Else
              // BDD node 203:if
              // EP node  68508:Forward
              // BDD node 225:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:24]); }
            }
          }
          // EP node  56609:Else
          // BDD node 201:if
          // EP node  63100:Forward
          // BDD node 228:FORWARD
          @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:24]); }
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  58096:ParserReject
        // BDD node 230:DROP
      }

    }

    forwarding_tbl.apply();
    if (meta.leaving != 0) {
      hdr.recirc.setInvalid();
    }
    if (meta.leaving == 1) {
      hdr.st.setInvalid();
    }

    ig_tm_md.bypass_egress = 1;
    hdr.sc.setInvalid();

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
