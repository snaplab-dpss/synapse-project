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
  bit<32> s32_5;
  bit<32> s32_6;
}

// The chain's temporaries: header fields for the same reason, but nothing crosses a cut in
// them, so the header is valid inside a pass only and invalidated before every deparser: it
// never travels with the packet, whose bytes on the recirculation ports are the throughput.
header scratch_h {
  bit<32> s32_0;
  bit<32> s32_8;
  bit<32> s32_10;
  bit<32> s32_7;
  bit<32> s32_9;
}

header egress_state_h {
  bit<16> code_path;
  bit<32> time; // The ingress clock, ingress_mac_tstamp[47:16]: the packet's time in the egress too.
  bit<32> e32_0;
  bit<32> e32_1;
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
  bit<8> data2;
  bit<8> data3;
  bit<16> data4;
  bit<32> data5;
  bit<32> data6;
}
header hdr2_h {
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<32> data3;
  bit<16> data4;
  bit<16> data5;
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
  egress_state_h egress_state;
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
  bit<1> shared_run_1;
  bit<1> shared_run_2;
  bit<1> shared_run_3;
  bit<32> bf_1073927040_estimate;
  bit<8> bf_1073927040_row_hits;
  bit<32> key_32b_0;
  bit<32> op_lshr_457_out;
  bit<1> to_egress;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  egress_state_h egress_state;
  scratch_h sc;
  state_h st;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

}

struct synapse_egress_metadata_t {
  bit<32> op_sub_333_out;
  bit<32> op_lshr_334_out;
  bit<32> op_add_335_out;
  bit<32> op_xor_453_out;
  bit<32> op_xor_454_out;
  bit<32> op_xor_456_out;
  bit<32> op_xor_344_out;
  bit<32> cond_operand_90_0_out;
  bit<32> hdr_val0;
  bit<8> hdr_val1;
  bit<32> op_lshr_446_out;
  bit<32> op_sub_447_out;
  bit<32> op_lshr_448_out;
  bit<32> hdr_val2;
  bit<32> hdr_val3;
  bit<8> hdr_val4;
  bit<1> redo_checksum;
  bit<16> l4_len;

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
    pkt.extract(hdr.egress_state);
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
    transition select (hdr.hdr1.data3) {
      8w0x11: parser_201;
      default: parser_6;
    }
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr1.data3) {
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

  Register<bit<1>,_>(1048576, 0) bf_1073927040_row_0;
  Register<bit<1>,_>(1048576, 0) bf_1073927040_row_1;

  action bf_1073927040_all_rows_hit() {
    meta.bf_1073927040_estimate = 1;
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_0_583;
  CRCPolynomial<bit<32>>(32w0x7b17a39f, true, false, false, 32w0xffffffff, 32w0xffffffff) bf_1073927040_hash_1_583_poly; // p1
  Hash<bit<20>>(HashAlgorithm_t.CUSTOM, bf_1073927040_hash_1_583_poly) bf_1073927040_hash_1_583;

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073927040_row_0) bf_1073927040_row_0_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073927040_row_0_read_value;
  action bf_1073927040_row_0_read_execute() {
    bf_1073927040_row_0_read_value = bf_1073927040_row_0_read.execute(bf_1073927040_hash_0_583.get({
      hdr.hdr1.data5,
      hdr.hdr1.data6,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0]
    }));
    meta.bf_1073927040_row_hits[0:0] = bf_1073927040_row_0_read_value[0:0];
  }

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073927040_row_1) bf_1073927040_row_1_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073927040_row_1_read_value;
  action bf_1073927040_row_1_read_execute() {
    bf_1073927040_row_1_read_value = bf_1073927040_row_1_read.execute(bf_1073927040_hash_1_583.get({
      hdr.hdr1.data5,
      hdr.hdr1.data6,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0]
    }));
    meta.bf_1073927040_row_hits[1:1] = bf_1073927040_row_1_read_value[0:0];
  }

  bit<32> vector_table_1073939616_88_get_value_param0 = 32w0;
  action vector_table_1073939616_88_get_value(bit<32> _vector_table_1073939616_88_get_value_param0) {
    vector_table_1073939616_88_get_value_param0 = _vector_table_1073939616_88_get_value_param0;
  }

  table vector_table_1073939616_88 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1073939616_88_get_value;
    }
    size = 2;
  }

  action compute_op_xor_303() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.st.s32_1;
  }

  bit<32> vector_table_1073939616_105_get_value_param0 = 32w0;
  action vector_table_1073939616_105_get_value(bit<32> _vector_table_1073939616_105_get_value_param0) {
    vector_table_1073939616_105_get_value_param0 = _vector_table_1073939616_105_get_value_param0;
  }

  table vector_table_1073939616_105 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1073939616_105_get_value;
    }
    size = 2;
  }

  action compute_rotate_left_107() {
    hdr.sc.s32_0 = 32w2225785509;
  }

  action compute_rotate_left_108_x() {
    hdr.st.s32_1 = (32w0x3b355c4b) ^ (hdr.hdr1.data5);
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
    @in_hash { hdr.st.s32_5 = hdr.sc.s32_0[18:0] ++ hdr.sc.s32_0[31:19]; }
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
    hdr.st.s32_4 = hdr.st.s32_5 ^ hdr.st.s32_1;
    hdr.st.s32_3 = hdr.sc.s32_0 ^ hdr.st.s32_2;
  }

  action compute_rotate_left_113() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.sc.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.sc.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
    hdr.sc.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_115() {
    hdr.sc.s32_0 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_4 = (hdr.sc.s32_7) ^ (hdr.sc.s32_9);
    hdr.st.s32_3 = (hdr.sc.s32_8) ^ (hdr.sc.s32_10);
  }

  action compute_rotate_left_116() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
  }

  action compute_rotate_left_117() {
    @in_hash { hdr.sc.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.sc.s32_9 = (hdr.sc.s32_10) + (hdr.st.s32_4);
    hdr.st.s32_2 = hdr.sc.s32_0 + hdr.st.s32_3;
  }

  action compute_rotate_left_118() {
    hdr.st.s32_6 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_3 = hdr.sc.s32_8 ^ hdr.st.s32_2;
    hdr.st.s32_4 = hdr.sc.s32_7 ^ hdr.sc.s32_9;
  }

  action compute_op_shl_379_a() {
    hdr.sc.s32_0 = hdr.hdr2.data0 >> 16;
  }

  action compute_op_shl_379() {
    hdr.st.s32_5 = hdr.sc.s32_0 << 32w0x00000010;
    hdr.st.s32_1 = (bit<32>)(hdr.hdr2.data0[15:0]);
  }

  action compute_op_xor_381() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.hdr1.data6;
  }

  action compute_op_or_380() {
    hdr.st.s32_5 = hdr.st.s32_5 | hdr.st.s32_1;
  }

  action compute_op_xor_384() {
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_5;
  }

  action compute_rotate_left_137() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.sc.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.sc.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
    hdr.sc.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_139() {
    hdr.sc.s32_0 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_4 = (hdr.sc.s32_7) ^ (hdr.sc.s32_9);
    hdr.st.s32_3 = (hdr.sc.s32_8) ^ (hdr.sc.s32_10);
  }

  action compute_rotate_left_140() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
  }

  action compute_rotate_left_141() {
    @in_hash { hdr.sc.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.sc.s32_9 = (hdr.sc.s32_10) + (hdr.st.s32_4);
    hdr.st.s32_2 = hdr.sc.s32_0 + hdr.st.s32_3;
  }

  action compute_rotate_left_142() {
    hdr.st.s32_6 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_3 = hdr.sc.s32_8 ^ hdr.st.s32_2;
    hdr.st.s32_4 = hdr.sc.s32_7 ^ hdr.sc.s32_9;
  }

  action compute_op_xor_416() {
    @in_hash { hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.hdr2.data1; }
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_0_677272;
  CRCPolynomial<bit<32>>(32w0x7b17a39f, true, false, false, 32w0xffffffff, 32w0xffffffff) bf_1073927040_hash_1_677272_poly; // p1
  Hash<bit<20>>(HashAlgorithm_t.CUSTOM, bf_1073927040_hash_1_677272_poly) bf_1073927040_hash_1_677272;

  RegisterAction<bit<1>, bit<20>, void>(bf_1073927040_row_0) bf_1073927040_row_0_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073927040_row_0_set_to_one_execute() {
    bf_1073927040_row_0_set_to_one.execute(bf_1073927040_hash_0_677272.get({
      hdr.hdr1.data5,
      hdr.hdr1.data6,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0]
    }));
  }

  RegisterAction<bit<1>, bit<20>, void>(bf_1073927040_row_1) bf_1073927040_row_1_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073927040_row_1_set_to_one_execute() {
    bf_1073927040_row_1_set_to_one.execute(bf_1073927040_hash_1_677272.get({
      hdr.hdr1.data5,
      hdr.hdr1.data6,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0]
    }));
  }

  action compute_op_lshr_457() {
    meta.op_lshr_457_out = meta.time;
  }


  apply {
    meta.shared_run_1 = 0;
    meta.shared_run_2 = 0;
    meta.shared_run_3 = 0;
    meta.to_egress = 0;
    hdr.sc.setValid();
    hdr.st.setValid();

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  306727:ArithmeticOp
        // BDD node 265:op_shl
        meta.shared_run_3 = 1;
        meta.shared_run_1 = 1;
        meta.shared_run_2 = 1;
        // EP node  309323:ArithmeticOp
        // BDD node 266:op_or
        // EP node  312216:ArithmeticOp
        // BDD node 267:op_xor
        // EP node  315408:ArithmeticOp
        // BDD node 270:op_xor
        // EP node  318611:RotateLeft
        // BDD node 40:rotate_left
        // EP node  321824:RotateLeft
        // BDD node 41:rotate_left
        // EP node  324465:RotateLeft
        // BDD node 42:rotate_left
        // EP node  326822:ArithmeticOp
        // BDD node 269:op_add
        // EP node  329776:RotateLeft
        // BDD node 43:rotate_left
        // EP node  332739:ArithmeticOp
        // BDD node 271:op_add
        // EP node  335713:RotateLeft
        // BDD node 44:rotate_left
        // EP node  338696:ArithmeticOp
        // BDD node 272:op_xor
        // EP node  341690:RotateLeft
        // BDD node 45:rotate_left
        // EP node  344395:ArithmeticOp
        // BDD node 273:op_add
        // EP node  347409:ArithmeticOp
        // BDD node 274:op_xor
        // EP node  350433:ArithmeticOp
        // BDD node 275:op_add
        // EP node  353769:ArithmeticOp
        // BDD node 276:op_xor
        // EP node  357419:ArithmeticOp
        // BDD node 278:op_xor
        // EP node  361081:RotateLeft
        // BDD node 46:rotate_left
        // EP node  364754:RotateLeft
        // BDD node 47:rotate_left
        // EP node  367828:RotateLeft
        // BDD node 48:rotate_left
        // EP node  370605:ArithmeticOp
        // BDD node 277:op_add
        // EP node  373699:RotateLeft
        // BDD node 49:rotate_left
        // EP node  376802:ArithmeticOp
        // BDD node 279:op_add
        // EP node  379916:RotateLeft
        // BDD node 50:rotate_left
        // EP node  383039:ArithmeticOp
        // BDD node 280:op_xor
        // EP node  386173:RotateLeft
        // BDD node 51:rotate_left
        // EP node  389004:ArithmeticOp
        // BDD node 281:op_add
        // EP node  392158:ArithmeticOp
        // BDD node 282:op_xor
        // EP node  395322:ArithmeticOp
        // BDD node 283:op_add
        // EP node  398812:ArithmeticOp
        // BDD node 284:op_xor
        // EP node  401996:ArithmeticOp
        // BDD node 287:op_xor
        // EP node  404553:SendToEgress
        // BDD node 285:op_add
        meta.to_egress = 1;
        hdr.recirc.setValid();
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 1;
        hdr.egress_state.time = meta.time;
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(1);
      } else if (hdr.recirc.code_path == 1) {
        // EP node  482932:ArithmeticOp
        // BDD node 303:op_xor
        compute_op_xor_303();
        meta.shared_run_1 = 1;
        meta.shared_run_2 = 1;
        // EP node  486465:RotateLeft
        // BDD node 64:rotate_left
        // EP node  490007:RotateLeft
        // BDD node 65:rotate_left
        // EP node  492852:RotateLeft
        // BDD node 66:rotate_left
        // EP node  495350:ArithmeticOp
        // BDD node 305:op_add
        // EP node  498211:RotateLeft
        // BDD node 67:rotate_left
        // EP node  501079:ArithmeticOp
        // BDD node 307:op_add
        // EP node  503956:RotateLeft
        // BDD node 68:rotate_left
        // EP node  507200:ArithmeticOp
        // BDD node 308:op_xor
        // EP node  510454:RotateLeft
        // BDD node 69:rotate_left
        // EP node  513356:ArithmeticOp
        // BDD node 309:op_add
        // EP node  516628:ArithmeticOp
        // BDD node 310:op_xor
        // EP node  519909:ArithmeticOp
        // BDD node 311:op_add
        // EP node  523563:ArithmeticOp
        // BDD node 312:op_xor
        // EP node  527592:ArithmeticOp
        // BDD node 314:op_xor
        // EP node  531632:RotateLeft
        // BDD node 70:rotate_left
        // EP node  535682:RotateLeft
        // BDD node 71:rotate_left
        // EP node  539008:RotateLeft
        // BDD node 72:rotate_left
        // EP node  541974:ArithmeticOp
        // BDD node 313:op_add
        // EP node  545318:RotateLeft
        // BDD node 73:rotate_left
        // EP node  548670:ArithmeticOp
        // BDD node 315:op_add
        // EP node  552032:RotateLeft
        // BDD node 74:rotate_left
        // EP node  555402:ArithmeticOp
        // BDD node 316:op_xor
        // EP node  558782:RotateLeft
        // BDD node 75:rotate_left
        // EP node  561796:ArithmeticOp
        // BDD node 317:op_add
        // EP node  565194:ArithmeticOp
        // BDD node 318:op_xor
        // EP node  568601:ArithmeticOp
        // BDD node 319:op_add
        // EP node  572395:ArithmeticOp
        // BDD node 320:op_xor
        // EP node  576578:ArithmeticOp
        // BDD node 322:op_xor
        // EP node  580391:SendToEgress
        // BDD node 76:rotate_left
        meta.to_egress = 1;
        hdr.recirc.setValid();
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 2;
        hdr.egress_state.time = meta.time;
        nf_dev[15:0] = bswap16(16w0x0000);
      } else if (hdr.recirc.code_path == 2) {
        // EP node  28603:ArithmeticOp
        // BDD node 379:op_shl
        meta.shared_run_3 = 1;
        meta.shared_run_1 = 1;
        meta.shared_run_2 = 1;
        // EP node  29174:ArithmeticOp
        // BDD node 380:op_or
        // EP node  29845:ArithmeticOp
        // BDD node 381:op_xor
        // EP node  30618:ArithmeticOp
        // BDD node 384:op_xor
        // EP node  31399:RotateLeft
        // BDD node 131:rotate_left
        // EP node  32187:RotateLeft
        // BDD node 132:rotate_left
        // EP node  32788:RotateLeft
        // BDD node 133:rotate_left
        // EP node  33296:ArithmeticOp
        // BDD node 383:op_add
        // EP node  34010:RotateLeft
        // BDD node 134:rotate_left
        // EP node  34730:ArithmeticOp
        // BDD node 385:op_add
        // EP node  35458:RotateLeft
        // BDD node 135:rotate_left
        // EP node  36192:ArithmeticOp
        // BDD node 386:op_xor
        // EP node  36934:RotateLeft
        // BDD node 136:rotate_left
        // EP node  37578:ArithmeticOp
        // BDD node 387:op_add
        // EP node  38334:ArithmeticOp
        // BDD node 388:op_xor
        // EP node  39097:ArithmeticOp
        // BDD node 389:op_add
        // EP node  39975:ArithmeticOp
        // BDD node 390:op_xor
        // EP node  40970:ArithmeticOp
        // BDD node 392:op_xor
        // EP node  41974:RotateLeft
        // BDD node 137:rotate_left
        // EP node  42986:RotateLeft
        // BDD node 138:rotate_left
        // EP node  43784:RotateLeft
        // BDD node 139:rotate_left
        // EP node  44476:ArithmeticOp
        // BDD node 391:op_add
        // EP node  45288:RotateLeft
        // BDD node 140:rotate_left
        // EP node  46106:ArithmeticOp
        // BDD node 393:op_add
        // EP node  46932:RotateLeft
        // BDD node 141:rotate_left
        // EP node  47764:ArithmeticOp
        // BDD node 394:op_xor
        // EP node  48604:RotateLeft
        // BDD node 142:rotate_left
        // EP node  49332:ArithmeticOp
        // BDD node 395:op_add
        // EP node  50186:ArithmeticOp
        // BDD node 396:op_xor
        // EP node  51047:ArithmeticOp
        // BDD node 397:op_add
        // EP node  52037:ArithmeticOp
        // BDD node 398:op_xor
        // EP node  53158:ArithmeticOp
        // BDD node 400:op_xor
        // EP node  54039:SendToEgress
        // BDD node 399:op_xor
        meta.to_egress = 1;
        hdr.recirc.setValid();
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 4;
        hdr.egress_state.time = meta.time;
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(3);
      } else if (hdr.recirc.code_path == 3) {
        // EP node  77359:ArithmeticOp
        // BDD node 416:op_xor
        compute_op_xor_416();
        meta.shared_run_1 = 1;
        meta.shared_run_2 = 1;
        // EP node  78628:RotateLeft
        // BDD node 155:rotate_left
        // EP node  79904:RotateLeft
        // BDD node 156:rotate_left
        // EP node  80871:RotateLeft
        // BDD node 157:rotate_left
        // EP node  81684:ArithmeticOp
        // BDD node 418:op_add
        // EP node  82663:RotateLeft
        // BDD node 158:rotate_left
        // EP node  83647:ArithmeticOp
        // BDD node 420:op_add
        // EP node  84638:RotateLeft
        // BDD node 159:rotate_left
        // EP node  85799:ArithmeticOp
        // BDD node 421:op_xor
        // EP node  86968:RotateLeft
        // BDD node 160:rotate_left
        // EP node  87978:ArithmeticOp
        // BDD node 422:op_add
        // EP node  89161:ArithmeticOp
        // BDD node 423:op_xor
        // EP node  90351:ArithmeticOp
        // BDD node 424:op_add
        // EP node  91717:ArithmeticOp
        // BDD node 425:op_xor
        // EP node  93261:ArithmeticOp
        // BDD node 427:op_xor
        // EP node  94814:RotateLeft
        // BDD node 161:rotate_left
        // EP node  96375:RotateLeft
        // BDD node 162:rotate_left
        // EP node  97600:RotateLeft
        // BDD node 163:rotate_left
        // EP node  98658:ArithmeticOp
        // BDD node 426:op_add
        // EP node  99897:RotateLeft
        // BDD node 164:rotate_left
        // EP node  101142:ArithmeticOp
        // BDD node 428:op_add
        // EP node  102395:RotateLeft
        // BDD node 165:rotate_left
        // EP node  103654:ArithmeticOp
        // BDD node 429:op_xor
        // EP node  104921:RotateLeft
        // BDD node 166:rotate_left
        // EP node  106015:ArithmeticOp
        // BDD node 430:op_add
        // EP node  107296:ArithmeticOp
        // BDD node 431:op_xor
        // EP node  108584:ArithmeticOp
        // BDD node 432:op_add
        // EP node  110062:ArithmeticOp
        // BDD node 433:op_xor
        // EP node  111732:ArithmeticOp
        // BDD node 435:op_xor
        // EP node  113225:SendToEgress
        // BDD node 167:rotate_left
        meta.to_egress = 1;
        hdr.recirc.setValid();
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 5;
        hdr.egress_state.time = meta.time;
        nf_dev[15:0] = hdr.egress_state.e32_0[15:0];
      }

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
          // EP node  701676:Forward
          // BDD node 9:FORWARD
          @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
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
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[31:16][7:0])) & (32w0x00000002))){
                // EP node  390:Then
                // BDD node 13:if
                // EP node  583:BloomFilterQuery
                // BDD node 14:bf_query
                meta.bf_1073927040_estimate = 0;
                meta.bf_1073927040_row_hits = 0;
                bf_1073927040_row_0_read_execute();
                bf_1073927040_row_1_read_execute();
                if (meta.bf_1073927040_row_hits == 8w0x03) {
                  bf_1073927040_all_rows_hit();
                }
                // EP node  856:If
                // BDD node 15:if
                if ((32w0x00000000) == (meta.bf_1073927040_estimate)){
                  // EP node  857:Then
                  // BDD node 15:if
                  // EP node  148681:VectorTableLookup
                  // BDD node 88:vector_borrow
                  meta.key_32b_0 = 32w0x00000000;
                  vector_table_1073939616_88.apply();
                  // EP node  151619:RotateLeft
                  // BDD node 16:rotate_left
                  compute_rotate_left_107();
                  compute_rotate_left_108_x();
                  compute_rotate_left_108_x_k();
                  compute_rotate_left_108();
                  compute_rotate_left_110();
                  compute_rotate_left_111();
                  compute_rotate_left_111_k();
                  compute_rotate_left_112();
                  meta.shared_run_1 = 1;
                  // EP node  154571:RotateLeft
                  // BDD node 17:rotate_left
                  // EP node  157309:RotateLeft
                  // BDD node 18:rotate_left
                  // EP node  159831:RotateLeft
                  // BDD node 19:rotate_left
                  // EP node  162363:ArithmeticOp
                  // BDD node 231:op_xor
                  // EP node  164677:ArithmeticOp
                  // BDD node 232:op_add
                  // EP node  167232:RotateLeft
                  // BDD node 20:rotate_left
                  // EP node  169797:ArithmeticOp
                  // BDD node 233:op_xor
                  // EP node  172141:ArithmeticOp
                  // BDD node 234:op_add
                  // EP node  174729:RotateLeft
                  // BDD node 21:rotate_left
                  // EP node  177093:ArithmeticOp
                  // BDD node 235:op_add
                  // EP node  179703:ArithmeticOp
                  // BDD node 236:op_xor
                  // EP node  182324:ArithmeticOp
                  // BDD node 237:op_add
                  // EP node  185194:ArithmeticOp
                  // BDD node 238:op_xor
                  // EP node  188315:ArithmeticOp
                  // BDD node 240:op_xor
                  // EP node  191449:RotateLeft
                  // BDD node 22:rotate_left
                  // EP node  194595:RotateLeft
                  // BDD node 23:rotate_left
                  // EP node  197271:RotateLeft
                  // BDD node 24:rotate_left
                  // EP node  199715:ArithmeticOp
                  // BDD node 239:op_add
                  // EP node  202413:RotateLeft
                  // BDD node 25:rotate_left
                  // EP node  205121:ArithmeticOp
                  // BDD node 241:op_add
                  // EP node  207841:RotateLeft
                  // BDD node 26:rotate_left
                  // EP node  210571:ArithmeticOp
                  // BDD node 242:op_xor
                  // EP node  213313:RotateLeft
                  // BDD node 27:rotate_left
                  // EP node  215817:ArithmeticOp
                  // BDD node 243:op_add
                  // EP node  218581:ArithmeticOp
                  // BDD node 244:op_xor
                  // EP node  221356:ArithmeticOp
                  // BDD node 245:op_add
                  // EP node  224394:ArithmeticOp
                  // BDD node 246:op_xor
                  // EP node  227697:ArithmeticOp
                  // BDD node 248:op_xor
                  // EP node  230504:SendToEgress
                  // BDD node 247:op_xor
                  meta.to_egress = 1;
                  hdr.recirc.setValid();
                  hdr.egress_state.setValid();
                  hdr.egress_state.code_path = 0;
                  hdr.egress_state.time = meta.time;
                  hdr.egress_state.e32_0 = vector_table_1073939616_88_get_value_param0;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(0);
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
                if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[31:16][7:0])) & (32w0x00000010))){
                  // EP node  1442:Then
                  // BDD node 104:if
                  // EP node  1892:VectorTableLookup
                  // BDD node 105:vector_borrow
                  meta.key_32b_0 = 32w0x00000000;
                  vector_table_1073939616_105.apply();
                  // EP node  2205:Ignore
                  // BDD node 106:vector_return
                  // EP node  2528:RotateLeft
                  // BDD node 107:rotate_left
                  compute_rotate_left_107();
                  compute_rotate_left_108_x();
                  compute_rotate_left_108_x_k();
                  compute_rotate_left_108();
                  compute_rotate_left_110();
                  compute_rotate_left_111();
                  compute_rotate_left_111_k();
                  compute_rotate_left_112();
                  meta.shared_run_1 = 1;
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
                  // EP node  13973:SendToEgress
                  // BDD node 361:op_xor
                  meta.to_egress = 1;
                  hdr.recirc.setValid();
                  hdr.egress_state.setValid();
                  hdr.egress_state.code_path = 3;
                  hdr.egress_state.time = meta.time;
                  hdr.egress_state.e32_0 = meta.dev;
                  hdr.egress_state.e32_1 = vector_table_1073939616_105_get_value_param0;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(2);
                } else {
                  // EP node  1443:Else
                  // BDD node 104:if
                  // EP node  708785:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  278:Else
              // BDD node 12:if
              // EP node  667444:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[31:16][7:0])) & (32w0x00000040))){
                // EP node  667445:Then
                // BDD node 188:if
                // EP node  672564:Forward
                // BDD node 192:FORWARD
                @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
              } else {
                // EP node  667446:Else
                // BDD node 188:if
                // EP node  677272:BloomFilterSet
                // BDD node 193:bf_set
                bf_1073927040_row_0_set_to_one_execute();
                bf_1073927040_row_1_set_to_one_execute();
                // EP node  682897:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  124:Else
          // BDD node 10:if
          // EP node  702558:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  685488:ParserCondition
          // BDD node 201:if
          // EP node  685489:Then
          // BDD node 201:if
          // EP node  691593:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  699033:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev[15:0])){
              // EP node  699034:Then
              // BDD node 203:if
              // EP node  705219:ParserCondition
              // BDD node 204:if
              // EP node  705220:Then
              // BDD node 204:if
              // EP node  710125:ParserCondition
              // BDD node 205:if
              // EP node  710126:Then
              // BDD node 205:if
              // EP node  729560:Forward
              // BDD node 209:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
              // EP node  710127:Else
              // BDD node 205:if
              // EP node  715982:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  721851:Ignore
                // BDD node 211:vector_borrow
                // EP node  727285:ArithmeticOp
                // BDD node 457:op_lshr
                compute_op_lshr_457();
                // EP node  730476:SendToController
                // BDD node 212:vector_return
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(0);
                hdr.cpu.time = meta.time;
                hdr.cpu.op_lshr_457_out = (bit<64>)meta.op_lshr_457_out;
              }
              // EP node  705221:Else
              // BDD node 204:if
              // EP node  728650:Forward
              // BDD node 221:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
            } else {
              // EP node  699035:Else
              // BDD node 203:if
              // EP node  725018:Forward
              // BDD node 225:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
            }
          }
          // EP node  685490:Else
          // BDD node 201:if
          // EP node  707893:Forward
          // BDD node 228:FORWARD
          @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  690721:ParserReject
        // BDD node 230:DROP
      }

    }
    if (meta.shared_run_3 == 1) {
      compute_op_shl_379_a();
      compute_op_shl_379();
      compute_op_xor_381();
      compute_op_or_380();
      compute_op_xor_384();
    }
    if (meta.shared_run_1 == 1) {
      compute_rotate_left_113();
      compute_rotate_left_115();
      compute_rotate_left_116();
      compute_rotate_left_117();
      compute_rotate_left_118();
    }
    if (meta.shared_run_2 == 1) {
      compute_rotate_left_137();
      compute_rotate_left_139();
      compute_rotate_left_140();
      compute_rotate_left_141();
      compute_rotate_left_142();
    }

    forwarding_tbl.apply();
    if (meta.leaving != 0 && meta.to_egress == 0) {
      hdr.recirc.setInvalid();
      hdr.egress_state.setInvalid();
    }
    if (meta.leaving == 0) {
      hdr.cpu.setInvalid();
    }
    if (meta.leaving == 1 && meta.to_egress == 0) {
      hdr.st.setInvalid();
    }

    if (meta.to_egress == 1) {
      ig_tm_md.bypass_egress = 0;
    } else {
      ig_tm_md.bypass_egress = 1;
    }
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
    eg_md.redo_checksum = 0;
    pkt.extract(hdr.recirc);
    pkt.extract(hdr.egress_state);
    pkt.extract(hdr.st);
    pkt.extract(hdr.hdr0);
    pkt.extract(hdr.hdr1);
    pkt.extract(hdr.hdr2);
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
  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

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

  action compute_op_add_285_b() {
    @in_hash { hdr.sc.s32_0 = hdr.hdr2.data1; }
  }

  action compute_op_add_285() {
    hdr.st.s32_1 = 32w0xffffffff + hdr.sc.s32_0;
  }

  action compute_op_xor_289() {
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_1;
  }

  action compute_op_sub_333() {
    eg_md.op_sub_333_out = hdr.egress_state.time - hdr.egress_state.e32_0;
    eg_md.op_add_335_out = 32w0xffffffff + hdr.hdr2.data2;
  }

  action compute_op_lshr_334() {
    eg_md.op_lshr_334_out = eg_md.op_sub_333_out >> 32w0x0000000c;
  }

  action compute_op_xor_344() {
    eg_md.op_xor_344_out = eg_md.op_add_335_out ^ eg_md.op_xor_456_out;
  }

  action compute_cond_operand_90_0() {
    eg_md.cond_operand_90_0_out = (eg_md.op_lshr_334_out) - (eg_md.op_xor_344_out);
  }

  action hdr_val0_calc() { eg_md.hdr_val0 = (32w0xffffffff) + (hdr.hdr2.data1); }

  action hdr_val1_calc() { eg_md.hdr_val1 = (hdr.hdr2.data3[31:16][7:0]) | (8w0x40); }

  action rewrite_92() {
    hdr.hdr2.data1 = eg_md.hdr_val0;
    hdr.hdr2.data3 = 8w0x50 ++ eg_md.hdr_val1 ++ hdr.hdr2.data3[15:8] ++ hdr.hdr2.data3[7:0];
  }
  action rewrite_93() {
    hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;
  }
  action compute_op_xor_361() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.hdr1.data5;
  }

  action compute_op_xor_364() {
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.hdr1.data6;
  }

  action compute_rotate_left_119() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.sc.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
  }

  action compute_rotate_left_120() {
    hdr.sc.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.sc.s32_0 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_4 = (hdr.sc.s32_7) ^ (hdr.sc.s32_9);
    hdr.sc.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_122() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
    hdr.st.s32_3 = (hdr.sc.s32_8) ^ (hdr.sc.s32_10);
    hdr.sc.s32_9 = (hdr.sc.s32_10) + (hdr.st.s32_4);
  }

  action compute_rotate_left_123() {
    @in_hash { hdr.sc.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_6 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_2 = hdr.sc.s32_0 + hdr.st.s32_3;
    hdr.st.s32_4 = hdr.sc.s32_7 ^ hdr.sc.s32_9;
  }

  action compute_op_xor_372() {
    hdr.st.s32_3 = hdr.sc.s32_8 ^ hdr.st.s32_2;
  }

  action compute_rotate_left_125() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.sc.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
  }

  action compute_rotate_left_126() {
    hdr.sc.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.sc.s32_0 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_4 = (hdr.sc.s32_7) ^ (hdr.sc.s32_9);
    hdr.sc.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_128() {
    @in_hash { hdr.sc.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
    hdr.st.s32_3 = (hdr.sc.s32_8) ^ (hdr.sc.s32_10);
    hdr.sc.s32_9 = (hdr.sc.s32_10) + (hdr.st.s32_4);
  }

  action compute_rotate_left_129() {
    @in_hash { hdr.sc.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_6 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
    hdr.st.s32_2 = hdr.sc.s32_0 + hdr.st.s32_3;
    hdr.st.s32_4 = hdr.sc.s32_7 ^ hdr.sc.s32_9;
  }

  action compute_op_xor_378() {
    hdr.st.s32_3 = hdr.sc.s32_8 ^ hdr.st.s32_2;
  }

  action compute_op_xor_399() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.st.s32_5;
  }

  action compute_op_xor_402() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.hdr2.data1; }
  }

  action compute_op_lshr_446() {
    eg_md.op_lshr_446_out = hdr.egress_state.time;
  }

  action compute_op_sub_447() {
    eg_md.op_sub_447_out = eg_md.op_lshr_446_out[31:0] - hdr.egress_state.e32_1;
  }

  action compute_op_lshr_448() {
    eg_md.op_lshr_448_out = eg_md.op_sub_447_out >> 32w0x0000000c;
  }

  action compute_op_add_450_b() {
    @in_hash { hdr.st.s32_5 = hdr.st.s32_3; }
  }

  action compute_op_xor_456() {
    @in_hash { eg_md.op_xor_456_out = hdr.st.s32_2 ^ hdr.st.s32_4 ^ hdr.st.s32_6 ^ hdr.st.s32_3; }
  }

  action hdr_val2_calc() { eg_md.hdr_val2 = (eg_md.op_lshr_448_out) ^ (eg_md.op_xor_456_out); }

  action hdr_val3_calc() { eg_md.hdr_val3 = (32w0x00000001) + (hdr.hdr2.data1); }

  action hdr_val4_calc() { eg_md.hdr_val4 = (hdr.hdr2.data3[31:16][7:0]) | (8w0x12); }

  action rewrite_180() {
    hdr.hdr2.data0 = hdr.hdr2.data0[15:0] ++ hdr.hdr2.data0[31:16];
    hdr.hdr2.data1 = eg_md.hdr_val2;
    hdr.hdr2.data2 = eg_md.hdr_val3;
    hdr.hdr2.data3 = 8w0x50 ++ eg_md.hdr_val4 ++ hdr.hdr2.data3[15:8] ++ hdr.hdr2.data3[7:0];
  }
  action rewrite_181() {
    swap32(hdr.hdr1.data5, hdr.hdr1.data6);
    hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;
  }
  action compute_rotate_left_128_v0() {
    hdr.st.s32_3 = (hdr.sc.s32_8) ^ (hdr.sc.s32_10);
    hdr.sc.s32_9 = (hdr.sc.s32_10) + (hdr.st.s32_4);
  }

  action compute_rotate_left_129_v0() {
    @in_hash { hdr.sc.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_6 = hdr.sc.s32_9[15:0] ++ hdr.sc.s32_9[31:16];
  }

  action compute_rotate_left_129_v1() {
    hdr.st.s32_2 = hdr.sc.s32_0 + hdr.st.s32_3;
    hdr.st.s32_4 = hdr.sc.s32_7 ^ hdr.sc.s32_9;
  }


  apply {
    hdr.sc.setValid();
    if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4 || hdr.egress_state.code_path == 5) {
      if (hdr.egress_state.code_path == 2) {
        // EP node  581547:RotateLeft
        // BDD node 76:rotate_left
        compute_op_sub_333();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 3) {
        compute_op_xor_361();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 3) {
        compute_op_xor_364();
      }
      if (hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 4) {
        compute_op_xor_399();
      }
      if (hdr.egress_state.code_path == 2) {
        compute_op_lshr_334();
      }
      compute_rotate_left_119();
      if (hdr.egress_state.code_path == 4) {
        compute_op_xor_402();
      }
      if (hdr.egress_state.code_path == 1) {
        compute_op_add_285_b();
        compute_op_add_285();
        compute_op_xor_289();
      }
      compute_rotate_left_120();
      compute_rotate_left_122();
      compute_rotate_left_123();
      compute_op_xor_372();
      compute_rotate_left_125();
      compute_rotate_left_126();
      compute_rotate_left_128();
      if (hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 5) {
        compute_op_add_450_b();
      }
      if (hdr.egress_state.code_path == 5) {
        compute_rotate_left_129_v0();
        // EP node  115106:RotateLeft
        // BDD node 168:rotate_left
        // EP node  116048:RotateLeft
        // BDD node 169:rotate_left
        // EP node  116806:ArithmeticOp
        // BDD node 434:op_add
        // EP node  117758:RotateLeft
        // BDD node 170:rotate_left
        // EP node  118714:ArithmeticOp
        // BDD node 436:op_add
        // EP node  119676:RotateLeft
        // BDD node 171:rotate_left
        // EP node  120642:ArithmeticOp
        // BDD node 437:op_xor
        // EP node  121614:RotateLeft
        // BDD node 172:rotate_left
        // EP node  122396:ArithmeticOp
        // BDD node 438:op_add
        // EP node  123378:ArithmeticOp
        // BDD node 439:op_xor
        // EP node  124365:ArithmeticOp
        // BDD node 440:op_add
        // EP node  125555:ArithmeticOp
        // BDD node 441:op_xor
        // EP node  126950:ArithmeticOp
        // BDD node 443:op_xor
        // EP node  128352:RotateLeft
        // BDD node 173:rotate_left
        // EP node  129760:RotateLeft
        // BDD node 174:rotate_left
        // EP node  130772:RotateLeft
        // BDD node 175:rotate_left
        // EP node  131586:ArithmeticOp
        // BDD node 442:op_add
        // EP node  132608:RotateLeft
        // BDD node 176:rotate_left
        // EP node  133634:ArithmeticOp
        // BDD node 444:op_add
        // EP node  134666:RotateLeft
        // BDD node 177:rotate_left
        // EP node  135702:ArithmeticOp
        // BDD node 445:op_xor
        // EP node  136744:RotateLeft
        // BDD node 178:rotate_left
        // EP node  137791:ChecksumUpdate
        // BDD node 179:nf_set_rte_ipv4_udptcp_checksum
        // EP node  138633:ArithmeticOp
        // BDD node 446:op_lshr
        compute_op_lshr_446();
        compute_op_sub_447();
        compute_op_lshr_448();
        compute_rotate_left_128_v0();
        compute_rotate_left_129_v1();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_rotate_left_129();
      }
      if (hdr.egress_state.code_path == 2) {
      }
      compute_op_xor_378();
      if (hdr.egress_state.code_path == 2) {
      }
      if (hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 5) {
        compute_op_xor_456();
      }
      if (hdr.egress_state.code_path == 5) {
        // EP node  139479:ArithmeticOp
        // BDD node 447:op_sub
        // EP node  140329:ArithmeticOp
        // BDD node 448:op_lshr
        // EP node  140970:ArithmeticOp
        // BDD node 449:op_xor
        // EP node  141614:ArithmeticOp
        // BDD node 450:op_add
        // EP node  142261:ArithmeticOp
        // BDD node 451:op_add
        // EP node  142911:ArithmeticOp
        // BDD node 452:op_xor
        // EP node  143564:ArithmeticOp
        // BDD node 453:op_xor
        // EP node  144220:ArithmeticOp
        // BDD node 454:op_xor
        // EP node  144660:ArithmeticOp
        // BDD node 455:op_xor
        // EP node  145102:ArithmeticOp
        // BDD node 456:op_xor
        // EP node  145546:ModifyHeader
        // BDD node 180:packet_return_chunk
        hdr_val2_calc();
        hdr_val3_calc();
        hdr_val4_calc();
        rewrite_180();
        // EP node  145992:ModifyHeader
        // BDD node 181:packet_return_chunk
        rewrite_181();
        // EP node  146887:Forward
        // BDD node 183:FORWARD
        eg_md.redo_checksum = 1;
        eg_md.l4_len = 16w20;
        hdr.recirc.setInvalid();
        hdr.egress_state.setInvalid();
        hdr.st.setInvalid();
      }
      if (hdr.egress_state.code_path == 2) {
        compute_op_xor_344();
        compute_cond_operand_90_0();
        // EP node  584986:RotateLeft
        // BDD node 77:rotate_left
        // EP node  587669:RotateLeft
        // BDD node 78:rotate_left
        // EP node  589975:ArithmeticOp
        // BDD node 321:op_add
        // EP node  592672:RotateLeft
        // BDD node 79:rotate_left
        // EP node  595375:ArithmeticOp
        // BDD node 323:op_add
        // EP node  598086:RotateLeft
        // BDD node 80:rotate_left
        // EP node  600803:ArithmeticOp
        // BDD node 324:op_xor
        // EP node  603528:RotateLeft
        // BDD node 81:rotate_left
        // EP node  605870:ArithmeticOp
        // BDD node 325:op_add
        // EP node  608609:ArithmeticOp
        // BDD node 326:op_xor
        // EP node  611355:ArithmeticOp
        // BDD node 327:op_add
        // EP node  614501:ArithmeticOp
        // BDD node 328:op_xor
        // EP node  618049:ArithmeticOp
        // BDD node 330:op_xor
        // EP node  621606:RotateLeft
        // BDD node 82:rotate_left
        // EP node  625171:RotateLeft
        // BDD node 83:rotate_left
        // EP node  627952:RotateLeft
        // BDD node 84:rotate_left
        // EP node  630342:ArithmeticOp
        // BDD node 329:op_add
        // EP node  633137:RotateLeft
        // BDD node 85:rotate_left
        // EP node  635938:ArithmeticOp
        // BDD node 331:op_add
        // EP node  638747:RotateLeft
        // BDD node 86:rotate_left
        // EP node  641562:ArithmeticOp
        // BDD node 332:op_xor
        // EP node  644385:RotateLeft
        // BDD node 87:rotate_left
        // EP node  646407:Ignore
        // BDD node 89:vector_return
        // EP node  648838:ArithmeticOp
        // BDD node 333:op_sub
        // EP node  650870:ArithmeticOp
        // BDD node 334:op_lshr
        // EP node  652500:ArithmeticOp
        // BDD node 335:op_add
        // EP node  653726:ArithmeticOp
        // BDD node 336:op_xor
        // EP node  654955:ArithmeticOp
        // BDD node 337:op_add
        // EP node  656187:ArithmeticOp
        // BDD node 338:op_add
        // EP node  657422:ArithmeticOp
        // BDD node 339:op_xor
        // EP node  658660:ArithmeticOp
        // BDD node 340:op_xor
        // EP node  659901:ArithmeticOp
        // BDD node 341:op_xor
        // EP node  660731:ArithmeticOp
        // BDD node 342:op_xor
        // EP node  661563:ArithmeticOp
        // BDD node 343:op_xor
        // EP node  662397:ArithmeticOp
        // BDD node 344:op_xor
        // EP node  663233:If
        // BDD node 90:if
        bool cond0 = false;
        if ((eg_md.cond_operand_90_0_out[31:8]) == (24w0x000000)){
          if ((eg_md.cond_operand_90_0_out[7:0]) <= (8w0x02)){
            cond0 = true;
          }
        }
        if (cond0) {
          // EP node  663234:Then
          // BDD node 90:if
          // EP node  673416:ChecksumUpdate
          // BDD node 91:nf_set_rte_ipv4_udptcp_checksum
          // EP node  674270:ModifyHeader
          // BDD node 92:packet_return_chunk
          hdr_val0_calc();
          hdr_val1_calc();
          rewrite_92();
          // EP node  675126:ModifyHeader
          // BDD node 93:packet_return_chunk
          rewrite_93();
          // EP node  676841:Forward
          // BDD node 95:FORWARD
          eg_md.redo_checksum = 1;
          eg_md.l4_len = 16w20;
          hdr.recirc.setInvalid();
          hdr.egress_state.setInvalid();
          hdr.st.setInvalid();
        } else {
          // EP node  663235:Else
          // BDD node 90:if
          // EP node  666600:Drop
          // BDD node 99:DROP
          ig_intr_dprs_md.drop_ctl = 1;
        }
      }
    }
    hdr.sc.setInvalid();

  }
}

control EgressDeparser(
  packet_out pkt,
  inout synapse_egress_headers_t hdr,
  in    synapse_egress_metadata_t eg_md,
  in    egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md
) {
  Checksum() ipv4_checksum;
  Checksum() l4_checksum;

  apply {
    if (eg_md.redo_checksum == 1) {
      hdr.hdr1.data4 = ipv4_checksum.update({hdr.hdr1.data0, hdr.hdr1.data1, hdr.hdr1.data2, hdr.hdr1.data3, hdr.hdr1.data5, hdr.hdr1.data6});
      hdr.hdr2.data4 = l4_checksum.update({hdr.hdr1.data5, hdr.hdr1.data6, 8w0, hdr.hdr1.data3, eg_md.l4_len, hdr.hdr2.data0, hdr.hdr2.data1, hdr.hdr2.data2, hdr.hdr2.data3, hdr.hdr2.data5});
    }

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

