// sc-m8 with path B's cookie xor chain (453 -> 454 -> 455, three hash ops and levels) as one hash op over its four words, as the ground truth writes its cookie
// branches set, the egress chain under (code_path == 0 || code_path == 3), as the ground truth calls its rounds
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
  bit<32> bf_1073926928_estimate;
  bit<32> f32_0;
  bit<32> f32_1;
  bit<32> rotated_86;      // m11: the seven values the walk's emitter skipped as header fields,
  bit<32> unrolled_101;    // in the controller's cpu_hdr_extra_t order
  bit<32> rotated_87;
  bit<32> unrolled_106;
  bit<32> rotated_84;
  bit<32> unrolled_100;
  bit<32> rotated_85;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> f32_0;
  bit<32> f32_1;
  bit<32> f32_2;
  bit<32> f32_3;


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
}

header egress_state_h {
  bit<16> code_path;
  bit<32> bf_1073926928_estimate;
  bit<32> vector_reg_value0;
  bit<32> dev;
  bit<32> vector_reg_value1;
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
  bit<8> data2_ttl;     // m10: split so the checksum is a whole field
  bit<8> data2_proto;
  bit<16> data2_csum;
  bit<32> data3;
  bit<32> data4;
}
header hdr2_h {
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<16> data3;
  bit<16> data4;
  bit<16> data5_csum;   // m10: split so the checksum is a whole field
  bit<16> data5_urg;
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
  bit<1> redo_checksum;   // m10
  bit<16> tcp_len;        // m10
  bit<8> lap1;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  bit<32> bf_1073926928_estimate;
  bit<24> key_24b_0;
  bit<8> key_8b_1;
  bit<24> key_24b_2;
  bit<8> key_8b_3;
  bit<16> key_16b_4;
  bit<16> key_16b_5;
  bit<32> vector_reg_value0;
  bit<32> vector_reg_value1;
  bit<32> op_xor_457_out;
  bit<32> hdr_val0;
  bit<32> hdr_val1;
  bit<8> hdr_val2;
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
  // The egress reads the clock itself rather than having it carried across the crossing: the
  // ingress keeps time as ingress_mac_tstamp[47:16], and the backend rewrites shifts of it to
  // match, a convention a value travelling in the state header would not carry with it.
  bit<32> time;
  bit<32> op_lshr_447_out;
  bit<32> op_sub_448_out;
  bit<32> op_lshr_449_out;
  bit<32> op_add_336_out;
  bit<32> op_xor_453_out;
  bit<32> op_xor_454_out;
  bit<32> op_xor_455_out;
  bit<32> op_xor_456_out;

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
    meta.redo_checksum = 0;
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
    transition select (hdr.hdr1.data2_proto) {
      8w0x11: parser_201;
      default: parser_6;
    }
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr1.data2_proto) {
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
    transition parser_225;
  }
  state parser_11 {
    pkt.extract(hdr.hdr2);
    transition parser_192;
  }
  state parser_225 {
    transition accept;
  }
  state parser_192 {
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
    hdr.recirc.setInvalid();
    hdr.cuckoo.setInvalid();
    hdr.egress_state.setInvalid();
    hdr.st.setInvalid();

    fwd(CPU_PCIE_PORT);
  }

  action fwd_nf_dev(bit<16> port) {
    hdr.cpu.setInvalid();
    hdr.recirc.setInvalid();
    hdr.cuckoo.setInvalid();
    hdr.egress_state.setInvalid();
    hdr.st.setInvalid();

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

  Register<bit<1>,_>(1048576, 0) bf_1073926928_row_0;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_538;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_538;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_602053;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_602053;
  Register<bit<1>,_>(1048576, 0) bf_1073926928_row_1;

  bit<20> bf_1073926928_hash_0_value;
  bit<20> bf_1073926928_hash_1_value;

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073926928_row_0) bf_1073926928_row_0_read_and_set = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
      value = 1;
    }
  };

  bit<1> bf_1073926928_row_0_read_and_set_value;
  action bf_1073926928_row_0_read_and_set_execute() {
    bf_1073926928_row_0_read_and_set_value = bf_1073926928_row_0_read_and_set.execute(bf_1073926928_hash_0_value);
    meta.bf_1073926928_estimate[0:0] = bf_1073926928_row_0_read_and_set_value[0:0];
  }

  RegisterAction<bit<1>, bit<20>, void>(bf_1073926928_row_0) bf_1073926928_row_0_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073926928_row_0_set_to_one_execute() {
    bf_1073926928_row_0_set_to_one.execute(bf_1073926928_hash_0_602053.get({
      hdr.hdr1.data3[31:8],
      hdr.hdr1.data3[7:0],
      hdr.hdr1.data4[31:8],
      hdr.hdr1.data4[7:0],
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0xfbc31fc7
    }));
  }

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073926928_row_0) bf_1073926928_row_0_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073926928_row_0_read_value;
  action bf_1073926928_row_0_read_execute() {
    bf_1073926928_row_0_read_value = bf_1073926928_row_0_read.execute(bf_1073926928_hash_0_538.get({
      hdr.hdr1.data3[31:8],
      hdr.hdr1.data3[7:0],
      hdr.hdr1.data4[31:8],
      hdr.hdr1.data4[7:0],
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0xfbc31fc7
    }));
    meta.bf_1073926928_estimate[0:0] = bf_1073926928_row_0_read_value[0:0];
  }

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073926928_row_1) bf_1073926928_row_1_read_and_set = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
      value = 1;
    }
  };

  bit<1> bf_1073926928_row_1_read_and_set_value;
  action bf_1073926928_row_1_read_and_set_execute() {
    bf_1073926928_row_1_read_and_set_value = bf_1073926928_row_1_read_and_set.execute(bf_1073926928_hash_1_value);
    meta.bf_1073926928_estimate[1:1] = bf_1073926928_row_1_read_and_set_value[0:0];
  }

  RegisterAction<bit<1>, bit<20>, void>(bf_1073926928_row_1) bf_1073926928_row_1_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073926928_row_1_set_to_one_execute() {
    bf_1073926928_row_1_set_to_one.execute(bf_1073926928_hash_1_602053.get({
      hdr.hdr1.data3[31:8],
      hdr.hdr1.data3[7:0],
      hdr.hdr1.data4[31:8],
      hdr.hdr1.data4[7:0],
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0x2681580b
    }));
  }

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073926928_row_1) bf_1073926928_row_1_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073926928_row_1_read_value;
  action bf_1073926928_row_1_read_execute() {
    bf_1073926928_row_1_read_value = bf_1073926928_row_1_read.execute(bf_1073926928_hash_1_538.get({
      hdr.hdr1.data3[31:8],
      hdr.hdr1.data3[7:0],
      hdr.hdr1.data4[31:8],
      hdr.hdr1.data4[7:0],
      hdr.hdr2.data0[31:16],
      hdr.hdr2.data0[15:0],
      32w0x2681580b
    }));
    meta.bf_1073926928_estimate[1:1] = bf_1073926928_row_1_read_value[0:0];
  }


  Register<bit<32>,_>(1, 0) vector_register_1073939504_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073939504_0) vector_register_1073939504_0_read_146605 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action compute_op_add_281() {
    hdr.st.s32_1 = hdr.st.s32_3 + hdr.st.s32_7;
    hdr.st.s32_8 = hdr.st.s32_8 ^ hdr.st.s32_3;
  }

  action compute_op_add_285_b() {
    @in_hash { hdr.st.s32_0 = hdr.hdr2.data1; }
  }

  action compute_op_add_285() {
    hdr.st.s32_0 = 32w0xffffffff + hdr.st.s32_0;
  }

  action compute_rotate_left_53_x() {
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_0);
  }

  action compute_rotate_left_53() {
    hdr.st.s32_5 = hdr.st.s32_2[23:0] ++ hdr.st.s32_2[31:24];
    hdr.st.s32_10 = hdr.st.s32_10 + hdr.st.s32_2;
  }

  action compute_rotate_left_56_x() {
    hdr.st.s32_5 = (hdr.st.s32_5) ^ (hdr.st.s32_10);
    hdr.st.s32_1 = (hdr.st.s32_10) + (hdr.st.s32_1);
  }

  action compute_rotate_left_56() {
    @in_hash { hdr.st.s32_2 = hdr.st.s32_5[24:0] ++ hdr.st.s32_5[31:25]; }
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_4 = (hdr.st.s32_4) ^ (hdr.st.s32_1);
    hdr.st.s32_3 = hdr.st.s32_3 + hdr.st.s32_5;
  }

  action compute_rotate_left_59_x() {
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_3);
    hdr.st.s32_1 = (hdr.st.s32_3) + (hdr.st.s32_4);
  }

  action compute_rotate_left_59() {
    hdr.st.s32_5 = hdr.st.s32_2[23:0] ++ hdr.st.s32_2[31:24];
    hdr.st.s32_8 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_6 = hdr.st.s32_6 + hdr.st.s32_2;
  }

  action compute_rotate_left_58() {
    @in_hash { hdr.st.s32_2 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.st.s32_5 = (hdr.st.s32_5) ^ (hdr.st.s32_6);
  }

  action compute_rotate_left_61_x() {
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_1);
    @in_hash { hdr.st.s32_3 = hdr.st.s32_5[24:0] ++ hdr.st.s32_5[31:25]; }
    hdr.st.s32_8 = hdr.st.s32_8 + hdr.st.s32_5;
  }

  action compute_rotate_left_61() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_2[18:0] ++ hdr.st.s32_2[31:19]; }
    hdr.st.s32_6 = (hdr.st.s32_6) + (hdr.st.s32_2);
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_8);
    hdr.st.s32_0 = hdr.st.s32_8 ^ hdr.st.s32_0;
  }

  action compute_rotate_left_63() {
    hdr.st.s32_2 = hdr.st.s32_6[15:0] ++ hdr.st.s32_6[31:16];
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_6);
    hdr.st.s32_4 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
  }

  action compute_rotate_left_66_x() {
    hdr.st.s32_0 = (hdr.st.s32_0) + (hdr.st.s32_1);
    hdr.st.s32_2 = hdr.st.s32_2 + hdr.st.s32_3;
  }

  action compute_rotate_left_64() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_1[26:0] ++ hdr.st.s32_1[31:27]; }
    hdr.st.s32_5 = hdr.st.s32_0[15:0] ++ hdr.st.s32_0[31:16];
    hdr.st.s32_6 = (hdr.st.s32_4) ^ (hdr.st.s32_2);
  }

  action compute_rotate_left_67_x() {
    hdr.st.s32_0 = (hdr.st.s32_3) ^ (hdr.st.s32_0);
    @in_hash { hdr.st.s32_1 = hdr.st.s32_6[24:0] ++ hdr.st.s32_6[31:25]; }
  }

  action compute_rotate_left_67() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_0[18:0] ++ hdr.st.s32_0[31:19]; }
  }

  action compute_rotate_left_82_x() {
    hdr.st.s32_3 = (hdr.st.s32_4) ^ (hdr.st.s32_3);
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.st.s32_5;
  }

  action compute_rotate_left_82_shl() {
    hdr.st.s32_5 = hdr.st.s32_3 << 5;
    hdr.st.s32_6 = hdr.st.s32_3 >> 27;
    hdr.st.s32_1 = hdr.st.s32_1 + hdr.st.s32_2;
  }

  action compute_rotate_left_82_or() {
    hdr.st.s32_5 = hdr.st.s32_5 | hdr.st.s32_6;
    hdr.st.s32_2 = (hdr.st.s32_8) ^ (hdr.st.s32_1);
    hdr.st.s32_7 = (hdr.st.s32_1) + (hdr.st.s32_3);
  }

  action compute_rotate_left_83() {
    hdr.st.s32_3 = hdr.st.s32_2[23:0] ++ hdr.st.s32_2[31:24];
    hdr.st.s32_4 = hdr.st.s32_7[15:0] ++ hdr.st.s32_7[31:16];
    hdr.st.s32_5 = (hdr.st.s32_5) ^ (hdr.st.s32_7);
    hdr.st.s32_0 = hdr.st.s32_0 + hdr.st.s32_2;
  }

  action compute_rotate_left_85_shl() {
    hdr.st.s32_1 = hdr.st.s32_5 << 13;
    hdr.st.s32_6 = hdr.st.s32_5 >> 19;
    hdr.st.s32_2 = (hdr.st.s32_3) ^ (hdr.st.s32_0);
    hdr.st.s32_8 = (hdr.st.s32_0) + (hdr.st.s32_5);
  }

  action compute_rotate_left_85_or() {
    hdr.st.s32_1 = hdr.st.s32_1 | hdr.st.s32_6;
    hdr.st.s32_7 = hdr.st.s32_2 << 7;
    hdr.st.s32_9 = hdr.st.s32_2 >> 25;
    hdr.st.s32_10 = hdr.st.s32_8[15:0] ++ hdr.st.s32_8[31:16];
  }

  action compute_rotate_left_86_or() {
    hdr.st.s32_9 = hdr.st.s32_7 | hdr.st.s32_9;
  }


  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073939504_0) vector_register_1073939504_0_read_1737 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action compute_rotate_left_107() {
    hdr.st.s32_0 = 32w2225785509;
  }

  action compute_rotate_left_108_x() {
    hdr.st.s32_1 = (32w0x3b355c4b) ^ (hdr.hdr1.data3);
  }

  action compute_rotate_left_108_x_k() {
    hdr.st.s32_2 = 32w3399118710;
  }

  action compute_rotate_left_108() {
    hdr.st.s32_3 = hdr.st.s32_1[23:0] ++ hdr.st.s32_1[31:24];
    hdr.st.s32_0 = (32w0x6f76ca9a) ^ (hdr.st.s32_0);
    hdr.st.s32_4 = 32w0x5d476351 + hdr.st.s32_1;
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
    hdr.st.s32_1 = (32w0x5d476351) + (hdr.st.s32_1);
  }

  action compute_rotate_left_112() {
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_5 = (hdr.st.s32_5) ^ (hdr.st.s32_1);
    hdr.st.s32_0 = (hdr.st.s32_0) ^ (hdr.st.s32_2);
  }

  action compute_rotate_left_113() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_5[26:0] ++ hdr.st.s32_5[31:27]; }
    hdr.st.s32_3 = hdr.st.s32_0[23:0] ++ hdr.st.s32_0[31:24];
    hdr.st.s32_2 = (hdr.st.s32_2) + (hdr.st.s32_5);
    hdr.st.s32_6 = hdr.st.s32_6 + hdr.st.s32_0;
  }

  action compute_rotate_left_115() {
    hdr.st.s32_4 = hdr.st.s32_2[15:0] ++ hdr.st.s32_2[31:16];
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_2);
    hdr.st.s32_0 = (hdr.st.s32_3) ^ (hdr.st.s32_6);
  }

  action compute_rotate_left_116() {
    @in_hash { hdr.st.s32_2 = hdr.st.s32_1[18:0] ++ hdr.st.s32_1[31:19]; }
  }

  action compute_rotate_left_117() {
    @in_hash { hdr.st.s32_5 = hdr.st.s32_0[24:0] ++ hdr.st.s32_0[31:25]; }
    hdr.st.s32_7 = (hdr.st.s32_6) + (hdr.st.s32_1);
  }

  action compute_rotate_left_118() {
    hdr.st.s32_7 = hdr.st.s32_7[15:0] ++ hdr.st.s32_7[31:16];
  }

  action compute_op_add_396() {
    hdr.st.s32_1 = hdr.st.s32_3 + hdr.st.s32_7;
    hdr.st.s32_8 = hdr.st.s32_8 ^ hdr.st.s32_3;
  }

  action compute_rotate_left_143_x() {
    hdr.st.s32_0 = (hdr.st.s32_0) ^ (hdr.st.s32_1);
    hdr.st.s32_9 = hdr.st.s32_9 + hdr.st.s32_8;
  }

  action compute_op_xor_399() {
    hdr.st.s32_2 = hdr.st.s32_2 ^ hdr.st.s32_9;
    hdr.st.s32_6 = hdr.st.s32_9 ^ hdr.st.s32_6;
  }

  action compute_rotate_left_143() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_0[26:0] ++ hdr.st.s32_0[31:27]; }
    hdr.st.s32_6 = (hdr.st.s32_6) + (hdr.st.s32_0);
  }

  action compute_rotate_left_144_x() {
    @in_hash { hdr.st.s32_9 = (hdr.st.s32_2) ^ (hdr.hdr2.data1); }
    hdr.st.s32_3 = hdr.st.s32_6[15:0] ++ hdr.st.s32_6[31:16];
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_6);
  }

  action compute_rotate_left_144() {
    hdr.st.s32_7 = hdr.st.s32_9[23:0] ++ hdr.st.s32_9[31:24];
    @in_hash { hdr.st.s32_4 = hdr.st.s32_1[18:0] ++ hdr.st.s32_1[31:19]; }
    hdr.st.s32_10 = hdr.st.s32_10 + hdr.st.s32_9;
  }

  action compute_rotate_left_147_x() {
    hdr.st.s32_5 = (hdr.st.s32_7) ^ (hdr.st.s32_10);
    hdr.st.s32_1 = (hdr.st.s32_10) + (hdr.st.s32_1);
  }

  action compute_rotate_left_148() {
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_4 = (hdr.st.s32_4) ^ (hdr.st.s32_1);
    hdr.st.s32_3 = hdr.st.s32_3 + hdr.st.s32_5;
  }

  action compute_rotate_left_147() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_5[24:0] ++ hdr.st.s32_5[31:25]; }
  }

  action compute_rotate_left_149() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.st.s32_2 = (hdr.st.s32_3) + (hdr.st.s32_4);
  }

  action compute_rotate_left_150_x() {
    hdr.st.s32_0 = (hdr.st.s32_0) ^ (hdr.st.s32_3);
    hdr.st.s32_4 = hdr.st.s32_2[15:0] ++ hdr.st.s32_2[31:16];
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_2);
  }

  action compute_rotate_left_150() {
    hdr.st.s32_3 = hdr.st.s32_0[23:0] ++ hdr.st.s32_0[31:24];
    @in_hash { hdr.st.s32_2 = hdr.st.s32_1[18:0] ++ hdr.st.s32_1[31:19]; }
    hdr.st.s32_6 = hdr.st.s32_6 + hdr.st.s32_0;
  }

  action compute_rotate_left_153_x() {
    hdr.st.s32_0 = (hdr.st.s32_3) ^ (hdr.st.s32_6);
    hdr.st.s32_1 = (hdr.st.s32_6) + (hdr.st.s32_1);
  }

  action compute_rotate_left_153() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_0[24:0] ++ hdr.st.s32_0[31:25]; }
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_1);
    hdr.st.s32_4 = hdr.st.s32_4 + hdr.st.s32_0;
  }

  action compute_rotate_left_155() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_2[26:0] ++ hdr.st.s32_2[31:27]; }
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_4);
  }

  action compute_op_xor_417() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_4 ^ hdr.hdr2.data1; }
  }

  action compute_rotate_left_156() {
    hdr.st.s32_4 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_1 = (hdr.st.s32_1) + (hdr.st.s32_2);
    hdr.st.s32_7 = hdr.st.s32_6 + hdr.st.s32_3;
  }

  action compute_rotate_left_157() {
    hdr.st.s32_6 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_0 = (hdr.st.s32_0) ^ (hdr.st.s32_1);
    hdr.st.s32_2 = (hdr.st.s32_4) ^ (hdr.st.s32_7);
  }

  action compute_rotate_left_158() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_0[18:0] ++ hdr.st.s32_0[31:19]; }
  }

  action compute_rotate_left_159() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_2[24:0] ++ hdr.st.s32_2[31:25]; }
    hdr.st.s32_8 = (hdr.st.s32_7) + (hdr.st.s32_0);
  }

  action compute_rotate_left_160() {
    hdr.st.s32_10 = hdr.st.s32_8[15:0] ++ hdr.st.s32_8[31:16];
  }

  action compute_op_xor_457() {
    meta.op_xor_457_out = hdr.recirc.f32_2 ^ hdr.recirc.f32_3;
  }

  action swap_action_180() {
    hdr.hdr2.data0 = hdr.hdr2.data0[15:0] ++ hdr.hdr2.data0[31:16];
  }
  action hdr_val0_calc() { meta.hdr_val0 = (hdr.recirc.f32_0) ^ (meta.op_xor_457_out); }

  action hdr_val1_calc() { meta.hdr_val1 = (32w0x00000001) + (hdr.hdr2.data1); }

  action hdr_val2_calc() { meta.hdr_val2 = (hdr.hdr2.data3[7:0]) | (8w0x12); }

  action swap_action_181() {
    swap32(hdr.hdr1.data3, hdr.hdr1.data4);
  }

  apply {
    meta.lap1 = 0;
    hdr.st.setValid();

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  356197:ArithmeticOp
        // BDD node 281:op_add
        compute_op_add_281();
        compute_rotate_left_143_x();
        compute_op_xor_399();
        compute_rotate_left_143();
        compute_op_add_285_b();
        compute_rotate_left_144_x();
        compute_op_add_285();
        compute_rotate_left_144();
        compute_rotate_left_53_x();
        compute_rotate_left_53();
        compute_rotate_left_56_x();
        compute_rotate_left_56();
        compute_rotate_left_59_x();
        compute_rotate_left_59();
        compute_rotate_left_58();
        compute_rotate_left_61_x();
        compute_rotate_left_61();
        compute_rotate_left_63();
        compute_rotate_left_66_x();
        compute_rotate_left_64();
        compute_rotate_left_67_x();
        compute_rotate_left_67();
        // EP node  358722:RotateLeft
        // BDD node 52:rotate_left
        // EP node  362515:ArithmeticOp
        // BDD node 282:op_xor
        // EP node  364740:ArithmeticOp
        // BDD node 283:op_add
        // EP node  367289:ArithmeticOp
        // BDD node 284:op_xor
        // EP node  369528:ArithmeticOp
        // BDD node 285:op_add
        // EP node  372093:RotateLeft
        // BDD node 53:rotate_left
        // EP node  374346:ArithmeticOp
        // BDD node 286:op_xor
        // EP node  376607:ArithmeticOp
        // BDD node 287:op_xor
        // EP node  379197:RotateLeft
        // BDD node 54:rotate_left
        // EP node  381472:ArithmeticOp
        // BDD node 288:op_add
        // EP node  384078:RotateLeft
        // BDD node 55:rotate_left
        // EP node  387667:ArithmeticOp
        // BDD node 289:op_xor
        // EP node  389963:ArithmeticOp
        // BDD node 290:op_add
        // EP node  392593:RotateLeft
        // BDD node 56:rotate_left
        // EP node  396215:ArithmeticOp
        // BDD node 291:op_xor
        // EP node  398861:RotateLeft
        // BDD node 57:rotate_left
        // EP node  401185:ArithmeticOp
        // BDD node 292:op_add
        // EP node  403847:RotateLeft
        // BDD node 58:rotate_left
        // EP node  407513:ArithmeticOp
        // BDD node 293:op_xor
        // EP node  409858:ArithmeticOp
        // BDD node 294:op_add
        // EP node  412544:RotateLeft
        // BDD node 59:rotate_left
        // EP node  414903:ArithmeticOp
        // BDD node 295:op_xor
        // EP node  417605:RotateLeft
        // BDD node 60:rotate_left
        // EP node  419978:ArithmeticOp
        // BDD node 296:op_add
        // EP node  422696:RotateLeft
        // BDD node 61:rotate_left
        // EP node  426439:ArithmeticOp
        // BDD node 297:op_xor
        // EP node  428833:ArithmeticOp
        // BDD node 298:op_add
        // EP node  431575:RotateLeft
        // BDD node 62:rotate_left
        // EP node  435351:ArithmeticOp
        // BDD node 299:op_xor
        // EP node  438109:RotateLeft
        // BDD node 63:rotate_left
        // EP node  440531:ArithmeticOp
        // BDD node 300:op_add
        // EP node  443305:RotateLeft
        // BDD node 64:rotate_left
        // EP node  447125:ArithmeticOp
        // BDD node 301:op_xor
        // EP node  449568:ArithmeticOp
        // BDD node 302:op_add
        // EP node  452714:RotateLeft
        // BDD node 65:rotate_left
        // EP node  455520:ArithmeticOp
        // BDD node 303:op_xor
        // EP node  457984:ArithmeticOp
        // BDD node 304:op_xor
        // EP node  460806:RotateLeft
        // BDD node 66:rotate_left
        // EP node  463284:ArithmeticOp
        // BDD node 305:op_add
        // EP node  466122:RotateLeft
        // BDD node 67:rotate_left
        // EP node  468967:ArithmeticOp
        // BDD node 306:op_xor
        // EP node  471466:ArithmeticOp
        // BDD node 307:op_add
        // EP node  474328:RotateLeft
        // BDD node 68:rotate_left
        // EP node  478269:ArithmeticOp
        // BDD node 308:op_xor
        // EP node  480788:SendToEgress
        // BDD node 69:rotate_left
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 1;
        hdr.egress_state.bf_1073926928_estimate = hdr.egress_state.bf_1073926928_estimate;
        hdr.egress_state.vector_reg_value0 = hdr.egress_state.vector_reg_value0;
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(1);
      } else if (hdr.recirc.code_path == 1) {
        // EP node  547848:RotateLeftShifts
        // BDD node 82:rotate_left
        compute_rotate_left_82_x();
        compute_rotate_left_82_shl();
        compute_rotate_left_82_or();
        compute_rotate_left_83();
        compute_rotate_left_85_shl();
        compute_rotate_left_85_or();
        compute_rotate_left_86_or();
        // EP node  550207:ArithmeticOp
        // BDD node 326:op_xor
        // EP node  552572:ArithmeticOp
        // BDD node 327:op_add
        // EP node  555336:RotateLeft
        // BDD node 83:rotate_left
        // EP node  557713:ArithmeticOp
        // BDD node 328:op_xor
        // EP node  560491:RotateLeft
        // BDD node 84:rotate_left
        // EP node  562880:ArithmeticOp
        // BDD node 329:op_add
        // EP node  566069:RotateLeftShifts
        // BDD node 85:rotate_left
        // EP node  568869:ArithmeticOp
        // BDD node 330:op_xor
        // EP node  571676:ArithmeticOp
        // BDD node 331:op_add
        // EP node  575290:RotateLeftShifts
        // BDD node 86:rotate_left
        // EP node  578111:ArithmeticOp
        // BDD node 332:op_xor
        // EP node  581341:RotateLeft
        // BDD node 87:rotate_left
        // EP node  583772:SendToEgress
        // BDD node 333:op_lshr
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 2;
        hdr.egress_state.bf_1073926928_estimate = hdr.egress_state.bf_1073926928_estimate;
        hdr.egress_state.vector_reg_value0 = hdr.egress_state.vector_reg_value0;
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(2);
      } else if (hdr.recirc.code_path == 2) {
        // EP node  592724:ArithmeticOp
        // BDD node 337:op_xor
        // EP node  593966:Recirculate
        // BDD node 338:op_add
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(3);
      } else if (hdr.recirc.code_path == 3) {
        // EP node  595632:SendToController
        // BDD node 338:op_add
        fwd_op = fwd_op_t.FORWARD_TO_CPU;
        build_cpu_hdr(0);
        hdr.cpu.bf_1073926928_estimate = hdr.egress_state.bf_1073926928_estimate;
        hdr.cpu.f32_0 = hdr.recirc.f32_0;
        hdr.cpu.f32_1 = hdr.recirc.f32_1;
        @in_hash { hdr.cpu.rotated_86 = hdr.st.s32_9; }     // m11
        @in_hash { hdr.cpu.unrolled_101 = hdr.st.s32_5; }
        @in_hash { hdr.cpu.rotated_87 = hdr.st.s32_10; }
        @in_hash { hdr.cpu.unrolled_106 = hdr.st.s32_2; }
        @in_hash { hdr.cpu.rotated_84 = hdr.st.s32_4; }
        @in_hash { hdr.cpu.unrolled_100 = hdr.st.s32_0; }
        @in_hash { hdr.cpu.rotated_85 = hdr.st.s32_1; }
      } else if (hdr.recirc.code_path == 4) {
        // EP node  43277:ArithmeticOp
        // BDD node 396:op_add
        compute_op_add_396();
        compute_rotate_left_143_x();
        compute_op_xor_399();
        compute_rotate_left_143();
        compute_rotate_left_144_x();
        compute_rotate_left_144();
        compute_rotate_left_147_x();
        compute_rotate_left_148();
        compute_rotate_left_147();
        compute_rotate_left_149();
        compute_rotate_left_150_x();
        compute_rotate_left_150();
        compute_rotate_left_153_x();
        compute_rotate_left_153();
        compute_rotate_left_155();
        compute_op_xor_417();
        compute_rotate_left_156();
        compute_rotate_left_157();
        compute_rotate_left_158();
        compute_rotate_left_159();
        compute_rotate_left_160();
        // EP node  44010:RotateLeft
        // BDD node 143:rotate_left
        // EP node  44991:ArithmeticOp
        // BDD node 397:op_xor
        // EP node  45614:ArithmeticOp
        // BDD node 398:op_add
        // EP node  46365:ArithmeticOp
        // BDD node 399:op_xor
        // EP node  47246:RotateLeft
        // BDD node 144:rotate_left
        // EP node  48009:ArithmeticOp
        // BDD node 400:op_xor
        // EP node  48652:ArithmeticOp
        // BDD node 401:op_xor
        // EP node  49555:RotateLeft
        // BDD node 145:rotate_left
        // EP node  50337:ArithmeticOp
        // BDD node 402:op_add
        // EP node  51254:RotateLeft
        // BDD node 146:rotate_left
        // EP node  52438:ArithmeticOp
        // BDD node 403:op_xor
        // EP node  53238:ArithmeticOp
        // BDD node 404:op_add
        // EP node  54176:RotateLeft
        // BDD node 147:rotate_left
        // EP node  55387:ArithmeticOp
        // BDD node 405:op_xor
        // EP node  56339:RotateLeft
        // BDD node 148:rotate_left
        // EP node  57163:ArithmeticOp
        // BDD node 406:op_add
        // EP node  58129:RotateLeft
        // BDD node 149:rotate_left
        // EP node  59376:ArithmeticOp
        // BDD node 407:op_xor
        // EP node  60218:ArithmeticOp
        // BDD node 408:op_add
        // EP node  61205:RotateLeft
        // BDD node 150:rotate_left
        // EP node  62059:ArithmeticOp
        // BDD node 409:op_xor
        // EP node  63060:RotateLeft
        // BDD node 151:rotate_left
        // EP node  63926:ArithmeticOp
        // BDD node 410:op_add
        // EP node  64941:RotateLeft
        // BDD node 152:rotate_left
        // EP node  66251:ArithmeticOp
        // BDD node 411:op_xor
        // EP node  67135:ArithmeticOp
        // BDD node 412:op_add
        // EP node  68171:RotateLeft
        // BDD node 153:rotate_left
        // EP node  69508:ArithmeticOp
        // BDD node 413:op_xor
        // EP node  70558:RotateLeft
        // BDD node 154:rotate_left
        // EP node  71466:ArithmeticOp
        // BDD node 414:op_add
        // EP node  72530:RotateLeft
        // BDD node 155:rotate_left
        // EP node  73903:ArithmeticOp
        // BDD node 415:op_xor
        // EP node  74829:ArithmeticOp
        // BDD node 416:op_add
        // EP node  76067:RotateLeft
        // BDD node 156:rotate_left
        // EP node  77159:ArithmeticOp
        // BDD node 417:op_xor
        // EP node  78103:ArithmeticOp
        // BDD node 418:op_xor
        // EP node  79209:RotateLeft
        // BDD node 157:rotate_left
        // EP node  80165:ArithmeticOp
        // BDD node 419:op_add
        // EP node  81285:RotateLeft
        // BDD node 158:rotate_left
        // EP node  82730:ArithmeticOp
        // BDD node 420:op_xor
        // EP node  83704:ArithmeticOp
        // BDD node 421:op_add
        // EP node  84845:RotateLeft
        // BDD node 159:rotate_left
        // EP node  86317:ArithmeticOp
        // BDD node 422:op_xor
        // EP node  87472:RotateLeft
        // BDD node 160:rotate_left
        // EP node  88305:SendToEgress
        // BDD node 423:op_add
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.code_path = 4;
        hdr.egress_state.dev = hdr.egress_state.dev;
        hdr.egress_state.vector_reg_value1 = hdr.egress_state.vector_reg_value1;
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(5);
      } else if (hdr.recirc.code_path == 5) {
        // EP node  136949:ArithmeticOp
        // BDD node 457:op_xor
        compute_op_xor_457();
        // EP node  137622:ModifyHeader
        // BDD node 180:packet_return_chunk
        swap_action_180();
        hdr_val0_calc();
        hdr_val1_calc();
        hdr_val2_calc();
        hdr.hdr2.data1 = meta.hdr_val0;
        hdr.hdr2.data2 = meta.hdr_val1;
        hdr.hdr2.data3 = 8w0x50 ++ meta.hdr_val2;
        // EP node  138298:ModifyHeader
        // BDD node 181:packet_return_chunk
        swap_action_181();
        hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;
        meta.redo_checksum = 1;   // m10: BDD node 179 (nf_set_rte_ipv4_udptcp_checksum)
        meta.tcp_len = 20;
        // EP node  139656:Forward
        // BDD node 183:FORWARD
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
          // EP node  66:ParserCondition
          // BDD node 6:if
          // EP node  67:Then
          // BDD node 6:if
          // EP node  629368:Forward
          // BDD node 9:FORWARD
          @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]); }
          // EP node  68:Else
          // BDD node 6:if
          // EP node  103:ParserCondition
          // BDD node 10:if
          // EP node  104:Then
          // BDD node 10:if
          // EP node  135:ParserExtraction
          // BDD node 11:packet_borrow_next_chunk
          if(hdr.hdr2.isValid()) {
            // EP node  228:If
            // BDD node 12:if
            if ((16w0x0000) != (meta.dev[15:0])){
              // EP node  229:Then
              // BDD node 12:if
              // EP node  323:If
              // BDD node 13:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000002))){
                // EP node  324:Then
                // BDD node 13:if
                // EP node  538:BloomFilterQuery
                // BDD node 14:bf_query
                meta.bf_1073926928_estimate = 0;
                bf_1073926928_row_0_read_execute();
                bf_1073926928_row_1_read_execute();
                // EP node  833:If
                // BDD node 15:if
                if ((32w0x00000000) == (meta.bf_1073926928_estimate)){
              meta.lap1 = 1;
                  // EP node  834:Then
                  // BDD node 15:if
                  // EP node  141888:RotateLeft
                  // BDD node 16:rotate_left
                  // EP node  146605:VectorRegisterLookup
                  // BDD node 88:vector_borrow
                  meta.vector_reg_value0 = vector_register_1073939504_0_read_146605.execute(32w0x00000000);
                  // EP node  149317:Ignore
                  // BDD node 89:vector_return
                  // EP node  152268:RotateLeft
                  // BDD node 17:rotate_left
                  // EP node  154779:RotateLeft
                  // BDD node 18:rotate_left
                  // EP node  157073:RotateLeft
                  // BDD node 19:rotate_left
                  // EP node  160522:ArithmeticOp
                  // BDD node 231:op_xor
                  // EP node  162606:ArithmeticOp
                  // BDD node 232:op_add
                  // EP node  164930:RotateLeft
                  // BDD node 20:rotate_left
                  // EP node  168424:ArithmeticOp
                  // BDD node 233:op_xor
                  // EP node  170535:ArithmeticOp
                  // BDD node 234:op_add
                  // EP node  172889:RotateLeft
                  // BDD node 21:rotate_left
                  // EP node  175018:ArithmeticOp
                  // BDD node 235:op_add
                  // EP node  177392:RotateLeft
                  // BDD node 22:rotate_left
                  // EP node  180961:ArithmeticOp
                  // BDD node 236:op_xor
                  // EP node  183117:ArithmeticOp
                  // BDD node 237:op_add
                  // EP node  185521:RotateLeft
                  // BDD node 23:rotate_left
                  // EP node  187695:ArithmeticOp
                  // BDD node 238:op_xor
                  // EP node  190119:RotateLeft
                  // BDD node 24:rotate_left
                  // EP node  192311:ArithmeticOp
                  // BDD node 239:op_add
                  // EP node  194755:RotateLeft
                  // BDD node 25:rotate_left
                  // EP node  198429:ArithmeticOp
                  // BDD node 240:op_xor
                  // EP node  200648:ArithmeticOp
                  // BDD node 241:op_add
                  // EP node  203122:RotateLeft
                  // BDD node 26:rotate_left
                  // EP node  206841:ArithmeticOp
                  // BDD node 242:op_xor
                  // EP node  209335:RotateLeft
                  // BDD node 27:rotate_left
                  // EP node  211340:SendToEgress
                  // BDD node 243:op_add
                  meta.to_egress = 1;
                  hdr.egress_state.setValid();
                  hdr.egress_state.code_path = 0;
                  hdr.egress_state.bf_1073926928_estimate = meta.bf_1073926928_estimate;
                  hdr.egress_state.vector_reg_value0 = meta.vector_reg_value0;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(0);
                } else {
                  // EP node  835:Else
                  // BDD node 15:if
                  // EP node  1259:Forward
                  // BDD node 103:FORWARD
                  nf_dev[15:0] = 16w0x0000;
                }
              } else {
                // EP node  325:Else
                // BDD node 13:if
                // EP node  1496:If
                // BDD node 104:if
                if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000010))){
              meta.lap1 = 1;
                  // EP node  1497:Then
                  // BDD node 104:if
                  // EP node  1737:VectorRegisterLookup
                  // BDD node 105:vector_borrow
                  meta.vector_reg_value1 = vector_register_1073939504_0_read_1737.execute(32w0x00000000);
                  // EP node  2229:Ignore
                  // BDD node 106:vector_return
                  // EP node  2552:RotateLeft
                  // BDD node 107:rotate_left
                  // EP node  3110:RotateLeft
                  // BDD node 108:rotate_left
                  // EP node  3421:RotateLeft
                  // BDD node 109:rotate_left
                  // EP node  3707:RotateLeft
                  // BDD node 110:rotate_left
                  // EP node  4106:ArithmeticOp
                  // BDD node 346:op_xor
                  // EP node  4372:ArithmeticOp
                  // BDD node 347:op_add
                  // EP node  4682:RotateLeft
                  // BDD node 111:rotate_left
                  // EP node  5114:ArithmeticOp
                  // BDD node 348:op_xor
                  // EP node  5401:ArithmeticOp
                  // BDD node 349:op_add
                  // EP node  5735:RotateLeft
                  // BDD node 112:rotate_left
                  // EP node  6036:ArithmeticOp
                  // BDD node 350:op_add
                  // EP node  6386:RotateLeft
                  // BDD node 113:rotate_left
                  // EP node  6873:ArithmeticOp
                  // BDD node 351:op_xor
                  // EP node  7195:ArithmeticOp
                  // BDD node 352:op_add
                  // EP node  7569:RotateLeft
                  // BDD node 114:rotate_left
                  // EP node  7905:ArithmeticOp
                  // BDD node 353:op_xor
                  // EP node  8295:RotateLeft
                  // BDD node 115:rotate_left
                  // EP node  8645:ArithmeticOp
                  // BDD node 354:op_add
                  // EP node  9051:RotateLeft
                  // BDD node 116:rotate_left
                  // EP node  9615:ArithmeticOp
                  // BDD node 355:op_xor
                  // EP node  9986:ArithmeticOp
                  // BDD node 356:op_add
                  // EP node  10416:RotateLeft
                  // BDD node 117:rotate_left
                  // EP node  11013:ArithmeticOp
                  // BDD node 357:op_xor
                  // EP node  11459:RotateLeft
                  // BDD node 118:rotate_left
                  // EP node  11802:SendToEgress
                  // BDD node 358:op_add
                  meta.to_egress = 1;
                  hdr.egress_state.setValid();
                  hdr.egress_state.code_path = 3;
                  hdr.egress_state.dev = meta.dev;
                  hdr.egress_state.vector_reg_value1 = meta.vector_reg_value1;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(4);
                } else {
                  // EP node  1498:Else
                  // BDD node 104:if
                  // EP node  636885:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
          if (meta.lap1 == 1) {
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
              // EP node  230:Else
              // BDD node 12:if
              // EP node  596480:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000040))){
                // EP node  596481:Then
                // BDD node 188:if
                // EP node  601624:Forward
                // BDD node 192:FORWARD
                @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]); }
              } else {
                // EP node  596482:Else
                // BDD node 188:if
                // EP node  602053:BloomFilterSet
                // BDD node 193:bf_set
                bf_1073926928_row_0_set_to_one_execute();
                bf_1073926928_row_1_set_to_one_execute();
                // EP node  608939:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  105:Else
          // BDD node 10:if
          // EP node  630246:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  611518:ParserCondition
          // BDD node 201:if
          // EP node  611519:Then
          // BDD node 201:if
          // EP node  618461:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  626737:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev[15:0])){
              // EP node  626738:Then
              // BDD node 203:if
              // EP node  633335:ParserCondition
              // BDD node 204:if
              // EP node  633336:Then
              // BDD node 204:if
              // EP node  638219:ParserCondition
              // BDD node 205:if
              // EP node  638220:Then
              // BDD node 205:if
              // EP node  656704:Forward
              // BDD node 209:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]); }
              // EP node  638221:Else
              // BDD node 205:if
              // EP node  644050:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  651243:SendToController
                // BDD node 211:vector_borrow
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(1);
              }
              // EP node  633337:Else
              // BDD node 204:if
              // EP node  655792:Forward
              // BDD node 221:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]); }
            } else {
              // EP node  626739:Else
              // BDD node 203:if
              // EP node  653515:Forward
              // BDD node 225:FORWARD
              @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]); }
            }
          }
          // EP node  611520:Else
          // BDD node 201:if
          // EP node  635997:Forward
          // BDD node 228:FORWARD
          @in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]); }
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  617593:ParserReject
        // BDD node 230:DROP
      }

    }

    forwarding_tbl.apply();
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
  Checksum() ipv4_checksum;   // m10
  Checksum() tcp_checksum;

  apply {
    if (meta.redo_checksum == 1) {
      hdr.hdr1.data2_csum = ipv4_checksum.update({
        hdr.hdr1.data0, hdr.hdr1.data1, hdr.hdr1.data2_ttl, hdr.hdr1.data2_proto, hdr.hdr1.data3, hdr.hdr1.data4
      });
      hdr.hdr2.data5_csum = tcp_checksum.update({
        hdr.hdr1.data3, hdr.hdr1.data4, 8w0, hdr.hdr1.data2_proto, meta.tcp_len,
        hdr.hdr2.data0, hdr.hdr2.data1, hdr.hdr2.data2, hdr.hdr2.data3, hdr.hdr2.data4, hdr.hdr2.data5_urg
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

  action compute_op_add_243() {
    hdr.st.s32_1 = hdr.st.s32_6 + hdr.st.s32_1;
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_6;
  }

  action compute_rotate_left_69_x() {
    hdr.st.s32_6 = (hdr.st.s32_2) + (hdr.st.s32_0);
    hdr.st.s32_4 = hdr.st.s32_4 ^ hdr.st.s32_2;
  }

  action compute_rotate_left_69() {
    hdr.st.s32_0 = hdr.st.s32_6[15:0] ++ hdr.st.s32_6[31:16];
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_6);
    hdr.st.s32_5 = hdr.st.s32_5 + hdr.st.s32_4;
  }

  action compute_rotate_left_71_x() {
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_5);
    hdr.st.s32_4 = (hdr.st.s32_5) + (hdr.st.s32_3);
  }

  action compute_rotate_left_71() {
    hdr.st.s32_8 = hdr.st.s32_1[23:0] ++ hdr.st.s32_1[31:24];
    hdr.st.s32_9 = hdr.st.s32_4[15:0] ++ hdr.st.s32_4[31:16];
    hdr.st.s32_7 = hdr.st.s32_0 + hdr.st.s32_1;
  }

  action compute_rotate_left_74_x() {
    hdr.st.s32_8 = (hdr.st.s32_8) ^ (hdr.st.s32_7);
  }

  action compute_rotate_left_74_shl() {
    hdr.st.s32_0 = hdr.st.s32_8 << 7;
    hdr.st.s32_2 = hdr.st.s32_8 >> 25;
    hdr.st.s32_9 = hdr.st.s32_9 + hdr.st.s32_8;
  }

  action compute_rotate_left_74_or() {
    hdr.st.s32_6 = hdr.st.s32_0 | hdr.st.s32_2;
  }

  action compute_rotate_left_77_x() {
    hdr.st.s32_6 = (hdr.st.s32_6) ^ (hdr.st.s32_9);
  }

  action compute_rotate_left_77() {
    hdr.st.s32_2 = hdr.st.s32_6[23:0] ++ hdr.st.s32_6[31:24];
  }

  action compute_rotate_left_70() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_3[26:0] ++ hdr.st.s32_3[31:27]; }
  }

  action compute_rotate_left_73_x() {
    hdr.st.s32_4 = (hdr.st.s32_0) ^ (hdr.st.s32_4);
  }

  action compute_rotate_left_73_shl() {
    hdr.st.s32_0 = hdr.st.s32_4 << 13;
    hdr.st.s32_3 = hdr.st.s32_4 >> 19;
    hdr.st.s32_7 = (hdr.st.s32_7) + (hdr.st.s32_4);
  }

  action compute_rotate_left_73_or() {
    hdr.st.s32_3 = hdr.st.s32_0 | hdr.st.s32_3;
    hdr.st.s32_5 = hdr.st.s32_7[15:0] ++ hdr.st.s32_7[31:16];
  }

  action compute_rotate_left_76_x() {
    hdr.st.s32_7 = (hdr.st.s32_3) ^ (hdr.st.s32_7);
    hdr.st.s32_5 = hdr.st.s32_5 + hdr.st.s32_6;
  }

  action compute_rotate_left_76_shl() {
    hdr.st.s32_0 = hdr.st.s32_7 << 5;
    hdr.st.s32_6 = hdr.st.s32_7 >> 27;
    hdr.st.s32_9 = (hdr.st.s32_9) + (hdr.st.s32_7);
    hdr.st.s32_3 = (hdr.st.s32_2) ^ (hdr.st.s32_5);
  }

  action compute_rotate_left_76_or() {
    hdr.st.s32_6 = hdr.st.s32_0 | hdr.st.s32_6;
    hdr.st.s32_1 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_8 = hdr.st.s32_3 << 7;
    hdr.st.s32_10 = hdr.st.s32_3 >> 25;
  }

  action compute_rotate_left_79_x() {
    hdr.st.s32_6 = (hdr.st.s32_6) ^ (hdr.st.s32_9);
    hdr.st.s32_8 = hdr.st.s32_8 | hdr.st.s32_10;
  }

  action compute_rotate_left_79_shl() {
    hdr.st.s32_4 = hdr.st.s32_6 << 13;
    hdr.st.s32_7 = hdr.st.s32_6 >> 19;
    hdr.st.s32_3 = (hdr.st.s32_5) + (hdr.st.s32_6);
  }

  action compute_rotate_left_79_or() {
    hdr.st.s32_4 = hdr.st.s32_4 | hdr.st.s32_7;
    hdr.st.s32_0 = hdr.st.s32_3[15:0] ++ hdr.st.s32_3[31:16];
  }

  action compute_op_add_336() {
    eg_md.op_add_336_out = 32w0xffffffff + hdr.hdr2.data2;
  }

  action compute_op_sub_334() {
    eg_md.op_sub_448_out = eg_md.op_lshr_447_out[31:0] - hdr.egress_state.vector_reg_value0;
  }

  action compute_op_lshr_333() {   // m11
    @in_hash { eg_md.op_lshr_447_out = eg_md.time; }
  }

  action compute_op_lshr_335() {   // m11
    eg_md.op_lshr_449_out = eg_md.op_sub_448_out >> 32w0x0000000c;
  }

  action compute_op_add_358() {
    hdr.st.s32_1 = hdr.st.s32_6 + hdr.st.s32_1;
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_6;
    hdr.st.s32_0 = hdr.hdr2.data0 >> 16;
  }

  action compute_rotate_left_119_x() {
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_1);
    hdr.st.s32_4 = hdr.st.s32_4 + hdr.st.s32_3;
    hdr.st.s32_0 = hdr.st.s32_0 << 32w0x00000010;
    hdr.st.s32_6 = (bit<32>)(hdr.hdr2.data0[15:0]);
  }

  action compute_rotate_left_119() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_2[26:0] ++ hdr.st.s32_2[31:27]; }
    hdr.st.s32_5 = hdr.st.s32_5 ^ hdr.st.s32_4;
  }

  action compute_op_xor_362() {
    hdr.st.s32_8 = hdr.st.s32_4 ^ hdr.hdr1.data3;
    hdr.st.s32_6 = hdr.st.s32_0 | hdr.st.s32_6;
  }

  action compute_rotate_left_120_x() {
    hdr.st.s32_0 = (hdr.st.s32_5) ^ (hdr.hdr1.data4);
    hdr.st.s32_3 = (hdr.st.s32_8) + (hdr.st.s32_2);
  }

  action compute_rotate_left_120() {
    hdr.st.s32_2 = hdr.st.s32_0[23:0] ++ hdr.st.s32_0[31:24];
    hdr.st.s32_5 = hdr.st.s32_3[15:0] ++ hdr.st.s32_3[31:16];
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_3);
    hdr.st.s32_7 = hdr.st.s32_7 + hdr.st.s32_0;
  }

  action compute_rotate_left_122() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_1[18:0] ++ hdr.st.s32_1[31:19]; }
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_7);
    hdr.st.s32_1 = (hdr.st.s32_7) + (hdr.st.s32_1);
  }

  action compute_rotate_left_123() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_2[24:0] ++ hdr.st.s32_2[31:25]; }
    hdr.st.s32_4 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_7 = (hdr.st.s32_0) ^ (hdr.st.s32_1);
    hdr.st.s32_5 = hdr.st.s32_5 + hdr.st.s32_2;
  }

  action compute_rotate_left_125() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_7[26:0] ++ hdr.st.s32_7[31:27]; }
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_5);
    hdr.st.s32_7 = (hdr.st.s32_5) + (hdr.st.s32_7);
  }

  action compute_rotate_left_126() {
    hdr.st.s32_2 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_5 = hdr.st.s32_7[15:0] ++ hdr.st.s32_7[31:16];
    hdr.st.s32_1 = (hdr.st.s32_0) ^ (hdr.st.s32_7);
    hdr.st.s32_4 = hdr.st.s32_4 + hdr.st.s32_3;
  }

  action compute_rotate_left_128() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_1[18:0] ++ hdr.st.s32_1[31:19]; }
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_4);
    hdr.st.s32_1 = (hdr.st.s32_4) + (hdr.st.s32_1);
  }

  action compute_rotate_left_129() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_2[24:0] ++ hdr.st.s32_2[31:25]; }
    hdr.st.s32_4 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_7 = (hdr.st.s32_0) ^ (hdr.st.s32_1);
    hdr.st.s32_5 = hdr.st.s32_5 + hdr.st.s32_2;
  }

  action compute_rotate_left_131() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_7[26:0] ++ hdr.st.s32_7[31:27]; }
    hdr.st.s32_3 = hdr.st.s32_3 ^ hdr.st.s32_5;
  }

  action compute_op_xor_382() {
    hdr.st.s32_1 = hdr.st.s32_5 ^ hdr.hdr1.data4;
  }

  action compute_rotate_left_132_x() {
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_6);
    hdr.st.s32_1 = (hdr.st.s32_1) + (hdr.st.s32_7);
  }

  action compute_rotate_left_132() {
    hdr.st.s32_2 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_5 = hdr.st.s32_1[15:0] ++ hdr.st.s32_1[31:16];
    hdr.st.s32_7 = (hdr.st.s32_0) ^ (hdr.st.s32_1);
    hdr.st.s32_4 = hdr.st.s32_4 + hdr.st.s32_3;
  }

  action compute_rotate_left_134() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_7[18:0] ++ hdr.st.s32_7[31:19]; }
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_4);
    hdr.st.s32_7 = (hdr.st.s32_4) + (hdr.st.s32_7);
  }

  action compute_rotate_left_135() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_2[24:0] ++ hdr.st.s32_2[31:25]; }
    hdr.st.s32_3 = hdr.st.s32_7[15:0] ++ hdr.st.s32_7[31:16];
    hdr.st.s32_4 = (hdr.st.s32_0) ^ (hdr.st.s32_7);
    hdr.st.s32_5 = hdr.st.s32_5 + hdr.st.s32_2;
  }

  action compute_rotate_left_137() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_4[26:0] ++ hdr.st.s32_4[31:27]; }
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_5);
    hdr.st.s32_4 = (hdr.st.s32_5) + (hdr.st.s32_4);
  }

  action compute_rotate_left_138() {
    hdr.st.s32_8 = hdr.st.s32_1[23:0] ++ hdr.st.s32_1[31:24];
    hdr.st.s32_9 = hdr.st.s32_4[15:0] ++ hdr.st.s32_4[31:16];
    hdr.st.s32_7 = (hdr.st.s32_0) ^ (hdr.st.s32_4);
    hdr.st.s32_3 = hdr.st.s32_3 + hdr.st.s32_1;
  }

  action compute_rotate_left_140() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_7[18:0] ++ hdr.st.s32_7[31:19]; }
    hdr.st.s32_1 = (hdr.st.s32_8) ^ (hdr.st.s32_3);
    hdr.st.s32_4 = (hdr.st.s32_3) + (hdr.st.s32_7);
  }

  action compute_rotate_left_141() {
    @in_hash { hdr.st.s32_2 = hdr.st.s32_1[24:0] ++ hdr.st.s32_1[31:25]; }
    hdr.st.s32_10 = hdr.st.s32_4[15:0] ++ hdr.st.s32_4[31:16];
  }

  action compute_op_add_423() {
    hdr.st.s32_8 = hdr.st.s32_7 + hdr.st.s32_0;
    hdr.st.s32_4 = hdr.st.s32_4 ^ hdr.st.s32_7;
  }

  action compute_rotate_left_161_x() {
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_8);
    hdr.st.s32_6 = hdr.st.s32_6 + hdr.st.s32_4;
    @in_hash { eg_md.op_lshr_447_out = eg_md.time; }
  }

  action compute_rotate_left_162_x() {
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_6);
    hdr.st.s32_9 = (hdr.st.s32_6) + (hdr.st.s32_1);
    eg_md.op_sub_448_out = eg_md.op_lshr_447_out[31:0] - hdr.egress_state.vector_reg_value1;
  }

  action compute_rotate_left_162() {
    hdr.st.s32_2 = hdr.st.s32_3[23:0] ++ hdr.st.s32_3[31:24];
    hdr.st.s32_0 = hdr.st.s32_9[15:0] ++ hdr.st.s32_9[31:16];
    hdr.st.s32_4 = hdr.st.s32_10 + hdr.st.s32_3;
    eg_md.op_lshr_449_out = eg_md.op_sub_448_out >> 32w0x0000000c;
  }

  action compute_rotate_left_161() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_1[26:0] ++ hdr.st.s32_1[31:27]; }
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_4);
  }

  action compute_rotate_left_164_x() {
    hdr.st.s32_3 = (hdr.st.s32_3) ^ (hdr.st.s32_9);
    hdr.st.s32_6 = hdr.st.s32_0 + hdr.st.s32_2;
  }

  action compute_rotate_left_164() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_3[18:0] ++ hdr.st.s32_3[31:19]; }
  }

  action compute_rotate_left_165() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_2[24:0] ++ hdr.st.s32_2[31:25]; }
    hdr.st.s32_4 = (hdr.st.s32_4) + (hdr.st.s32_3);
  }

  action compute_rotate_left_166() {
    hdr.st.s32_9 = hdr.st.s32_4[15:0] ++ hdr.st.s32_4[31:16];
    hdr.st.s32_7 = (hdr.st.s32_0) ^ (hdr.st.s32_4);
    hdr.st.s32_1 = (hdr.st.s32_1) ^ (hdr.st.s32_6);
  }

  action compute_rotate_left_167() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_7[26:0] ++ hdr.st.s32_7[31:27]; }
    hdr.st.s32_8 = hdr.st.s32_1[23:0] ++ hdr.st.s32_1[31:24];
    hdr.st.s32_10 = (hdr.st.s32_6) + (hdr.st.s32_7);
    hdr.st.s32_9 = hdr.st.s32_9 + hdr.st.s32_1;
  }

  action compute_rotate_left_169() {
    hdr.st.s32_1 = hdr.st.s32_10[15:0] ++ hdr.st.s32_10[31:16];
    hdr.st.s32_4 = (hdr.st.s32_0) ^ (hdr.st.s32_10);
    hdr.st.s32_8 = (hdr.st.s32_8) ^ (hdr.st.s32_9);
  }

  action compute_rotate_left_170() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
    hdr.st.s32_4 = (hdr.st.s32_9) + (hdr.st.s32_4);
    hdr.st.s32_1 = hdr.st.s32_1 + hdr.st.s32_8;
  }

  action compute_rotate_left_171() {
    @in_hash { hdr.st.s32_2 = hdr.st.s32_8[24:0] ++ hdr.st.s32_8[31:25]; }
    hdr.st.s32_9 = hdr.st.s32_4[15:0] ++ hdr.st.s32_4[31:16];
    hdr.st.s32_6 = (hdr.st.s32_0) ^ (hdr.st.s32_4);
  }

  action compute_rotate_left_173() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_6[26:0] ++ hdr.st.s32_6[31:27]; }
    hdr.st.s32_2 = (hdr.st.s32_2) ^ (hdr.st.s32_1);
    hdr.st.s32_6 = (hdr.st.s32_1) + (hdr.st.s32_6);
  }

  action compute_rotate_left_174() {
    hdr.st.s32_1 = hdr.st.s32_2[23:0] ++ hdr.st.s32_2[31:24];
    hdr.st.s32_8 = hdr.st.s32_6[15:0] ++ hdr.st.s32_6[31:16];
    hdr.st.s32_3 = (hdr.st.s32_0) ^ (hdr.st.s32_6);
    hdr.st.s32_9 = hdr.st.s32_9 + hdr.st.s32_2;
  }

  action compute_rotate_left_176() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_3[18:0] ++ hdr.st.s32_3[31:19]; }
    hdr.st.s32_6 = (hdr.st.s32_1) ^ (hdr.st.s32_9);
    hdr.st.s32_3 = (hdr.st.s32_9) + (hdr.st.s32_3);
  }

  action compute_rotate_left_177() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_6[24:0] ++ hdr.st.s32_6[31:25]; }
    hdr.st.s32_5 = hdr.st.s32_3[15:0] ++ hdr.st.s32_3[31:16];
  }

  action compute_op_add_451_b() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_6; }
  }

  action compute_op_add_451() {
    hdr.st.s32_1 = hdr.st.s32_8 + hdr.st.s32_1;
  }

  action compute_op_xor_456() {
    @in_hash { eg_md.op_xor_456_out = hdr.st.s32_0 ^ hdr.st.s32_1; }
  }

  action compute_op_xor_455() {
    @in_hash { eg_md.op_xor_455_out = hdr.st.s32_1 ^ hdr.st.s32_0 ^ hdr.st.s32_3 ^ hdr.st.s32_5; }
  }


  apply {
    eg_md.time = eg_intr_md_from_prsr.global_tstamp[47:16];
    if (hdr.egress_state.code_path == 0 || hdr.egress_state.code_path == 3) {
      compute_op_add_358();
      compute_rotate_left_119_x();
      compute_rotate_left_119();
      compute_op_xor_362();
      compute_rotate_left_120_x();
      compute_rotate_left_120();
      compute_rotate_left_122();
      compute_rotate_left_123();
      compute_rotate_left_125();
      compute_rotate_left_126();
      compute_rotate_left_128();
      compute_rotate_left_129();
      compute_rotate_left_131();
      compute_op_xor_382();
      compute_rotate_left_132_x();
      compute_rotate_left_132();
      compute_rotate_left_134();
      compute_rotate_left_135();
      compute_rotate_left_137();
      compute_rotate_left_138();
      compute_rotate_left_140();
      compute_rotate_left_141();
    } else if (hdr.egress_state.code_path == 1) {
      // EP node  482236:RotateLeft
      // BDD node 69:rotate_left
      compute_rotate_left_69_x();
      compute_rotate_left_69();
      compute_rotate_left_71_x();
      compute_rotate_left_71();
      compute_rotate_left_74_x();
      compute_rotate_left_74_shl();
      compute_rotate_left_74_or();
      compute_rotate_left_77_x();
      compute_rotate_left_77();
      compute_rotate_left_70();
      compute_rotate_left_73_x();
      compute_rotate_left_73_shl();
      compute_rotate_left_73_or();
      compute_rotate_left_76_x();
      compute_rotate_left_76_shl();
      compute_rotate_left_76_or();
      compute_rotate_left_79_x();
      compute_rotate_left_79_shl();
      compute_rotate_left_79_or();
      // EP node  484038:ArithmeticOp
      // BDD node 309:op_add
      // EP node  486206:RotateLeft
      // BDD node 70:rotate_left
      // EP node  489466:ArithmeticOp
      // BDD node 310:op_xor
      // EP node  491283:ArithmeticOp
      // BDD node 311:op_add
      // EP node  493469:RotateLeft
      // BDD node 71:rotate_left
      // EP node  495296:ArithmeticOp
      // BDD node 312:op_xor
      // EP node  497494:RotateLeft
      // BDD node 72:rotate_left
      // EP node  499331:ArithmeticOp
      // BDD node 313:op_add
      // EP node  501909:RotateLeftShifts
      // BDD node 73:rotate_left
      // EP node  503756:ArithmeticOp
      // BDD node 314:op_xor
      // EP node  505608:ArithmeticOp
      // BDD node 315:op_add
      // EP node  508207:RotateLeftShifts
      // BDD node 74:rotate_left
      // EP node  510069:ArithmeticOp
      // BDD node 316:op_xor
      // EP node  512309:RotateLeft
      // BDD node 75:rotate_left
      // EP node  514181:ArithmeticOp
      // BDD node 317:op_add
      // EP node  516808:RotateLeftShifts
      // BDD node 76:rotate_left
      // EP node  518690:ArithmeticOp
      // BDD node 318:op_xor
      // EP node  520577:ArithmeticOp
      // BDD node 319:op_add
      // EP node  522847:RotateLeft
      // BDD node 77:rotate_left
      // EP node  524744:ArithmeticOp
      // BDD node 320:op_xor
      // EP node  527026:RotateLeft
      // BDD node 78:rotate_left
      // EP node  528933:ArithmeticOp
      // BDD node 321:op_add
      // EP node  531609:RotateLeftShifts
      // BDD node 79:rotate_left
      // EP node  533526:ArithmeticOp
      // BDD node 322:op_xor
      // EP node  535448:ArithmeticOp
      // BDD node 323:op_add
      // EP node  538145:RotateLeftShifts
      // BDD node 80:rotate_left
      // EP node  540077:ArithmeticOp
      // BDD node 324:op_xor
      // EP node  542401:RotateLeft
      // BDD node 81:rotate_left
      // EP node  544343:ArithmeticOp
      // BDD node 325:op_add
      // EP node  546289:Recirculate
      // BDD node 82:rotate_left
    } else if (hdr.egress_state.code_path == 2) {
      // EP node  585400:ArithmeticOp
      // BDD node 333:op_lshr
      compute_op_add_336();
      compute_op_lshr_333();   // m11: was compute_rotate_left_161_x, whose path-A ops clobbered s32_1/s32_6
      compute_op_sub_334();
      compute_op_lshr_335();   // m11: was compute_rotate_left_162, whose path-A ops clobbered s32_0/s32_2/s32_4
      // EP node  587427:ArithmeticOp
      // BDD node 334:op_sub
      // EP node  589459:ArithmeticOp
      // BDD node 335:op_lshr
      // EP node  591089:ArithmeticOp
      // BDD node 336:op_add
      // EP node  591906:Recirculate
      // BDD node 337:op_xor
      hdr.recirc.f32_0 = eg_md.op_lshr_449_out;
      hdr.recirc.f32_1 = eg_md.op_add_336_out;
    } else if (hdr.egress_state.code_path == 4) {
      // EP node  89473:ArithmeticOp
      // BDD node 423:op_add
      compute_op_add_423();
      compute_rotate_left_161_x();
      compute_rotate_left_162_x();
      compute_rotate_left_162();
      compute_rotate_left_161();
      compute_rotate_left_164_x();
      compute_rotate_left_164();
      compute_rotate_left_165();
      compute_rotate_left_166();
      compute_rotate_left_167();
      compute_rotate_left_169();
      compute_rotate_left_170();
      compute_rotate_left_171();
      compute_rotate_left_173();
      compute_rotate_left_174();
      compute_rotate_left_176();
      compute_rotate_left_177();
      compute_op_add_451_b();
      compute_op_add_451();
      compute_op_xor_456();
      compute_op_xor_455();
      // EP node  90305:RotateLeft
      // BDD node 161:rotate_left
      // EP node  91476:ArithmeticOp
      // BDD node 424:op_xor
      // EP node  92150:ArithmeticOp
      // BDD node 425:op_add
      // EP node  92997:RotateLeft
      // BDD node 162:rotate_left
      // EP node  93679:ArithmeticOp
      // BDD node 426:op_xor
      // EP node  94536:RotateLeft
      // BDD node 163:rotate_left
      // EP node  95226:ArithmeticOp
      // BDD node 427:op_add
      // EP node  96093:RotateLeft
      // BDD node 164:rotate_left
      // EP node  97313:ArithmeticOp
      // BDD node 428:op_xor
      // EP node  98015:ArithmeticOp
      // BDD node 429:op_add
      // EP node  98897:RotateLeft
      // BDD node 165:rotate_left
      // EP node  100138:ArithmeticOp
      // BDD node 430:op_xor
      // EP node  101030:RotateLeft
      // BDD node 166:rotate_left
      // EP node  101748:ArithmeticOp
      // BDD node 431:op_add
      // EP node  102650:RotateLeft
      // BDD node 167:rotate_left
      // EP node  103919:ArithmeticOp
      // BDD node 432:op_xor
      // EP node  104649:ArithmeticOp
      // BDD node 433:op_add
      // EP node  105566:RotateLeft
      // BDD node 168:rotate_left
      // EP node  106304:ArithmeticOp
      // BDD node 434:op_xor
      // EP node  107231:RotateLeft
      // BDD node 169:rotate_left
      // EP node  107977:ArithmeticOp
      // BDD node 435:op_add
      // EP node  108914:RotateLeft
      // BDD node 170:rotate_left
      // EP node  110232:ArithmeticOp
      // BDD node 436:op_xor
      // EP node  110990:ArithmeticOp
      // BDD node 437:op_add
      // EP node  111942:RotateLeft
      // BDD node 171:rotate_left
      // EP node  113281:ArithmeticOp
      // BDD node 438:op_xor
      // EP node  114243:RotateLeft
      // BDD node 172:rotate_left
      // EP node  115017:ArithmeticOp
      // BDD node 439:op_add
      // EP node  115989:RotateLeft
      // BDD node 173:rotate_left
      // EP node  117356:ArithmeticOp
      // BDD node 440:op_xor
      // EP node  118142:ArithmeticOp
      // BDD node 441:op_add
      // EP node  119129:RotateLeft
      // BDD node 174:rotate_left
      // EP node  119923:ArithmeticOp
      // BDD node 442:op_xor
      // EP node  120920:RotateLeft
      // BDD node 175:rotate_left
      // EP node  121722:ArithmeticOp
      // BDD node 443:op_add
      // EP node  122729:RotateLeft
      // BDD node 176:rotate_left
      // EP node  124145:ArithmeticOp
      // BDD node 444:op_xor
      // EP node  124959:ArithmeticOp
      // BDD node 445:op_add
      // EP node  125981:RotateLeft
      // BDD node 177:rotate_left
      // EP node  127418:ArithmeticOp
      // BDD node 446:op_xor
      // EP node  128450:RotateLeft
      // BDD node 178:rotate_left
      // EP node  129280:Ignore
      // BDD node 179:nf_set_rte_ipv4_udptcp_checksum
      // EP node  130321:ArithmeticOp
      // BDD node 447:op_lshr
      // EP node  131159:ArithmeticOp
      // BDD node 448:op_sub
      // EP node  132001:ArithmeticOp
      // BDD node 449:op_lshr
      // EP node  132636:ArithmeticOp
      // BDD node 450:op_xor
      // EP node  133274:ArithmeticOp
      // BDD node 451:op_add
      // EP node  133915:ArithmeticOp
      // BDD node 452:op_add
      // EP node  134559:ArithmeticOp
      // BDD node 453:op_xor
      // EP node  135206:ArithmeticOp
      // BDD node 454:op_xor
      // EP node  135856:ArithmeticOp
      // BDD node 455:op_xor
      // EP node  136292:ArithmeticOp
      // BDD node 456:op_xor
      // EP node  136511:Recirculate
      // BDD node 457:op_xor
      hdr.recirc.f32_0 = eg_md.op_lshr_449_out;
      hdr.recirc.f32_2 = eg_md.op_xor_455_out;
      hdr.recirc.f32_3 = eg_md.op_xor_456_out;
    }


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
