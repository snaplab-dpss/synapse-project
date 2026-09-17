// The same program with 12 state words in the ingress and 11 in the egress
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
  bit<32> s32_0;
  bit<32> s32_1;
  bit<32> s32_2;
  bit<32> s32_3;
  bit<32> s32_4;
  bit<32> s32_5;
  bit<32> s32_6;
  bit<32> s32_7;
  bit<32> s32_8;
  bit<32> s32_9;
  bit<32> s32_10;
  bit<32> s32_11;
}

header egress_state_h {
  bit<16> code_path;
  bit<32> time; // The ingress clock, ingress_mac_tstamp[47:16]: the packet's time in the egress too.
  bit<32> vector_table_1073939616_88_get_value_param0;
  bit<32> dev;
  bit<32> vector_table_1073939616_105_get_value_param0;
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
  bit<32> bf_1073927040_estimate;
  bit<32> key_32b_0;
  bit<1> to_egress;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  egress_state_h egress_state;
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

    transition accept;
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
      hdr.hdr1.data5,
      hdr.hdr1.data6,
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
      hdr.hdr1.data5,
      hdr.hdr1.data6,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0x2681580b
    }));
    meta.bf_1073927040_estimate[1:1] = bf_1073927040_row_1_read_value[0:0];
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

  action compute_op_add_285_b() {
    @in_hash { hdr.st.s32_0 = hdr.hdr2.data1; }
  }

  action compute_op_add_285() {
    hdr.st.s32_11 = 32w0xffffffff + hdr.st.s32_0;
  }

  action compute_op_xor_286() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.st.s32_5;
  }

  action compute_op_xor_289() {
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_11;
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
    hdr.st.s32_0 = 32w2225785509;
  }

  action compute_rotate_left_108_x() {
    hdr.st.s32_1 = (32w0x3b355c4b) ^ (hdr.hdr1.data5);
  }

  action compute_rotate_left_108_x_k() {
    hdr.st.s32_2 = 32w2862247782;
  }

  action compute_rotate_left_108() {
    hdr.st.s32_3 = hdr.st.s32_1[23:0] ++ hdr.st.s32_1[31:24];
    hdr.st.s32_0 = (32w0x6f66aa9a) ^ (hdr.st.s32_0);
    hdr.st.s32_4 = 32w0x5d574351 + hdr.st.s32_1;
  }

  action compute_rotate_left_110() {
    @in_hash { hdr.st.s32_5 = hdr.st.s32_0[18:0] ++ hdr.st.s32_0[31:19]; }
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_4);
    hdr.st.s32_1 = hdr.st.s32_1 + hdr.st.s32_0;
  }

  action compute_rotate_left_111() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_2 = hdr.st.s32_2 + hdr.st.s32_3;
  }

  action compute_rotate_left_111_k() {
    hdr.st.s32_1 = (32w0x5d574351) + (hdr.st.s32_1);
  }

  action compute_rotate_left_112() {
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_4 = hdr.st.s32_5 ^ hdr.st.s32_1;
    hdr.st.s32_3 = hdr.st.s32_0 ^ hdr.st.s32_2;
  }

  action compute_rotate_left_113() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.st.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
    hdr.st.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_115() {
    hdr.st.s32_0 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_4 = (hdr.st.s32_7) ^ (hdr.st.s32_9);
    hdr.st.s32_3 = (hdr.st.s32_8) ^ (hdr.st.s32_10);
  }

  action compute_rotate_left_116() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
  }

  action compute_rotate_left_117() {
    @in_hash { hdr.st.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_9 = (hdr.st.s32_10) + (hdr.st.s32_4);
    hdr.st.s32_2 = hdr.st.s32_0 + hdr.st.s32_3;
  }

  action compute_rotate_left_118() {
    hdr.st.s32_6 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_3 = hdr.st.s32_8 ^ hdr.st.s32_2;
    hdr.st.s32_4 = hdr.st.s32_7 ^ hdr.st.s32_9;
  }

  action compute_op_xor_361() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.hdr1.data5;
  }

  action compute_rotate_left_119() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.st.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
  }

  action compute_op_xor_364() {
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.hdr1.data6;
  }

  action compute_rotate_left_120() {
    hdr.st.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_0 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_4 = (hdr.st.s32_7) ^ (hdr.st.s32_9);
    hdr.st.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_122() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
    hdr.st.s32_3 = (hdr.st.s32_8) ^ (hdr.st.s32_10);
    hdr.st.s32_9 = (hdr.st.s32_10) + (hdr.st.s32_4);
  }

  action compute_rotate_left_123() {
    @in_hash { hdr.st.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_6 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_2 = hdr.st.s32_0 + hdr.st.s32_3;
    hdr.st.s32_4 = hdr.st.s32_7 ^ hdr.st.s32_9;
  }

  action compute_op_xor_372() {
    hdr.st.s32_3 = hdr.st.s32_8 ^ hdr.st.s32_2;
  }

  action compute_op_xor_399() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.st.s32_5;
  }

  action compute_op_xor_402() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.hdr2.data1; }
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_0_724789;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073927040_hash_1_724789;

  RegisterAction<bit<1>, bit<20>, void>(bf_1073927040_row_0) bf_1073927040_row_0_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073927040_row_0_set_to_one_execute() {
    bf_1073927040_row_0_set_to_one.execute(bf_1073927040_hash_0_724789.get({
      hdr.hdr1.data5,
      hdr.hdr1.data6,
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
    bf_1073927040_row_1_set_to_one.execute(bf_1073927040_hash_1_724789.get({
      hdr.hdr1.data5,
      hdr.hdr1.data6,
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0x2681580b
    }));
  }


  apply {
    meta.to_egress = 0;
    hdr.st.setValid();

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  393907:RotateLeft
        // BDD node 46:rotate_left
        compute_op_add_285_b();
        compute_op_add_285();
        compute_rotate_left_113();
        compute_rotate_left_115();
        compute_rotate_left_116();
        compute_rotate_left_117();
        compute_rotate_left_118();
        compute_op_xor_286();
        compute_rotate_left_119();
        compute_op_xor_289();
        compute_rotate_left_120();
        compute_rotate_left_122();
        compute_rotate_left_123();
        compute_op_xor_372();
        // EP node  398816:RotateLeft
        // BDD node 47:rotate_left
        // EP node  401285:RotateLeft
        // BDD node 48:rotate_left
        // EP node  403454:ArithmeticOp
        // BDD node 277:op_add
        // EP node  405939:RotateLeft
        // BDD node 49:rotate_left
        // EP node  409672:ArithmeticOp
        // BDD node 279:op_add
        // EP node  412173:RotateLeft
        // BDD node 50:rotate_left
        // EP node  415930:ArithmeticOp
        // BDD node 280:op_xor
        // EP node  418761:RotateLeft
        // BDD node 51:rotate_left
        // EP node  421287:ArithmeticOp
        // BDD node 281:op_add
        // EP node  424136:ArithmeticOp
        // BDD node 282:op_xor
        // EP node  426994:ArithmeticOp
        // BDD node 283:op_add
        // EP node  430178:ArithmeticOp
        // BDD node 284:op_xor
        // EP node  433054:ArithmeticOp
        // BDD node 287:op_xor
        // EP node  435620:ArithmeticOp
        // BDD node 285:op_add
        // EP node  438514:ArithmeticOp
        // BDD node 286:op_xor
        // EP node  441738:ArithmeticOp
        // BDD node 289:op_xor
        // EP node  444972:RotateLeft
        // BDD node 52:rotate_left
        // EP node  449831:RotateLeft
        // BDD node 53:rotate_left
        // EP node  452437:RotateLeft
        // BDD node 54:rotate_left
        // EP node  454726:ArithmeticOp
        // BDD node 288:op_add
        // EP node  457348:RotateLeft
        // BDD node 55:rotate_left
        // EP node  460959:ArithmeticOp
        // BDD node 290:op_add
        // EP node  463597:RotateLeft
        // BDD node 56:rotate_left
        // EP node  467230:ArithmeticOp
        // BDD node 291:op_xor
        // EP node  469884:RotateLeft
        // BDD node 57:rotate_left
        // EP node  472215:ArithmeticOp
        // BDD node 292:op_add
        // EP node  474885:ArithmeticOp
        // BDD node 293:op_xor
        // EP node  477563:ArithmeticOp
        // BDD node 294:op_add
        // EP node  480583:ArithmeticOp
        // BDD node 295:op_xor
        // EP node  483947:ArithmeticOp
        // BDD node 297:op_xor
        // EP node  486984:SendToEgress
        // BDD node 58:rotate_left
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 1;
        hdr.egress_state.time = meta.time;
        hdr.egress_state.vector_table_1073939616_88_get_value_param0 = hdr.egress_state.vector_table_1073939616_88_get_value_param0;
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(1);
      } else if (hdr.recirc.code_path == 1) {
        // EP node  569333:RotateLeft
        // BDD node 70:rotate_left
        compute_rotate_left_113();
        compute_rotate_left_115();
        compute_rotate_left_116();
        compute_rotate_left_117();
        compute_rotate_left_118();
        compute_rotate_left_119();
        compute_rotate_left_120();
        compute_rotate_left_122();
        compute_rotate_left_123();
        compute_op_xor_372();
        // EP node  574498:RotateLeft
        // BDD node 71:rotate_left
        // EP node  577094:RotateLeft
        // BDD node 72:rotate_left
        // EP node  579327:ArithmeticOp
        // BDD node 313:op_add
        // EP node  581937:RotateLeft
        // BDD node 73:rotate_left
        // EP node  585670:ArithmeticOp
        // BDD node 315:op_add
        // EP node  588294:RotateLeft
        // BDD node 74:rotate_left
        // EP node  592047:ArithmeticOp
        // BDD node 316:op_xor
        // EP node  595061:RotateLeft
        // BDD node 75:rotate_left
        // EP node  597707:ArithmeticOp
        // BDD node 317:op_add
        // EP node  600737:ArithmeticOp
        // BDD node 318:op_xor
        // EP node  603775:ArithmeticOp
        // BDD node 319:op_add
        // EP node  607200:ArithmeticOp
        // BDD node 320:op_xor
        // EP node  611014:ArithmeticOp
        // BDD node 322:op_xor
        // EP node  614838:RotateLeft
        // BDD node 76:rotate_left
        // EP node  620582:RotateLeft
        // BDD node 77:rotate_left
        // EP node  623660:RotateLeft
        // BDD node 78:rotate_left
        // EP node  626362:ArithmeticOp
        // BDD node 321:op_add
        // EP node  629456:RotateLeft
        // BDD node 79:rotate_left
        // EP node  633716:ArithmeticOp
        // BDD node 323:op_add
        // EP node  636826:RotateLeft
        // BDD node 80:rotate_left
        // EP node  641108:ArithmeticOp
        // BDD node 324:op_xor
        // EP node  644234:RotateLeft
        // BDD node 81:rotate_left
        // EP node  646978:ArithmeticOp
        // BDD node 325:op_add
        // EP node  650120:ArithmeticOp
        // BDD node 326:op_xor
        // EP node  653270:ArithmeticOp
        // BDD node 327:op_add
        // EP node  656821:ArithmeticOp
        // BDD node 328:op_xor
        // EP node  660775:ArithmeticOp
        // BDD node 330:op_xor
        // EP node  664343:SendToEgress
        // BDD node 82:rotate_left
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 2;
        hdr.egress_state.time = meta.time;
        hdr.egress_state.vector_table_1073939616_88_get_value_param0 = hdr.egress_state.vector_table_1073939616_88_get_value_param0;
        nf_dev[15:0] = bswap16(16w0x0000);
      } else if (hdr.recirc.code_path == 2) {
        // EP node  46244:RotateLeft
        // BDD node 137:rotate_left
        compute_rotate_left_113();
        compute_rotate_left_115();
        compute_rotate_left_116();
        compute_rotate_left_117();
        compute_rotate_left_118();
        compute_op_xor_399();
        compute_rotate_left_119();
        compute_op_xor_402();
        compute_rotate_left_120();
        compute_rotate_left_122();
        compute_rotate_left_123();
        compute_op_xor_372();
        // EP node  47589:RotateLeft
        // BDD node 138:rotate_left
        // EP node  48274:RotateLeft
        // BDD node 139:rotate_left
        // EP node  48852:ArithmeticOp
        // BDD node 391:op_add
        // EP node  49549:RotateLeft
        // BDD node 140:rotate_left
        // EP node  50482:ArithmeticOp
        // BDD node 393:op_add
        // EP node  51191:RotateLeft
        // BDD node 141:rotate_left
        // EP node  52140:ArithmeticOp
        // BDD node 394:op_xor
        // EP node  52980:RotateLeft
        // BDD node 142:rotate_left
        // EP node  53708:ArithmeticOp
        // BDD node 395:op_add
        // EP node  54562:ArithmeticOp
        // BDD node 396:op_xor
        // EP node  55423:ArithmeticOp
        // BDD node 397:op_add
        // EP node  56413:ArithmeticOp
        // BDD node 398:op_xor
        // EP node  57534:ArithmeticOp
        // BDD node 400:op_xor
        // EP node  58540:ArithmeticOp
        // BDD node 399:op_xor
        // EP node  59679:ArithmeticOp
        // BDD node 402:op_xor
        // EP node  60827:RotateLeft
        // BDD node 143:rotate_left
        // EP node  62492:RotateLeft
        // BDD node 144:rotate_left
        // EP node  63402:RotateLeft
        // BDD node 145:rotate_left
        // EP node  64190:ArithmeticOp
        // BDD node 401:op_add
        // EP node  65114:RotateLeft
        // BDD node 146:rotate_left
        // EP node  66307:ArithmeticOp
        // BDD node 403:op_add
        // EP node  67245:RotateLeft
        // BDD node 147:rotate_left
        // EP node  68456:ArithmeticOp
        // BDD node 404:op_xor
        // EP node  69408:RotateLeft
        // BDD node 148:rotate_left
        // EP node  70232:ArithmeticOp
        // BDD node 405:op_add
        // EP node  71198:ArithmeticOp
        // BDD node 406:op_xor
        // EP node  72171:ArithmeticOp
        // BDD node 407:op_add
        // EP node  73289:ArithmeticOp
        // BDD node 408:op_xor
        // EP node  74554:ArithmeticOp
        // BDD node 410:op_xor
        // EP node  75687:SendToEgress
        // BDD node 149:rotate_left
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 4;
        hdr.egress_state.time = meta.time;
        hdr.egress_state.dev = hdr.egress_state.dev;
        hdr.egress_state.vector_table_1073939616_105_get_value_param0 = hdr.egress_state.vector_table_1073939616_105_get_value_param0;
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(3);
      } else if (hdr.recirc.code_path == 3) {
        // EP node  106107:RotateLeft
        // BDD node 161:rotate_left
        compute_rotate_left_113();
        compute_rotate_left_115();
        compute_rotate_left_116();
        compute_rotate_left_117();
        compute_rotate_left_118();
        compute_rotate_left_119();
        compute_rotate_left_120();
        compute_rotate_left_122();
        compute_rotate_left_123();
        compute_op_xor_372();
        // EP node  108184:RotateLeft
        // BDD node 162:rotate_left
        // EP node  109235:RotateLeft
        // BDD node 163:rotate_left
        // EP node  110118:ArithmeticOp
        // BDD node 426:op_add
        // EP node  111181:RotateLeft
        // BDD node 164:rotate_left
        // EP node  112602:ArithmeticOp
        // BDD node 428:op_add
        // EP node  113677:RotateLeft
        // BDD node 165:rotate_left
        // EP node  115114:ArithmeticOp
        // BDD node 429:op_xor
        // EP node  116381:RotateLeft
        // BDD node 166:rotate_left
        // EP node  117475:ArithmeticOp
        // BDD node 430:op_add
        // EP node  118756:ArithmeticOp
        // BDD node 431:op_xor
        // EP node  120044:ArithmeticOp
        // BDD node 432:op_add
        // EP node  121522:ArithmeticOp
        // BDD node 433:op_xor
        // EP node  123192:ArithmeticOp
        // BDD node 435:op_xor
        // EP node  124871:RotateLeft
        // BDD node 167:rotate_left
        // EP node  127303:RotateLeft
        // BDD node 168:rotate_left
        // EP node  128626:RotateLeft
        // BDD node 169:rotate_left
        // EP node  129768:ArithmeticOp
        // BDD node 434:op_add
        // EP node  131105:RotateLeft
        // BDD node 170:rotate_left
        // EP node  132829:ArithmeticOp
        // BDD node 436:op_add
        // EP node  134180:RotateLeft
        // BDD node 171:rotate_left
        // EP node  135922:ArithmeticOp
        // BDD node 437:op_xor
        // EP node  137287:RotateLeft
        // BDD node 172:rotate_left
        // EP node  138465:ArithmeticOp
        // BDD node 438:op_add
        // EP node  139844:ArithmeticOp
        // BDD node 439:op_xor
        // EP node  141230:ArithmeticOp
        // BDD node 440:op_add
        // EP node  142820:ArithmeticOp
        // BDD node 441:op_xor
        // EP node  144616:ArithmeticOp
        // BDD node 443:op_xor
        // EP node  146221:SendToEgress
        // BDD node 173:rotate_left
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 5;
        hdr.egress_state.time = meta.time;
        hdr.egress_state.dev = hdr.egress_state.dev;
        hdr.egress_state.vector_table_1073939616_105_get_value_param0 = hdr.egress_state.vector_table_1073939616_105_get_value_param0;
        nf_dev[15:0] = hdr.egress_state.dev[15:0];
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
          // EP node  752230:Forward
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
                bf_1073927040_row_0_read_execute();
                bf_1073927040_row_1_read_execute();
                // EP node  856:If
                // BDD node 15:if
                if ((32w0x00000000) == (meta.bf_1073927040_estimate)){
                  // EP node  857:Then
                  // BDD node 15:if
                  // EP node  171018:RotateLeft
                  // BDD node 16:rotate_left
                  compute_rotate_left_107();
                  // EP node  173281:VectorTableLookup
                  // BDD node 88:vector_borrow
                  meta.key_32b_0 = 32w0x00000000;
                  vector_table_1073939616_88.apply();
                  // EP node  176005:Ignore
                  // BDD node 89:vector_return
                  // EP node  178969:RotateLeft
                  // BDD node 17:rotate_left
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
                  compute_op_xor_361();
                  compute_rotate_left_119();
                  compute_op_xor_364();
                  compute_rotate_left_120();
                  compute_rotate_left_122();
                  compute_rotate_left_123();
                  compute_op_xor_372();
                  // EP node  181491:RotateLeft
                  // BDD node 18:rotate_left
                  // EP node  183795:RotateLeft
                  // BDD node 19:rotate_left
                  // EP node  187259:ArithmeticOp
                  // BDD node 231:op_xor
                  // EP node  189352:ArithmeticOp
                  // BDD node 232:op_add
                  // EP node  191686:RotateLeft
                  // BDD node 20:rotate_left
                  // EP node  195195:ArithmeticOp
                  // BDD node 233:op_xor
                  // EP node  197315:ArithmeticOp
                  // BDD node 234:op_add
                  // EP node  199679:RotateLeft
                  // BDD node 21:rotate_left
                  // EP node  201817:ArithmeticOp
                  // BDD node 235:op_add
                  // EP node  204201:ArithmeticOp
                  // BDD node 236:op_xor
                  // EP node  206595:ArithmeticOp
                  // BDD node 237:op_add
                  // EP node  209238:ArithmeticOp
                  // BDD node 238:op_xor
                  // EP node  212132:ArithmeticOp
                  // BDD node 240:op_xor
                  // EP node  215038:RotateLeft
                  // BDD node 22:rotate_left
                  // EP node  219650:RotateLeft
                  // BDD node 23:rotate_left
                  // EP node  222094:RotateLeft
                  // BDD node 24:rotate_left
                  // EP node  224304:ArithmeticOp
                  // BDD node 239:op_add
                  // EP node  226768:RotateLeft
                  // BDD node 25:rotate_left
                  // EP node  230472:ArithmeticOp
                  // BDD node 241:op_add
                  // EP node  232956:RotateLeft
                  // BDD node 26:rotate_left
                  // EP node  236690:ArithmeticOp
                  // BDD node 242:op_xor
                  // EP node  239194:RotateLeft
                  // BDD node 27:rotate_left
                  // EP node  241458:ArithmeticOp
                  // BDD node 243:op_add
                  // EP node  243982:ArithmeticOp
                  // BDD node 244:op_xor
                  // EP node  246516:ArithmeticOp
                  // BDD node 245:op_add
                  // EP node  249313:ArithmeticOp
                  // BDD node 246:op_xor
                  // EP node  252375:ArithmeticOp
                  // BDD node 248:op_xor
                  // EP node  255194:ArithmeticOp
                  // BDD node 247:op_xor
                  // EP node  258280:ArithmeticOp
                  // BDD node 250:op_xor
                  // EP node  261378:RotateLeft
                  // BDD node 28:rotate_left
                  // EP node  266294:RotateLeft
                  // BDD node 29:rotate_left
                  // EP node  268898:RotateLeft
                  // BDD node 30:rotate_left
                  // EP node  271252:ArithmeticOp
                  // BDD node 249:op_add
                  // EP node  273876:RotateLeft
                  // BDD node 31:rotate_left
                  // EP node  277820:ArithmeticOp
                  // BDD node 251:op_add
                  // EP node  280464:RotateLeft
                  // BDD node 32:rotate_left
                  // EP node  284438:ArithmeticOp
                  // BDD node 252:op_xor
                  // EP node  287102:RotateLeft
                  // BDD node 33:rotate_left
                  // EP node  289510:ArithmeticOp
                  // BDD node 253:op_add
                  // EP node  292194:ArithmeticOp
                  // BDD node 254:op_xor
                  // EP node  294888:ArithmeticOp
                  // BDD node 255:op_add
                  // EP node  297861:ArithmeticOp
                  // BDD node 256:op_xor
                  // EP node  301115:ArithmeticOp
                  // BDD node 258:op_xor
                  // EP node  304109:SendToEgress
                  // BDD node 34:rotate_left
                  meta.to_egress = 1;
                  hdr.egress_state.setValid();
                  hdr.egress_state.code_path = 0;
                  hdr.egress_state.time = meta.time;
                  hdr.egress_state.vector_table_1073939616_88_get_value_param0 = vector_table_1073939616_88_get_value_param0;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(0);
                } else {
                  // EP node  858:Else
                  // BDD node 15:if
                  // EP node  1282:Forward
                  // BDD node 103:FORWARD
                  nf_dev[15:0] = 16w0x0000;
                }
              } else {
                // EP node  391:Else
                // BDD node 13:if
                // EP node  1519:If
                // BDD node 104:if
                if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[31:16][7:0])) & (32w0x00000010))){
                  // EP node  1520:Then
                  // BDD node 104:if
                  // EP node  1970:VectorTableLookup
                  // BDD node 105:vector_borrow
                  meta.key_32b_0 = 32w0x00000000;
                  vector_table_1073939616_105.apply();
                  // EP node  2252:Ignore
                  // BDD node 106:vector_return
                  // EP node  2575:RotateLeft
                  // BDD node 107:rotate_left
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
                  compute_op_xor_361();
                  compute_rotate_left_119();
                  compute_op_xor_364();
                  compute_rotate_left_120();
                  compute_rotate_left_122();
                  compute_rotate_left_123();
                  compute_op_xor_372();
                  // EP node  2909:RotateLeft
                  // BDD node 108:rotate_left
                  // EP node  3220:RotateLeft
                  // BDD node 109:rotate_left
                  // EP node  3506:RotateLeft
                  // BDD node 110:rotate_left
                  // EP node  3905:ArithmeticOp
                  // BDD node 345:op_xor
                  // EP node  4171:ArithmeticOp
                  // BDD node 346:op_add
                  // EP node  4481:RotateLeft
                  // BDD node 111:rotate_left
                  // EP node  4913:ArithmeticOp
                  // BDD node 347:op_xor
                  // EP node  5200:ArithmeticOp
                  // BDD node 348:op_add
                  // EP node  5534:RotateLeft
                  // BDD node 112:rotate_left
                  // EP node  5835:ArithmeticOp
                  // BDD node 349:op_add
                  // EP node  6185:ArithmeticOp
                  // BDD node 350:op_xor
                  // EP node  6543:ArithmeticOp
                  // BDD node 351:op_add
                  // EP node  6953:ArithmeticOp
                  // BDD node 352:op_xor
                  // EP node  7417:ArithmeticOp
                  // BDD node 354:op_xor
                  // EP node  7891:RotateLeft
                  // BDD node 113:rotate_left
                  // EP node  8610:RotateLeft
                  // BDD node 114:rotate_left
                  // EP node  9008:RotateLeft
                  // BDD node 115:rotate_left
                  // EP node  9365:ArithmeticOp
                  // BDD node 353:op_add
                  // EP node  9779:RotateLeft
                  // BDD node 116:rotate_left
                  // EP node  10354:ArithmeticOp
                  // BDD node 355:op_add
                  // EP node  10784:RotateLeft
                  // BDD node 117:rotate_left
                  // EP node  11381:ArithmeticOp
                  // BDD node 356:op_xor
                  // EP node  11827:RotateLeft
                  // BDD node 118:rotate_left
                  // EP node  12226:ArithmeticOp
                  // BDD node 357:op_add
                  // EP node  12688:ArithmeticOp
                  // BDD node 358:op_xor
                  // EP node  13158:ArithmeticOp
                  // BDD node 359:op_add
                  // EP node  13694:ArithmeticOp
                  // BDD node 360:op_xor
                  // EP node  14298:ArithmeticOp
                  // BDD node 362:op_xor
                  // EP node  14852:ArithmeticOp
                  // BDD node 361:op_xor
                  // EP node  15476:ArithmeticOp
                  // BDD node 364:op_xor
                  // EP node  16110:RotateLeft
                  // BDD node 119:rotate_left
                  // EP node  17069:RotateLeft
                  // BDD node 120:rotate_left
                  // EP node  17595:RotateLeft
                  // BDD node 121:rotate_left
                  // EP node  18064:ArithmeticOp
                  // BDD node 363:op_add
                  // EP node  18606:RotateLeft
                  // BDD node 122:rotate_left
                  // EP node  19357:ArithmeticOp
                  // BDD node 365:op_add
                  // EP node  19915:RotateLeft
                  // BDD node 123:rotate_left
                  // EP node  20688:ArithmeticOp
                  // BDD node 366:op_xor
                  // EP node  21262:RotateLeft
                  // BDD node 124:rotate_left
                  // EP node  21773:ArithmeticOp
                  // BDD node 367:op_add
                  // EP node  22363:ArithmeticOp
                  // BDD node 368:op_xor
                  // EP node  22961:ArithmeticOp
                  // BDD node 369:op_add
                  // EP node  23641:ArithmeticOp
                  // BDD node 370:op_xor
                  // EP node  24405:ArithmeticOp
                  // BDD node 372:op_xor
                  // EP node  25102:SendToEgress
                  // BDD node 125:rotate_left
                  meta.to_egress = 1;
                  hdr.egress_state.setValid();
                  hdr.egress_state.code_path = 3;
                  hdr.egress_state.time = meta.time;
                  hdr.egress_state.dev = meta.dev;
                  hdr.egress_state.vector_table_1073939616_105_get_value_param0 = vector_table_1073939616_105_get_value_param0;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(2);
                } else {
                  // EP node  1521:Else
                  // BDD node 104:if
                  // EP node  759781:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  278:Else
              // BDD node 12:if
              // EP node  714961:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[31:16][7:0])) & (32w0x00000040))){
                // EP node  714962:Then
                // BDD node 188:if
                // EP node  720081:Forward
                // BDD node 192:FORWARD
                @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
              } else {
                // EP node  714963:Else
                // BDD node 188:if
                // EP node  724789:BloomFilterSet
                // BDD node 193:bf_set
                bf_1073927040_row_0_set_to_one_execute();
                bf_1073927040_row_1_set_to_one_execute();
                // EP node  731707:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  124:Else
          // BDD node 10:if
          // EP node  753112:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  734298:ParserCondition
          // BDD node 201:if
          // EP node  734299:Then
          // BDD node 201:if
          // EP node  741273:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  749587:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev[15:0])){
              // EP node  749588:Then
              // BDD node 203:if
              // EP node  756215:ParserCondition
              // BDD node 204:if
              // EP node  756216:Then
              // BDD node 204:if
              // EP node  761121:ParserCondition
              // BDD node 205:if
              // EP node  761122:Then
              // BDD node 205:if
              // EP node  779237:Forward
              // BDD node 209:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
              // EP node  761123:Else
              // BDD node 205:if
              // EP node  766978:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  773752:SendToController
                // BDD node 211:vector_borrow
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(0);
                hdr.cpu.time = meta.time;
              }
              // EP node  756217:Else
              // BDD node 204:if
              // EP node  778321:Forward
              // BDD node 221:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
            } else {
              // EP node  749589:Else
              // BDD node 203:if
              // EP node  776034:Forward
              // BDD node 225:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
            }
          }
          // EP node  734300:Else
          // BDD node 201:if
          // EP node  758889:Forward
          // BDD node 228:FORWARD
          @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[31:24]); }
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  740401:ParserReject
        // BDD node 230:DROP
      }

    }

    forwarding_tbl.apply();
    if (meta.leaving != 0 && meta.to_egress == 0) {
      hdr.recirc.setInvalid();
      hdr.egress_state.setInvalid();
    }
    if (meta.leaving == 1 && meta.to_egress == 0) {
      hdr.st.setInvalid();
    }

    if (meta.to_egress == 1) {
      ig_tm_md.bypass_egress = 0;
    } else {
      ig_tm_md.bypass_egress = 1;
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

  action compute_op_or_266() {
    hdr.st.s32_5 = hdr.st.s32_1 | hdr.st.s32_5;
  }

  action compute_op_xor_270() {
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_5;
  }

  action compute_op_xor_303() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.st.s32_1;
  }

  action compute_op_sub_333() {
    eg_md.op_sub_333_out = hdr.egress_state.time - hdr.egress_state.vector_table_1073939616_88_get_value_param0;
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
  action compute_rotate_left_125() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.st.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
    hdr.st.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_op_shl_379_a() {
    hdr.st.s32_1 = hdr.hdr2.data0 >> 16;
  }

  action compute_op_or_380_b() {
    hdr.st.s32_5 = (bit<32>)(hdr.hdr2.data0[15:0]);
  }

  action compute_rotate_left_127() {
    hdr.st.s32_0 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_4 = (hdr.st.s32_7) ^ (hdr.st.s32_9);
    hdr.st.s32_3 = (hdr.st.s32_8) ^ (hdr.st.s32_10);
  }

  action compute_op_shl_379() {
    hdr.st.s32_1 = hdr.st.s32_1 << 32w0x00000010;
  }

  action compute_rotate_left_128() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
  }

  action compute_rotate_left_129() {
    @in_hash { hdr.st.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_9 = (hdr.st.s32_10) + (hdr.st.s32_4);
    hdr.st.s32_2 = hdr.st.s32_0 + hdr.st.s32_3;
  }

  action compute_rotate_left_130() {
    hdr.st.s32_6 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_3 = hdr.st.s32_8 ^ hdr.st.s32_2;
    hdr.st.s32_4 = hdr.st.s32_7 ^ hdr.st.s32_9;
  }

  action compute_op_or_380() {
    hdr.st.s32_5 = hdr.st.s32_1 | hdr.st.s32_5;
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.hdr1.data6;
  }

  action compute_rotate_left_131() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.st.s32_9 = (hdr.st.s32_2) + (hdr.st.s32_4);
  }

  action compute_op_xor_384() {
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_5;
  }

  action compute_rotate_left_132() {
    hdr.st.s32_8 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_0 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_4 = (hdr.st.s32_7) ^ (hdr.st.s32_9);
    hdr.st.s32_10 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_134() {
    @in_hash { hdr.st.s32_7 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
    hdr.st.s32_3 = (hdr.st.s32_8) ^ (hdr.st.s32_10);
    hdr.st.s32_9 = (hdr.st.s32_10) + (hdr.st.s32_4);
  }

  action compute_rotate_left_135() {
    @in_hash { hdr.st.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_6 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_2 = hdr.st.s32_0 + hdr.st.s32_3;
    hdr.st.s32_4 = hdr.st.s32_7 ^ hdr.st.s32_9;
  }

  action compute_op_xor_392() {
    hdr.st.s32_3 = hdr.st.s32_8 ^ hdr.st.s32_2;
  }

  action compute_op_xor_416() {
    @in_hash { hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.hdr2.data1; }
  }

  action compute_op_lshr_446() {
    eg_md.op_lshr_446_out = hdr.egress_state.time;
  }

  action compute_op_sub_447() {
    eg_md.op_sub_447_out = eg_md.op_lshr_446_out[31:0] - hdr.egress_state.vector_table_1073939616_105_get_value_param0;
  }

  action compute_op_lshr_448() {
    eg_md.op_lshr_448_out = eg_md.op_sub_447_out >> 32w0x0000000c;
  }

  action compute_op_add_450_b() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_3; }
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
  action compute_rotate_left_127_v0() {
    hdr.st.s32_3 = (hdr.st.s32_8) ^ (hdr.st.s32_10);
  }

  action compute_rotate_left_130_v0() {
    hdr.st.s32_6 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
  }

  action compute_rotate_left_130_v1() {
    hdr.st.s32_3 = hdr.st.s32_8 ^ hdr.st.s32_2;
    hdr.st.s32_4 = hdr.st.s32_7 ^ hdr.st.s32_9;
  }

  action compute_rotate_left_129_v0() {
    @in_hash { hdr.st.s32_8 = hdr.st.s32_3[24:0] ++ hdr.st.s32_3[31:25]; }
    hdr.st.s32_9 = (hdr.st.s32_10) + (hdr.st.s32_4);
  }

  action compute_rotate_left_129_v1() {
    hdr.st.s32_9 = (hdr.st.s32_10) + (hdr.st.s32_4);
    hdr.st.s32_2 = hdr.st.s32_0 + hdr.st.s32_3;
  }

  action compute_op_or_380_v0() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.hdr1.data6;
  }


  apply {
    if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4 || hdr.egress_state.code_path == 5) {
      if (hdr.egress_state.code_path == 2) {
        // EP node  670296:RotateLeft
        // BDD node 82:rotate_left
        compute_op_sub_333();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 3) {
        compute_op_or_380_b();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 3) {
        compute_op_shl_379_a();
      }
      compute_rotate_left_125();
      if (hdr.egress_state.code_path == 2) {
        compute_op_lshr_334();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 3) {
        compute_op_shl_379();
      }
      compute_rotate_left_127();
      if (hdr.egress_state.code_path == 0) {
        compute_op_or_266();
      }
      compute_rotate_left_128();
      if (hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 5) {
        compute_op_add_450_b();
      }
      if (hdr.egress_state.code_path == 5) {
        compute_rotate_left_129_v0();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_rotate_left_129();
      }
      if (hdr.egress_state.code_path == 5) {
        compute_rotate_left_130_v0();
        // EP node  151047:RotateLeft
        // BDD node 174:rotate_left
        // EP node  152059:RotateLeft
        // BDD node 175:rotate_left
        // EP node  152873:ArithmeticOp
        // BDD node 442:op_add
        // EP node  153895:RotateLeft
        // BDD node 176:rotate_left
        // EP node  155332:ArithmeticOp
        // BDD node 444:op_add
        // EP node  156364:RotateLeft
        // BDD node 177:rotate_left
        // EP node  157815:ArithmeticOp
        // BDD node 445:op_xor
        // EP node  158857:RotateLeft
        // BDD node 178:rotate_left
        // EP node  159904:ChecksumUpdate
        // BDD node 179:nf_set_rte_ipv4_udptcp_checksum
        // EP node  160746:ArithmeticOp
        // BDD node 446:op_lshr
        compute_op_lshr_446();
        compute_rotate_left_127_v0();
        compute_op_sub_447();
        compute_op_lshr_448();
        compute_rotate_left_129_v1();
        compute_rotate_left_130_v1();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_rotate_left_130();
      }
      if (hdr.egress_state.code_path == 0) {
        compute_op_or_380_v0();
      }
      if (hdr.egress_state.code_path == 1) {
        compute_op_xor_303();
      }
      if (hdr.egress_state.code_path == 3) {
        compute_op_or_380();
      }
      if (hdr.egress_state.code_path == 4) {
        compute_op_xor_416();
      }
      if (hdr.egress_state.code_path == 2) {
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_rotate_left_131();
      }
      if (hdr.egress_state.code_path == 0) {
        compute_op_xor_270();
      }
      if (hdr.egress_state.code_path == 3) {
        compute_op_xor_384();
      }
      if (hdr.egress_state.code_path == 2) {
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_rotate_left_132();
      }
      if (hdr.egress_state.code_path == 2 || hdr.egress_state.code_path == 5) {
        compute_op_xor_456();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_rotate_left_134();
      }
      if (hdr.egress_state.code_path == 5) {
        // EP node  161592:ArithmeticOp
        // BDD node 447:op_sub
        // EP node  162442:ArithmeticOp
        // BDD node 448:op_lshr
        // EP node  163083:ArithmeticOp
        // BDD node 449:op_xor
        // EP node  163727:ArithmeticOp
        // BDD node 450:op_add
        // EP node  164374:ArithmeticOp
        // BDD node 451:op_add
        // EP node  165024:ArithmeticOp
        // BDD node 452:op_xor
        // EP node  165677:ArithmeticOp
        // BDD node 453:op_xor
        // EP node  166333:ArithmeticOp
        // BDD node 454:op_xor
        // EP node  166773:ArithmeticOp
        // BDD node 455:op_xor
        // EP node  167215:ArithmeticOp
        // BDD node 456:op_xor
        // EP node  167659:ModifyHeader
        // BDD node 180:packet_return_chunk
        hdr_val2_calc();
        hdr_val3_calc();
        hdr_val4_calc();
        rewrite_180();
        // EP node  168105:ModifyHeader
        // BDD node 181:packet_return_chunk
        rewrite_181();
        // EP node  169000:Forward
        // BDD node 183:FORWARD
        eg_md.redo_checksum = 1;
        eg_md.l4_len = 16w20;
        hdr.recirc.setInvalid();
        hdr.egress_state.setInvalid();
        hdr.st.setInvalid();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_rotate_left_135();
      }
      if (hdr.egress_state.code_path == 2) {
        compute_op_xor_344();
      }
      if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 1 || hdr.egress_state.code_path == 3 || hdr.egress_state.code_path == 4) {
        compute_op_xor_392();
      }
      if (hdr.egress_state.code_path == 2) {
        compute_cond_operand_90_0();
        // EP node  675459:RotateLeft
        // BDD node 83:rotate_left
        // EP node  677849:RotateLeft
        // BDD node 84:rotate_left
        // EP node  679846:ArithmeticOp
        // BDD node 329:op_add
        // EP node  682248:RotateLeft
        // BDD node 85:rotate_left
        // EP node  685859:ArithmeticOp
        // BDD node 331:op_add
        // EP node  688273:RotateLeft
        // BDD node 86:rotate_left
        // EP node  691902:ArithmeticOp
        // BDD node 332:op_xor
        // EP node  694328:RotateLeft
        // BDD node 87:rotate_left
        // EP node  696355:ArithmeticOp
        // BDD node 333:op_sub
        // EP node  698387:ArithmeticOp
        // BDD node 334:op_lshr
        // EP node  700017:ArithmeticOp
        // BDD node 335:op_add
        // EP node  701243:ArithmeticOp
        // BDD node 336:op_xor
        // EP node  702472:ArithmeticOp
        // BDD node 337:op_add
        // EP node  703704:ArithmeticOp
        // BDD node 338:op_add
        // EP node  704939:ArithmeticOp
        // BDD node 339:op_xor
        // EP node  706177:ArithmeticOp
        // BDD node 340:op_xor
        // EP node  707418:ArithmeticOp
        // BDD node 341:op_xor
        // EP node  708248:ArithmeticOp
        // BDD node 342:op_xor
        // EP node  709080:ArithmeticOp
        // BDD node 343:op_xor
        // EP node  709914:ArithmeticOp
        // BDD node 344:op_xor
        // EP node  710750:If
        // BDD node 90:if
        bool cond0 = false;
        if ((eg_md.cond_operand_90_0_out[31:8]) == (24w0x000000)){
          if ((eg_md.cond_operand_90_0_out[7:0]) <= (8w0x02)){
            cond0 = true;
          }
        }
        if (cond0) {
          // EP node  710751:Then
          // BDD node 90:if
          // EP node  720933:ChecksumUpdate
          // BDD node 91:nf_set_rte_ipv4_udptcp_checksum
          // EP node  721787:ModifyHeader
          // BDD node 92:packet_return_chunk
          hdr_val0_calc();
          hdr_val1_calc();
          rewrite_92();
          // EP node  722643:ModifyHeader
          // BDD node 93:packet_return_chunk
          rewrite_93();
          // EP node  724358:Forward
          // BDD node 95:FORWARD
          eg_md.redo_checksum = 1;
          eg_md.l4_len = 16w20;
          hdr.recirc.setInvalid();
          hdr.egress_state.setInvalid();
          hdr.st.setInvalid();
        } else {
          // EP node  710752:Else
          // BDD node 90:if
          // EP node  714117:Drop
          // BDD node 99:DROP
          ig_intr_dprs_md.drop_ctl = 1;
        }
      }
    }

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
