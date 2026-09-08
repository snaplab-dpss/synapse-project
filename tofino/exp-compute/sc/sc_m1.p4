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
  bit<32> rotate_left_16_or_out;
  bit<32> op_lshr_458_out;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<16> dev;
}

header recirc_state_h {
  bit<32> sslot32_0;
  bit<32> sslot32_1;
  bit<32> sslot32_2;
  bit<32> sslot32_3;
  bit<32> sslot32_4;
  bit<32> sslot32_5;
  bit<32> sslot32_6;
  bit<32> sslot32_7;
  bit<32> sslot32_8;
  bit<32> sslot32_9;
  bit<32> sslot32_10;
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
  bit<96> data0;
  bit<16> data1;
}
header hdr1_h {
  bit<8> data0;
  bit<24> data1;
  bit<40> data2;
  bit<24> data3;
  bit<32> data4;
  bit<32> data5;
}
header hdr2_h {
  bit<32> ports;
  bit<32> data2;
  bit<32> data3;
  bit<16> data4;
  bit<48> data5;
}
header hdr3_h {
  bit<16> data0;
  bit<48> data1;
}
header hdr4_h {
  bit<32> data0;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  recirc_state_h recirc_state;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;
  hdr4_h hdr4;

}

struct synapse_ingress_metadata_t {
  bit<32> tdelta2;
  bit<32> tdelta;
  bit<32> vector_table_1073939504_105_get_value_param0;
  bit<16> ingress_port;
  bit<16> dev;
  bit<32> time;
  bit<32> cslot32_0;
  bit<32> cslot32_1;
  bit<32> rotate_left_16_or_out;
  bit<1> bf_est0;
  bit<1> bf_est1;
  bit<32> key_32b_0;
  bit<32> key_32b_1;
  bit<16> key_16b_2;
  bit<16> key_16b_3;
  bit<32> cslot32_2;
  bit<32> cslot32_3;
  bit<32> cslot32_4;
  bit<32> cslot32_5;
  bit<32> op_lshr_447_out;
  bit<32> cslot32_6;
  bit<32> cslot32_7;
  bit<32> cslot32_8;
  bit<32> cslot32_9;
  bit<32> cslot32_10;
  bit<32> cslot32_11;
  bit<32> cslot32_12;
  bit<32> cslot32_13;
  bit<32> cslot32_14;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  recirc_state_h recirc_state;

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

    meta.ingress_port[8:0] = ig_intr_md.ingress_port;
    meta.dev = 0;
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
    pkt.extract(hdr.recirc_state);
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
    transition select (hdr.hdr0.data1) {
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
    transition select (hdr.hdr1.data3[23:16]) {
      8w0x11: parser_6;
      default: parser_201;
    }
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr1.data3[23:16]) {
      8w0x06: parser_9;
      default: parser_10;
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
    hdr.recirc_state.setInvalid();
    hdr.cuckoo.setInvalid();
    fwd(CPU_PCIE_PORT);
  }

  action fwd_nf_dev(bit<16> port) {
    hdr.cpu.setInvalid();
    hdr.recirc.setInvalid();
    hdr.recirc_state.setInvalid();
    hdr.cuckoo.setInvalid();
    fwd(port);
  }

  action set_ingress_dev(bit<16> nf_dev) {
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
  bit<16> nf_dev = 0;
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
    hdr.recirc_state.setValid();
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

  action compute_rotate_left_16_or() {
  }

  Register<bit<1>,_>(1048576, 0) bf_1073926928_row_0;
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
    meta.bf_est0 = bf_1073926928_row_0_read_and_set_value[0:0];
  }

  RegisterAction<bit<1>, bit<20>, void>(bf_1073926928_row_0) bf_1073926928_row_0_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073926928_row_0_set_to_one_execute() {
    bf_1073926928_row_0_set_to_one.execute(bf_1073926928_hash_0_value);
  }

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073926928_row_0) bf_1073926928_row_0_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073926928_row_0_read_value;
  action bf_1073926928_row_0_read_execute() {
    bf_1073926928_row_0_read_value = bf_1073926928_row_0_read.execute(bf_1073926928_hash_0_value);
    meta.bf_est0 = bf_1073926928_row_0_read_value[0:0];
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
    meta.bf_est1 = bf_1073926928_row_1_read_and_set_value[0:0];
  }

  RegisterAction<bit<1>, bit<20>, void>(bf_1073926928_row_1) bf_1073926928_row_1_set_to_one = {
    void apply(inout bit<1> value) {
      value = 1;
    }
  };

  action bf_1073926928_row_1_set_to_one_execute() {
    bf_1073926928_row_1_set_to_one.execute(bf_1073926928_hash_1_value);
  }

  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_1073926928_row_1) bf_1073926928_row_1_read = {
    void apply(inout bit<1> value, out bit<1> out_value) {
      out_value = value;
    }
  };

  bit<1> bf_1073926928_row_1_read_value;
  action bf_1073926928_row_1_read_execute() {
    bf_1073926928_row_1_read_value = bf_1073926928_row_1_read.execute(bf_1073926928_hash_1_value);
    meta.bf_est1 = bf_1073926928_row_1_read_value[0:0];
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_810;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_810;

  action bf_1073926928_hash_0_810_calc_810() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_810.get({
      meta.key_32b_0,
      meta.key_32b_1,
      meta.key_16b_2,
      meta.key_16b_3,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_810_calc_810() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_810.get({
      meta.key_32b_0,
      meta.key_32b_1,
      meta.key_16b_2,
      meta.key_16b_3,
      32w0x2681580b
    });
  }
  action cpu_copy_0() {
    hdr.cpu.bf_1073926928_estimate = (bit<32>) (meta.bf_est1 ++ meta.bf_est0);
  }

  action cpu_copy_1() {
    hdr.cpu.rotate_left_16_or_out = 32w2225785509;
  }

  action compute_op_xor_346() {
    meta.cslot32_2 = 32w0x3b355c4b ^ hdr.hdr1.data4;
  }

  action compute_rotate_left_108_x() {
    hdr.recirc_state.sslot32_0 = 32w0x3b355c4b ^ hdr.hdr1.data4;
  }

  action compute_rotate_left_108_shl() {
    meta.cslot32_3 = hdr.recirc_state.sslot32_0 << 8;
    meta.cslot32_4 = hdr.recirc_state.sslot32_0 >> 24;
  }

  action compute_rotate_left_108_or() {
    meta.cslot32_5 = meta.cslot32_3 | meta.cslot32_4;
  }

  action vector_table_1073939504_105_get_value(bit<32> _vector_table_1073939504_105_get_value_param0) {
    meta.vector_table_1073939504_105_get_value_param0 = _vector_table_1073939504_105_get_value_param0;
  }

  table vector_table_1073939504_105 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1073939504_105_get_value;
    }
    size = 2;
  }

  action compute_op_shl_380_a() {
    @in_hash { meta.op_lshr_447_out = (bit<32>) ig_intr_md.ingress_mac_tstamp[47:16]; }
  }

  action compute_op_add_347() {
    meta.cslot32_9 = 32w0x5d476351 + meta.cslot32_2;
    meta.cslot32_8 = 32w0x6f76ca9a ^ 32w2225785509;
    hdr.recirc_state.sslot32_1 = hdr.hdr2.ports;
  }

  action compute_rotate_left_111_x() {
    meta.cslot32_10 = meta.cslot32_5 ^ meta.cslot32_9;
    meta.cslot32_4 = meta.cslot32_2 + meta.cslot32_8;
  }

  action compute_tdelta() {
    meta.tdelta = meta.op_lshr_447_out - meta.vector_table_1073939504_105_get_value_param0;
  }

  action compute_rotate_left_111_shl() {
    meta.cslot32_8 = meta.cslot32_10 << 7;
    meta.cslot32_9 = meta.cslot32_10 >> 25;
    meta.cslot32_5 = 32w3399118710 + meta.cslot32_10;
    meta.cslot32_2 = 32w0x5d476351 + meta.cslot32_4;
  }

  action compute_rotate_left_111_or() {
    meta.cslot32_4 = meta.cslot32_8 | meta.cslot32_9;
    meta.cslot32_6 = meta.cslot32_2 << 16;
    meta.cslot32_7 = meta.cslot32_2 >> 16;
    meta.cslot32_10 = 32w2148007291 ^ meta.cslot32_2;
    meta.tdelta2 = meta.tdelta >> 32w0x0000000c;
  }
  action compute_tdelta_move() {
    hdr.recirc_state.sslot32_2 = meta.tdelta2;
  }

  action compute_rotate_left_114_x() {
    meta.cslot32_2 = meta.cslot32_4 ^ meta.cslot32_5;
    meta.cslot32_9 = meta.cslot32_6 | meta.cslot32_7;
    meta.cslot32_8 = meta.cslot32_10 << 5;
    meta.cslot32_3 = meta.cslot32_10 >> 27;
    meta.cslot32_11 = meta.cslot32_5 + meta.cslot32_10;
  }

  action compute_rotate_left_114_shl() {
    meta.cslot32_10 = meta.cslot32_2 << 8;
    meta.cslot32_7 = meta.cslot32_2 >> 24;
    meta.cslot32_6 = meta.cslot32_8 | meta.cslot32_3;
    meta.cslot32_4 = meta.cslot32_9 + meta.cslot32_2;
    meta.cslot32_5 = meta.cslot32_11 << 16;
    meta.cslot32_12 = meta.cslot32_11 >> 16;
  }

  action compute_rotate_left_114_or() {
    meta.cslot32_3 = meta.cslot32_10 | meta.cslot32_7;
    meta.cslot32_8 = meta.cslot32_5 | meta.cslot32_12;
    meta.cslot32_9 = meta.cslot32_6 ^ meta.cslot32_11;
  }

  action compute_rotate_left_116_shl() {
    meta.cslot32_12 = meta.cslot32_9 << 13;
    meta.cslot32_5 = meta.cslot32_9 >> 19;
    meta.cslot32_6 = meta.cslot32_3 ^ meta.cslot32_4;
    meta.cslot32_7 = meta.cslot32_4 + meta.cslot32_9;
  }

  action compute_rotate_left_116_or() {
    meta.cslot32_9 = meta.cslot32_12 | meta.cslot32_5;
    meta.cslot32_3 = meta.cslot32_6 << 7;
    meta.cslot32_4 = meta.cslot32_6 >> 25;
    meta.cslot32_10 = meta.cslot32_7 << 16;
    meta.cslot32_11 = meta.cslot32_7 >> 16;
    meta.cslot32_2 = meta.cslot32_8 + meta.cslot32_6;
  }

  action compute_rotate_left_117_or() {
    meta.cslot32_6 = meta.cslot32_3 | meta.cslot32_4;
    meta.cslot32_5 = meta.cslot32_10 | meta.cslot32_11;
    meta.cslot32_12 = meta.cslot32_9 ^ meta.cslot32_7;
    meta.cslot32_8 = meta.cslot32_2 ^ hdr.hdr1.data4;
  }

  action compute_rotate_left_119_shl() {
    meta.cslot32_11 = meta.cslot32_12 << 5;
    meta.cslot32_10 = meta.cslot32_12 >> 27;
    meta.cslot32_4 = meta.cslot32_6 ^ meta.cslot32_2;
    meta.cslot32_3 = meta.cslot32_8 + meta.cslot32_12;
  }

  action compute_rotate_left_119_or() {
    meta.cslot32_8 = meta.cslot32_11 | meta.cslot32_10;
    hdr.recirc_state.sslot32_3 = meta.cslot32_4 ^ hdr.hdr1.data5;
    meta.cslot32_12 = meta.cslot32_3 << 16;
    meta.cslot32_6 = meta.cslot32_3 >> 16;
  }

  action compute_rotate_left_120_shl() {
    meta.cslot32_4 = hdr.recirc_state.sslot32_3 << 8;
    meta.cslot32_10 = hdr.recirc_state.sslot32_3 >> 24;
    meta.cslot32_11 = meta.cslot32_5 + hdr.recirc_state.sslot32_3;
    meta.cslot32_2 = meta.cslot32_12 | meta.cslot32_6;
    meta.cslot32_9 = meta.cslot32_8 ^ meta.cslot32_3;
  }

  action compute_rotate_left_120_or() {
    meta.cslot32_6 = meta.cslot32_4 | meta.cslot32_10;
    meta.cslot32_12 = meta.cslot32_9 << 13;
    meta.cslot32_8 = meta.cslot32_9 >> 19;
    meta.cslot32_3 = meta.cslot32_11 + meta.cslot32_9;
  }

  action compute_rotate_left_123_x() {
    meta.cslot32_9 = meta.cslot32_6 ^ meta.cslot32_11;
    meta.cslot32_10 = meta.cslot32_12 | meta.cslot32_8;
    meta.cslot32_4 = meta.cslot32_3 << 16;
    meta.cslot32_5 = meta.cslot32_3 >> 16;
  }

  action compute_rotate_left_123_shl() {
    meta.cslot32_8 = meta.cslot32_9 << 7;
    meta.cslot32_12 = meta.cslot32_9 >> 25;
    hdr.recirc_state.sslot32_4 = meta.cslot32_4 | meta.cslot32_5;
    hdr.recirc_state.sslot32_5 = meta.cslot32_10 ^ meta.cslot32_3;
    hdr.recirc_state.sslot32_6 = meta.cslot32_2 + meta.cslot32_9;
  }

  action compute_rotate_left_123_or() {
    hdr.recirc_state.sslot32_7 = meta.cslot32_8 | meta.cslot32_12;
    meta.cslot32_5 = hdr.recirc_state.sslot32_5 << 5;
    meta.cslot32_4 = hdr.recirc_state.sslot32_5 >> 27;
    hdr.recirc_state.sslot32_8 = hdr.recirc_state.sslot32_6 + hdr.recirc_state.sslot32_5;
  }

  action compute_rotate_left_125_or() {
    hdr.recirc_state.sslot32_9 = meta.cslot32_5 | meta.cslot32_4;
    hdr.recirc_state.sslot32_10 = hdr.recirc_state.sslot32_7 ^ hdr.recirc_state.sslot32_6;
  }

  action recirc_copy_2() {
  }

  action recirc_copy_3() {
  }

  action compute_op_xor_375() {
    meta.cslot32_0 = hdr.recirc_state.sslot32_9 ^ hdr.recirc_state.sslot32_8;
    meta.cslot32_1 = hdr.recirc_state.sslot32_6 + hdr.recirc_state.sslot32_5;
    meta.cslot32_2 = hdr.recirc_state.sslot32_4 + hdr.recirc_state.sslot32_10;
    meta.cslot32_3 = hdr.recirc_state.sslot32_7 ^ hdr.recirc_state.sslot32_6;
  }

  action compute_rotate_left_128_shl() {
    meta.cslot32_4 = meta.cslot32_0 << 13;
    meta.cslot32_5 = meta.cslot32_0 >> 19;
    meta.cslot32_6 = meta.cslot32_1 << 16;
    meta.cslot32_7 = meta.cslot32_1 >> 16;
    meta.cslot32_8 = meta.cslot32_2 + meta.cslot32_0;
    meta.cslot32_9 = meta.cslot32_3 << 8;
    meta.cslot32_10 = meta.cslot32_3 >> 24;
  }

  action compute_rotate_left_128_or() {
    meta.cslot32_0 = meta.cslot32_4 | meta.cslot32_5;
    meta.cslot32_11 = meta.cslot32_6 | meta.cslot32_7;
    meta.cslot32_12 = meta.cslot32_8 << 16;
    meta.cslot32_13 = meta.cslot32_8 >> 16;
    meta.cslot32_14 = meta.cslot32_9 | meta.cslot32_10;
  }

  action compute_rotate_left_130_or() {
    meta.cslot32_10 = meta.cslot32_12 | meta.cslot32_13;
    meta.cslot32_9 = meta.cslot32_14 ^ meta.cslot32_2;
    meta.cslot32_7 = meta.cslot32_0 ^ meta.cslot32_8;
  }

  action compute_rotate_left_129_shl() {
    meta.cslot32_14 = meta.cslot32_9 << 7;
    meta.cslot32_13 = meta.cslot32_9 >> 25;
    meta.cslot32_12 = meta.cslot32_7 << 5;
    meta.cslot32_0 = meta.cslot32_7 >> 27;
    meta.cslot32_8 = meta.cslot32_11 + meta.cslot32_9;
  }

  action compute_rotate_left_129_or() {
    meta.cslot32_9 = meta.cslot32_14 | meta.cslot32_13;
    meta.cslot32_11 = meta.cslot32_12 | meta.cslot32_0;
    meta.cslot32_2 = meta.cslot32_8 ^ hdr.hdr1.data5;
  }

  action compute_rotate_left_133_x() {
    meta.cslot32_0 = meta.cslot32_2 + meta.cslot32_7;
    meta.cslot32_12 = meta.cslot32_9 ^ meta.cslot32_8;
  }

  action compute_rotate_left_133_shl() {
    meta.cslot32_2 = meta.cslot32_0 << 16;
    meta.cslot32_9 = meta.cslot32_0 >> 16;
    meta.cslot32_8 = meta.cslot32_12 ^ hdr.recirc_state.sslot32_1;
    meta.cslot32_7 = meta.cslot32_11 ^ meta.cslot32_0;
  }

  action compute_rotate_left_133_or() {
    meta.cslot32_12 = meta.cslot32_2 | meta.cslot32_9;
    meta.cslot32_0 = meta.cslot32_8 << 8;
    meta.cslot32_11 = meta.cslot32_8 >> 24;
    meta.cslot32_13 = meta.cslot32_7 << 13;
    meta.cslot32_14 = meta.cslot32_7 >> 19;
    meta.cslot32_6 = meta.cslot32_10 + meta.cslot32_8;
  }

  action compute_rotate_left_132_or() {
    meta.cslot32_9 = meta.cslot32_0 | meta.cslot32_11;
    meta.cslot32_2 = meta.cslot32_13 | meta.cslot32_14;
    meta.cslot32_10 = meta.cslot32_6 + meta.cslot32_7;
  }

  action compute_rotate_left_135_x() {
    meta.cslot32_14 = meta.cslot32_9 ^ meta.cslot32_6;
    meta.cslot32_13 = meta.cslot32_10 << 16;
    meta.cslot32_11 = meta.cslot32_10 >> 16;
    meta.cslot32_0 = meta.cslot32_2 ^ meta.cslot32_10;
  }

  action compute_rotate_left_135_shl() {
    meta.cslot32_10 = meta.cslot32_14 << 7;
    meta.cslot32_2 = meta.cslot32_14 >> 25;
    meta.cslot32_9 = meta.cslot32_13 | meta.cslot32_11;
    meta.cslot32_6 = meta.cslot32_0 << 5;
    meta.cslot32_7 = meta.cslot32_0 >> 27;
    meta.cslot32_5 = meta.cslot32_12 + meta.cslot32_14;
  }

  action compute_rotate_left_135_or() {
    meta.cslot32_11 = meta.cslot32_10 | meta.cslot32_2;
    meta.cslot32_13 = meta.cslot32_6 | meta.cslot32_7;
    meta.cslot32_14 = meta.cslot32_5 + meta.cslot32_0;
  }

  action compute_rotate_left_139_shl() {
    meta.cslot32_7 = meta.cslot32_14 << 16;
    meta.cslot32_6 = meta.cslot32_14 >> 16;
    meta.cslot32_2 = meta.cslot32_11 ^ meta.cslot32_5;
    meta.cslot32_10 = meta.cslot32_13 ^ meta.cslot32_14;
  }

  action compute_rotate_left_139_or() {
    meta.cslot32_14 = meta.cslot32_7 | meta.cslot32_6;
    meta.cslot32_13 = meta.cslot32_2 << 8;
    meta.cslot32_11 = meta.cslot32_2 >> 24;
    meta.cslot32_5 = meta.cslot32_10 << 13;
    meta.cslot32_0 = meta.cslot32_10 >> 19;
    meta.cslot32_12 = meta.cslot32_9 + meta.cslot32_2;
  }

  action compute_rotate_left_138_or() {
    meta.cslot32_2 = meta.cslot32_13 | meta.cslot32_11;
    meta.cslot32_6 = meta.cslot32_5 | meta.cslot32_0;
    meta.cslot32_7 = meta.cslot32_12 + meta.cslot32_10;
  }

  action compute_rotate_left_141_x() {
    meta.cslot32_0 = meta.cslot32_2 ^ meta.cslot32_12;
    meta.cslot32_5 = meta.cslot32_7 << 16;
    meta.cslot32_11 = meta.cslot32_7 >> 16;
    hdr.recirc_state.sslot32_10 = meta.cslot32_6 ^ meta.cslot32_7;
  }

  action compute_rotate_left_141_shl() {
    meta.cslot32_7 = meta.cslot32_0 << 7;
    meta.cslot32_6 = meta.cslot32_0 >> 25;
    hdr.recirc_state.sslot32_9 = meta.cslot32_5 | meta.cslot32_11;
    meta.cslot32_2 = meta.cslot32_14 + meta.cslot32_0;
    meta.cslot32_12 = hdr.recirc_state.sslot32_10 << 5;
    meta.cslot32_13 = hdr.recirc_state.sslot32_10 >> 27;
  }

  action compute_rotate_left_141_or() {
    meta.cslot32_11 = meta.cslot32_7 | meta.cslot32_6;
    hdr.recirc_state.sslot32_8 = meta.cslot32_12 | meta.cslot32_13;
    hdr.recirc_state.sslot32_7 = meta.cslot32_2 ^ hdr.recirc_state.sslot32_1;
  }

  action compute_op_add_402() {
    hdr.recirc_state.sslot32_1 = hdr.recirc_state.sslot32_7 + hdr.recirc_state.sslot32_10;
    hdr.recirc_state.sslot32_6 = meta.cslot32_11 ^ meta.cslot32_2;
  }

  action compute_rotate_left_146_x() {
    meta.cslot32_0 = hdr.recirc_state.sslot32_8 ^ hdr.recirc_state.sslot32_1;
    meta.cslot32_1 = hdr.recirc_state.sslot32_7 + hdr.recirc_state.sslot32_10;
    hdr.recirc_state.sslot32_5 = hdr.recirc_state.sslot32_6 ^ hdr.hdr2.data2;
  }

  action compute_rotate_left_146_shl() {
    meta.cslot32_2 = meta.cslot32_0 << 13;
    meta.cslot32_3 = meta.cslot32_0 >> 19;
    meta.cslot32_4 = meta.cslot32_1 << 16;
    meta.cslot32_5 = meta.cslot32_1 >> 16;
    meta.cslot32_6 = hdr.recirc_state.sslot32_5 << 8;
    meta.cslot32_7 = hdr.recirc_state.sslot32_5 >> 24;
  }

  action compute_rotate_left_146_or() {
    meta.cslot32_8 = meta.cslot32_2 | meta.cslot32_3;
    meta.cslot32_9 = meta.cslot32_4 | meta.cslot32_5;
    meta.cslot32_10 = meta.cslot32_6 | meta.cslot32_7;
    meta.cslot32_11 = hdr.recirc_state.sslot32_9 + hdr.recirc_state.sslot32_5;
  }

  action compute_rotate_left_147_x() {
    meta.cslot32_7 = meta.cslot32_10 ^ meta.cslot32_11;
    meta.cslot32_6 = meta.cslot32_11 + meta.cslot32_0;
  }

  action compute_rotate_left_147_shl() {
    meta.cslot32_11 = meta.cslot32_7 << 7;
    meta.cslot32_10 = meta.cslot32_7 >> 25;
    meta.cslot32_0 = meta.cslot32_6 << 16;
    meta.cslot32_5 = meta.cslot32_6 >> 16;
    meta.cslot32_4 = meta.cslot32_8 ^ meta.cslot32_6;
    meta.cslot32_3 = meta.cslot32_9 + meta.cslot32_7;
  }

  action compute_rotate_left_147_or() {
    meta.cslot32_6 = meta.cslot32_11 | meta.cslot32_10;
    meta.cslot32_7 = meta.cslot32_0 | meta.cslot32_5;
    meta.cslot32_9 = meta.cslot32_4 << 5;
    meta.cslot32_8 = meta.cslot32_4 >> 27;
    meta.cslot32_2 = meta.cslot32_3 + meta.cslot32_4;
  }

  action compute_rotate_left_149_or() {
    meta.cslot32_4 = meta.cslot32_9 | meta.cslot32_8;
    meta.cslot32_5 = meta.cslot32_2 << 16;
    meta.cslot32_0 = meta.cslot32_2 >> 16;
    meta.cslot32_10 = meta.cslot32_6 ^ meta.cslot32_3;
  }

  action compute_rotate_left_151_or() {
    meta.cslot32_8 = meta.cslot32_5 | meta.cslot32_0;
    meta.cslot32_9 = meta.cslot32_10 << 8;
    meta.cslot32_6 = meta.cslot32_10 >> 24;
    meta.cslot32_3 = meta.cslot32_7 + meta.cslot32_10;
    meta.cslot32_11 = meta.cslot32_4 ^ meta.cslot32_2;
  }

  action compute_rotate_left_150_or() {
    meta.cslot32_10 = meta.cslot32_9 | meta.cslot32_6;
    meta.cslot32_0 = meta.cslot32_11 << 13;
    meta.cslot32_5 = meta.cslot32_11 >> 19;
    meta.cslot32_4 = meta.cslot32_3 + meta.cslot32_11;
  }

  action compute_rotate_left_152_or() {
    meta.cslot32_11 = meta.cslot32_0 | meta.cslot32_5;
    meta.cslot32_6 = meta.cslot32_10 ^ meta.cslot32_3;
    meta.cslot32_9 = meta.cslot32_4 << 16;
    meta.cslot32_2 = meta.cslot32_4 >> 16;
  }

  action compute_rotate_left_153_shl() {
    meta.cslot32_5 = meta.cslot32_6 << 7;
    meta.cslot32_0 = meta.cslot32_6 >> 25;
    meta.cslot32_10 = meta.cslot32_8 + meta.cslot32_6;
    meta.cslot32_3 = meta.cslot32_9 | meta.cslot32_2;
    meta.cslot32_7 = meta.cslot32_11 ^ meta.cslot32_4;
  }

  action compute_rotate_left_153_or() {
    meta.cslot32_2 = meta.cslot32_5 | meta.cslot32_0;
    meta.cslot32_9 = meta.cslot32_7 << 5;
    meta.cslot32_6 = meta.cslot32_7 >> 27;
    meta.cslot32_11 = meta.cslot32_10 ^ hdr.hdr2.data2;
  }

  action compute_rotate_left_155_or() {
    meta.cslot32_0 = meta.cslot32_9 | meta.cslot32_6;
    meta.cslot32_5 = meta.cslot32_2 ^ meta.cslot32_10;
    meta.cslot32_4 = meta.cslot32_11 + meta.cslot32_7;
  }

  action compute_rotate_left_156_shl() {
    meta.cslot32_11 = meta.cslot32_5 << 8;
    meta.cslot32_6 = meta.cslot32_5 >> 24;
    meta.cslot32_9 = meta.cslot32_4 << 16;
    meta.cslot32_2 = meta.cslot32_4 >> 16;
    meta.cslot32_7 = meta.cslot32_0 ^ meta.cslot32_4;
    meta.cslot32_10 = meta.cslot32_3 + meta.cslot32_5;
  }

  action compute_rotate_left_156_or() {
    meta.cslot32_4 = meta.cslot32_11 | meta.cslot32_6;
    meta.cslot32_5 = meta.cslot32_9 | meta.cslot32_2;
    meta.cslot32_0 = meta.cslot32_7 << 13;
    meta.cslot32_3 = meta.cslot32_7 >> 19;
    meta.cslot32_8 = meta.cslot32_10 + meta.cslot32_7;
  }

  action compute_rotate_left_158_or() {
    meta.cslot32_7 = meta.cslot32_0 | meta.cslot32_3;
    meta.cslot32_2 = meta.cslot32_4 ^ meta.cslot32_10;
    meta.cslot32_9 = meta.cslot32_8 << 16;
    meta.cslot32_6 = meta.cslot32_8 >> 16;
  }

  action compute_rotate_left_159_shl() {
    meta.cslot32_3 = meta.cslot32_2 << 7;
    meta.cslot32_0 = meta.cslot32_2 >> 25;
    hdr.recirc_state.sslot32_6 = meta.cslot32_9 | meta.cslot32_6;
    hdr.recirc_state.sslot32_1 = meta.cslot32_7 ^ meta.cslot32_8;
    hdr.recirc_state.sslot32_7 = meta.cslot32_5 + meta.cslot32_2;
  }

  action compute_rotate_left_159_or() {
    hdr.recirc_state.sslot32_8 = meta.cslot32_3 | meta.cslot32_0;
    meta.cslot32_6 = hdr.recirc_state.sslot32_1 << 5;
    meta.cslot32_9 = hdr.recirc_state.sslot32_1 >> 27;
    hdr.recirc_state.sslot32_9 = hdr.recirc_state.sslot32_7 + hdr.recirc_state.sslot32_1;
  }

  action compute_rotate_left_161_or() {
    hdr.recirc_state.sslot32_10 = meta.cslot32_6 | meta.cslot32_9;
  }

  action compute_rotate_left_164_x() {
    meta.cslot32_0 = hdr.recirc_state.sslot32_10 ^ hdr.recirc_state.sslot32_9;
    meta.cslot32_1 = hdr.recirc_state.sslot32_8 ^ hdr.recirc_state.sslot32_7;
    meta.cslot32_2 = hdr.recirc_state.sslot32_7 + hdr.recirc_state.sslot32_1;
  }

  action compute_rotate_left_164_shl() {
    meta.cslot32_3 = meta.cslot32_0 << 13;
    meta.cslot32_4 = meta.cslot32_0 >> 19;
    meta.cslot32_5 = meta.cslot32_1 << 8;
    meta.cslot32_6 = meta.cslot32_1 >> 24;
    meta.cslot32_7 = hdr.recirc_state.sslot32_6 + meta.cslot32_1;
    meta.cslot32_8 = meta.cslot32_2 << 16;
    meta.cslot32_9 = meta.cslot32_2 >> 16;
  }

  action compute_rotate_left_164_or() {
    meta.cslot32_10 = meta.cslot32_3 | meta.cslot32_4;
    meta.cslot32_11 = meta.cslot32_5 | meta.cslot32_6;
    meta.cslot32_12 = meta.cslot32_8 | meta.cslot32_9;
    meta.cslot32_13 = meta.cslot32_7 + meta.cslot32_0;
  }

  action compute_rotate_left_165_x() {
    meta.cslot32_9 = meta.cslot32_11 ^ meta.cslot32_7;
    meta.cslot32_8 = meta.cslot32_13 << 16;
    meta.cslot32_6 = meta.cslot32_13 >> 16;
    meta.cslot32_5 = meta.cslot32_10 ^ meta.cslot32_13;
  }

  action compute_rotate_left_165_shl() {
    meta.cslot32_13 = meta.cslot32_9 << 7;
    meta.cslot32_11 = meta.cslot32_9 >> 25;
    meta.cslot32_10 = meta.cslot32_8 | meta.cslot32_6;
    meta.cslot32_7 = meta.cslot32_5 << 5;
    meta.cslot32_4 = meta.cslot32_5 >> 27;
    meta.cslot32_3 = meta.cslot32_12 + meta.cslot32_9;
  }

  action compute_rotate_left_165_or() {
    meta.cslot32_6 = meta.cslot32_13 | meta.cslot32_11;
    meta.cslot32_8 = meta.cslot32_7 | meta.cslot32_4;
    meta.cslot32_9 = meta.cslot32_3 + meta.cslot32_5;
  }

  action compute_rotate_left_168_x() {
    meta.cslot32_4 = meta.cslot32_6 ^ meta.cslot32_3;
    meta.cslot32_7 = meta.cslot32_9 << 16;
    meta.cslot32_11 = meta.cslot32_9 >> 16;
    meta.cslot32_13 = meta.cslot32_8 ^ meta.cslot32_9;
  }

  action compute_rotate_left_168_shl() {
    meta.cslot32_9 = meta.cslot32_4 << 8;
    meta.cslot32_8 = meta.cslot32_4 >> 24;
    meta.cslot32_6 = meta.cslot32_7 | meta.cslot32_11;
    meta.cslot32_3 = meta.cslot32_10 + meta.cslot32_4;
    meta.cslot32_5 = meta.cslot32_13 << 13;
    meta.cslot32_12 = meta.cslot32_13 >> 19;
  }

  action compute_rotate_left_168_or() {
    meta.cslot32_11 = meta.cslot32_9 | meta.cslot32_8;
    meta.cslot32_7 = meta.cslot32_5 | meta.cslot32_12;
    meta.cslot32_4 = meta.cslot32_3 + meta.cslot32_13;
  }

  action compute_rotate_left_171_x() {
    meta.cslot32_12 = meta.cslot32_11 ^ meta.cslot32_3;
    meta.cslot32_5 = meta.cslot32_4 << 16;
    meta.cslot32_8 = meta.cslot32_4 >> 16;
    meta.cslot32_9 = meta.cslot32_7 ^ meta.cslot32_4;
  }

  action compute_rotate_left_171_shl() {
    meta.cslot32_4 = meta.cslot32_12 << 7;
    meta.cslot32_7 = meta.cslot32_12 >> 25;
    meta.cslot32_11 = meta.cslot32_5 | meta.cslot32_8;
    meta.cslot32_3 = meta.cslot32_9 << 5;
    meta.cslot32_13 = meta.cslot32_9 >> 27;
    meta.cslot32_10 = meta.cslot32_6 + meta.cslot32_12;
  }

  action compute_rotate_left_171_or() {
    meta.cslot32_8 = meta.cslot32_4 | meta.cslot32_7;
    meta.cslot32_5 = meta.cslot32_3 | meta.cslot32_13;
    meta.cslot32_12 = meta.cslot32_10 + meta.cslot32_9;
  }

  action compute_rotate_left_174_x() {
    meta.cslot32_13 = meta.cslot32_8 ^ meta.cslot32_10;
    meta.cslot32_3 = meta.cslot32_12 << 16;
    meta.cslot32_7 = meta.cslot32_12 >> 16;
    meta.cslot32_4 = meta.cslot32_5 ^ meta.cslot32_12;
  }

  action compute_rotate_left_174_shl() {
    meta.cslot32_12 = meta.cslot32_13 << 8;
    meta.cslot32_5 = meta.cslot32_13 >> 24;
    meta.cslot32_8 = meta.cslot32_3 | meta.cslot32_7;
    meta.cslot32_10 = meta.cslot32_11 + meta.cslot32_13;
    meta.cslot32_9 = meta.cslot32_4 << 13;
    meta.cslot32_6 = meta.cslot32_4 >> 19;
  }

  action compute_rotate_left_174_or() {
    meta.cslot32_7 = meta.cslot32_12 | meta.cslot32_5;
    meta.cslot32_3 = meta.cslot32_9 | meta.cslot32_6;
    meta.cslot32_13 = meta.cslot32_10 + meta.cslot32_4;
  }

  action compute_rotate_left_177_x() {
    meta.cslot32_6 = meta.cslot32_7 ^ meta.cslot32_10;
    meta.cslot32_9 = meta.cslot32_13 << 16;
    meta.cslot32_5 = meta.cslot32_13 >> 16;
    meta.cslot32_12 = meta.cslot32_3 ^ meta.cslot32_13;
  }

  action compute_rotate_left_177_shl() {
    meta.cslot32_13 = meta.cslot32_6 << 7;
    meta.cslot32_3 = meta.cslot32_6 >> 25;
    meta.cslot32_7 = meta.cslot32_8 + meta.cslot32_6;
    meta.cslot32_10 = meta.cslot32_9 | meta.cslot32_5;
  }

  action compute_rotate_left_177_or() {
    meta.cslot32_5 = meta.cslot32_13 | meta.cslot32_3;
    meta.cslot32_9 = meta.cslot32_7 ^ meta.cslot32_12;
  }

  action compute_op_xor_456() {
    meta.cslot32_3 = meta.cslot32_5 ^ meta.cslot32_7;
    meta.cslot32_13 = meta.cslot32_9 ^ meta.cslot32_10;
  }

  action compute_op_xor_457() {
    hdr.recirc_state.sslot32_10 = meta.cslot32_13 ^ meta.cslot32_3;
  }

  action swap_action_180() {
    hdr.hdr2.ports = hdr.hdr2.ports[15:0] ++ hdr.hdr2.ports[31:16];
  }
  action hdr_value_4() {
  }

  action hdr_snapshot_5() {
  }

  action hdr_value_6() {
  }

  action hdr_snapshot_7() {
  }

  action hdr_value_8() {
  }

  action hdr_write_9() {
    hdr.hdr2.data2 = (hdr.recirc_state.sslot32_2) ^ (hdr.recirc_state.sslot32_10);
  }

  action hdr_write_10() {
    hdr.hdr2.data3 = 32w0x00000001 + hdr.hdr2.data2;
  }

  action hdr_write_11() {
    hdr.hdr2.data4 = 8w0x50 ++ (hdr.hdr2.data4[7:0] | 8w0x12);
  }

  action hdr_write_12() {
  }

  action swap_action_181() {
    swap(hdr.hdr1.data4[31:24], hdr.hdr1.data5[31:24]);
    swap(hdr.hdr1.data4[23:16], hdr.hdr1.data5[23:16]);
    swap(hdr.hdr1.data4[15:8], hdr.hdr1.data5[15:8]);
    swap(hdr.hdr1.data4[7:0], hdr.hdr1.data5[7:0]);
  }
  action hdr_write_13() {
    hdr.hdr1.data0 = 8w0x45;
  }

  action hdr_write_14() {
    hdr.hdr1.data1[15:0] = 16w0x0028;
  }

  action hdr_write_15() {
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_132776;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_132776;

  action bf_1073926928_hash_0_132776_calc_132776() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_132776.get({
      meta.key_32b_0,
      meta.key_32b_1,
      meta.key_16b_2,
      meta.key_16b_3,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_132776_calc_132776() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_132776.get({
      meta.key_32b_0,
      meta.key_32b_1,
      meta.key_16b_2,
      meta.key_16b_3,
      32w0x2681580b
    });
  }
  action compute_op_lshr_458() {
    
  }

  action cpu_copy_16() {
    @in_hash { hdr.cpu.op_lshr_458_out = (bit<32>) ig_intr_md.ingress_mac_tstamp[47:16]; }
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  23174:ArithmeticOp
        // BDD node 375:op_xor
        compute_op_xor_375();
        compute_rotate_left_128_shl();
        compute_rotate_left_128_or();
        compute_rotate_left_130_or();
        compute_rotate_left_129_shl();
        compute_rotate_left_129_or();
        compute_rotate_left_133_x();
        compute_rotate_left_133_shl();
        compute_rotate_left_133_or();
        compute_rotate_left_132_or();
        compute_rotate_left_135_x();
        compute_rotate_left_135_shl();
        compute_rotate_left_135_or();
        compute_rotate_left_139_shl();
        compute_rotate_left_139_or();
        compute_rotate_left_138_or();
        compute_rotate_left_141_x();
        compute_rotate_left_141_shl();
        compute_rotate_left_141_or();
        compute_op_add_402();
        // EP node  23703:RotateLeftShifts
        // BDD node 128:rotate_left
        // EP node  24151:RotateLeftShifts
        // BDD node 127:rotate_left
        // EP node  24516:ArithmeticOp
        // BDD node 374:op_add
        // EP node  24974:RotateLeftShifts
        // BDD node 130:rotate_left
        // EP node  25347:RotateLeftShifts
        // BDD node 126:rotate_left
        // EP node  25815:RotateLeftShifts
        // BDD node 129:rotate_left
        // EP node  26196:ArithmeticOp
        // BDD node 376:op_add
        // EP node  26674:RotateLeftShifts
        // BDD node 131:rotate_left
        // EP node  27063:ArithmeticOp
        // BDD node 377:op_xor
        // EP node  27456:ArithmeticOp
        // BDD node 378:op_add
        // EP node  27949:ArithmeticOp
        // BDD node 383:op_xor
        // EP node  28350:ArithmeticOp
        // BDD node 382:op_xor
        // EP node  28853:RotateLeftShifts
        // BDD node 133:rotate_left
        // EP node  29262:ArithmeticOp
        // BDD node 379:op_xor
        // EP node  29775:RotateLeftShifts
        // BDD node 132:rotate_left
        // EP node  30192:ArithmeticOp
        // BDD node 384:op_add
        // EP node  30715:RotateLeftShifts
        // BDD node 134:rotate_left
        // EP node  31140:ArithmeticOp
        // BDD node 385:op_xor
        // EP node  31569:ArithmeticOp
        // BDD node 387:op_xor
        // EP node  31897:ArithmeticOp
        // BDD node 386:op_add
        // EP node  32546:ArithmeticOp
        // BDD node 388:op_add
        // EP node  33308:RotateLeftShifts
        // BDD node 135:rotate_left
        // EP node  33969:RotateLeftShifts
        // BDD node 136:rotate_left
        // EP node  34527:RotateLeftShifts
        // BDD node 137:rotate_left
        // EP node  34980:ArithmeticOp
        // BDD node 389:op_xor
        // EP node  35437:ArithmeticOp
        // BDD node 391:op_xor
        // EP node  35786:ArithmeticOp
        // BDD node 390:op_add
        // EP node  36477:ArithmeticOp
        // BDD node 392:op_add
        // EP node  37288:RotateLeftShifts
        // BDD node 139:rotate_left
        // EP node  37991:RotateLeftShifts
        // BDD node 138:rotate_left
        // EP node  38584:RotateLeftShifts
        // BDD node 140:rotate_left
        // EP node  39065:ArithmeticOp
        // BDD node 393:op_xor
        // EP node  39550:ArithmeticOp
        // BDD node 394:op_add
        // EP node  40158:RotateLeftShifts
        // BDD node 141:rotate_left
        // EP node  40651:ArithmeticOp
        // BDD node 395:op_xor
        // EP node  41269:RotateLeftShifts
        // BDD node 142:rotate_left
        // EP node  41770:ArithmeticOp
        // BDD node 396:op_add
        // EP node  42398:ArithmeticOp
        // BDD node 397:op_xor
        // EP node  43031:ArithmeticOp
        // BDD node 398:op_add
        // EP node  43794:RotateLeftShifts
        // BDD node 143:rotate_left
        // EP node  44437:ArithmeticOp
        // BDD node 400:op_xor
        // EP node  44958:ArithmeticOp
        // BDD node 401:op_xor
        // EP node  46008:ArithmeticOp
        // BDD node 402:op_add
        // EP node  47195:ArithmeticOp
        // BDD node 399:op_xor
        // EP node  48390:Recirculate
        // BDD node 146:rotate_left
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(1);
      } else if (hdr.recirc.code_path == 1) {
          // EP node  50003:RotateLeftShifts
          // BDD node 146:rotate_left
          compute_rotate_left_146_x();
          compute_rotate_left_146_shl();
          compute_rotate_left_146_or();
          compute_rotate_left_147_x();
          compute_rotate_left_147_shl();
          compute_rotate_left_147_or();
          compute_rotate_left_149_or();
          compute_rotate_left_151_or();
          compute_rotate_left_150_or();
          compute_rotate_left_152_or();
          compute_rotate_left_153_shl();
          compute_rotate_left_153_or();
          compute_rotate_left_155_or();
          compute_rotate_left_156_shl();
          compute_rotate_left_156_or();
          compute_rotate_left_158_or();
          compute_rotate_left_159_shl();
          compute_rotate_left_159_or();
          compute_rotate_left_161_or();
          // EP node  50808:RotateLeftShifts
          // BDD node 145:rotate_left
          // EP node  51486:ArithmeticOp
          // BDD node 403:op_xor
          // EP node  52169:RotateLeftShifts
          // BDD node 144:rotate_left
          // EP node  52722:ArithmeticOp
          // BDD node 404:op_add
          // EP node  53415:RotateLeftShifts
          // BDD node 147:rotate_left
          // EP node  53976:ArithmeticOp
          // BDD node 405:op_xor
          // EP node  54679:ArithmeticOp
          // BDD node 406:op_add
          // EP node  55526:RotateLeftShifts
          // BDD node 148:rotate_left
          // EP node  56239:RotateLeftShifts
          // BDD node 149:rotate_left
          // EP node  56816:ArithmeticOp
          // BDD node 407:op_xor
          // EP node  57397:ArithmeticOp
          // BDD node 409:op_xor
          // EP node  57839:ArithmeticOp
          // BDD node 408:op_add
          // EP node  58716:RotateLeftShifts
          // BDD node 151:rotate_left
          // EP node  59454:RotateLeftShifts
          // BDD node 150:rotate_left
          // EP node  60051:ArithmeticOp
          // BDD node 411:op_xor
          // EP node  60652:ArithmeticOp
          // BDD node 412:op_add
          // EP node  61405:ArithmeticOp
          // BDD node 410:op_add
          // EP node  62312:RotateLeftShifts
          // BDD node 152:rotate_left
          // EP node  63075:RotateLeftShifts
          // BDD node 153:rotate_left
          // EP node  63692:ArithmeticOp
          // BDD node 415:op_xor
          // EP node  64313:ArithmeticOp
          // BDD node 413:op_xor
          // EP node  65091:ArithmeticOp
          // BDD node 416:op_add
          // EP node  66182:RotateLeftShifts
          // BDD node 154:rotate_left
          // EP node  67125:ArithmeticOp
          // BDD node 414:op_add
          // EP node  68230:RotateLeftShifts
          // BDD node 155:rotate_left
          // EP node  69185:ArithmeticOp
          // BDD node 417:op_xor
          // EP node  69988:RotateLeftShifts
          // BDD node 156:rotate_left
          // EP node  70637:ArithmeticOp
          // BDD node 420:op_xor
          // EP node  71290:ArithmeticOp
          // BDD node 418:op_xor
          // EP node  72108:RotateLeftShifts
          // BDD node 157:rotate_left
          // EP node  72769:ArithmeticOp
          // BDD node 419:op_add
          // EP node  73597:ArithmeticOp
          // BDD node 422:op_xor
          // EP node  74266:ArithmeticOp
          // BDD node 421:op_add
          // EP node  75434:RotateLeftShifts
          // BDD node 158:rotate_left
          // EP node  76443:RotateLeftShifts
          // BDD node 159:rotate_left
          // EP node  77291:RotateLeftShifts
          // BDD node 160:rotate_left
          // EP node  77976:ArithmeticOp
          // BDD node 423:op_add
          // EP node  78834:ArithmeticOp
          // BDD node 426:op_xor
          // EP node  79527:ArithmeticOp
          // BDD node 424:op_xor
          // EP node  80224:RotateLeftShifts
          // BDD node 161:rotate_left
          // EP node  80753:ArithmeticOp
          // BDD node 425:op_add
          // EP node  81804:ArithmeticOp
          // BDD node 427:op_add
          // EP node  82860:Recirculate
          // BDD node 164:rotate_left
          fwd_op = fwd_op_t.RECIRCULATE;
          build_recirc_hdr(2);
      } else if (hdr.recirc.code_path == 2) {
            // EP node  83931:RotateLeftShifts
            // BDD node 164:rotate_left
            compute_rotate_left_164_x();
            compute_rotate_left_164_shl();
            compute_rotate_left_164_or();
            compute_rotate_left_165_x();
            compute_rotate_left_165_shl();
            compute_rotate_left_165_or();
            compute_rotate_left_168_x();
            compute_rotate_left_168_shl();
            compute_rotate_left_168_or();
            compute_rotate_left_171_x();
            compute_rotate_left_171_shl();
            compute_rotate_left_171_or();
            compute_rotate_left_174_x();
            compute_rotate_left_174_shl();
            compute_rotate_left_174_or();
            compute_rotate_left_177_x();
            compute_rotate_left_177_shl();
            compute_rotate_left_177_or();
            compute_op_xor_456();
            compute_op_xor_457();
            // EP node  85000:RotateLeftShifts
            // BDD node 162:rotate_left
            // EP node  85898:ArithmeticOp
            // BDD node 428:op_xor
            // EP node  86801:ArithmeticOp
            // BDD node 430:op_xor
            // EP node  87530:ArithmeticOp
            // BDD node 429:op_add
            // EP node  88803:RotateLeftShifts
            // BDD node 165:rotate_left
            // EP node  89902:RotateLeftShifts
            // BDD node 163:rotate_left
            // EP node  90825:RotateLeftShifts
            // BDD node 166:rotate_left
            // EP node  91570:ArithmeticOp
            // BDD node 432:op_xor
            // EP node  92319:ArithmeticOp
            // BDD node 431:op_add
            // EP node  93257:RotateLeftShifts
            // BDD node 167:rotate_left
            // EP node  94014:ArithmeticOp
            // BDD node 433:op_add
            // EP node  94962:RotateLeftShifts
            // BDD node 168:rotate_left
            // EP node  95727:ArithmeticOp
            // BDD node 436:op_xor
            // EP node  96496:ArithmeticOp
            // BDD node 434:op_xor
            // EP node  97459:RotateLeftShifts
            // BDD node 169:rotate_left
            // EP node  98236:ArithmeticOp
            // BDD node 435:op_add
            // EP node  99209:ArithmeticOp
            // BDD node 437:op_add
            // EP node  100380:RotateLeftShifts
            // BDD node 171:rotate_left
            // EP node  101363:RotateLeftShifts
            // BDD node 170:rotate_left
            // EP node  102156:ArithmeticOp
            // BDD node 438:op_xor
            // EP node  103149:RotateLeftShifts
            // BDD node 172:rotate_left
            // EP node  103950:ArithmeticOp
            // BDD node 439:op_add
            // EP node  104953:RotateLeftShifts
            // BDD node 173:rotate_left
            // EP node  105762:ArithmeticOp
            // BDD node 440:op_xor
            // EP node  106575:ArithmeticOp
            // BDD node 441:op_add
            // EP node  107593:RotateLeftShifts
            // BDD node 174:rotate_left
            // EP node  108414:ArithmeticOp
            // BDD node 442:op_xor
            // EP node  109442:RotateLeftShifts
            // BDD node 175:rotate_left
            // EP node  110271:ArithmeticOp
            // BDD node 444:op_xor
            // EP node  111104:ArithmeticOp
            // BDD node 443:op_add
            // EP node  112147:ArithmeticOp
            // BDD node 445:op_add
            // EP node  113402:RotateLeftShifts
            // BDD node 176:rotate_left
            // EP node  114455:RotateLeftShifts
            // BDD node 177:rotate_left
            // EP node  115304:ArithmeticOp
            // BDD node 450:op_xor
            // EP node  116157:ArithmeticOp
            // BDD node 451:op_add
            // EP node  117014:ArithmeticOp
            // BDD node 446:op_xor
            // EP node  118087:RotateLeftShifts
            // BDD node 178:rotate_left
            // EP node  118952:Ignore
            // BDD node 179:nf_set_rte_ipv4_udptcp_checksum
            // EP node  120034:ArithmeticOp
            // BDD node 452:op_add
            // EP node  120907:ArithmeticOp
            // BDD node 453:op_xor
            // EP node  121784:ArithmeticOp
            // BDD node 454:op_xor
            // EP node  122665:ArithmeticOp
            // BDD node 456:op_xor
            // EP node  123332:ArithmeticOp
            // BDD node 455:op_xor
            // EP node  124002:ArithmeticOp
            // BDD node 457:op_xor
            // EP node  124675:ModifyHeader
            // BDD node 180:packet_return_chunk
            swap_action_180();
            hdr_value_4();
            hdr_snapshot_5();
            hdr_value_6();
            hdr_snapshot_7();
            hdr_value_8();
            hdr_write_10();
                  hdr_write_9();
            
            hdr_write_11();
            hdr_write_12();
            // EP node  125351:ModifyHeader
            // BDD node 181:packet_return_chunk
            swap_action_181();
            hdr_write_13();
            hdr_write_14();
            hdr_write_15();
            // EP node  126709:Forward
            // BDD node 183:FORWARD
            nf_dev = meta.dev;
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
          // EP node  146281:Forward
          // BDD node 9:FORWARD
          nf_dev = (bit<16>)(hdr.hdr1.data5[31:24]);
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
            if ((16w0x0000) != (meta.dev)){
              // EP node  229:Then
              // BDD node 12:if
              // EP node  323:If
              // BDD node 13:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data4[7:0])) & (32w0x00000002))){
                // EP node  324:Then
                // BDD node 13:if
                // EP node  559:RotateLeftShifts
                // BDD node 16:rotate_left
                hdr.recirc.setValid();
    hdr.recirc_state.setValid();
                compute_rotate_left_16_or();
                // EP node  810:BloomFilterQuery
                // BDD node 14:bf_query
                meta.key_32b_0 = hdr.hdr1.data4;
                meta.key_32b_1 = hdr.hdr1.data5;
                meta.key_16b_2 = hdr.hdr2.ports[31:16];
                meta.key_16b_3 = hdr.hdr2.ports[15:0];
                bf_1073926928_hash_0_810_calc_810();
                bf_1073926928_hash_1_810_calc_810();
                meta.bf_est0 = 0;
                meta.bf_est1 = 0;
                bf_1073926928_row_0_read_execute();
                bf_1073926928_row_1_read_execute();
                // EP node  1117:If
                // BDD node 15:if
                if ((2w0) == (meta.bf_est1 ++ meta.bf_est0)){
                  // EP node  1118:Then
                  // BDD node 15:if
                  // EP node  129175:SendToController
                  // BDD node 88:vector_borrow
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(129175);
                  cpu_copy_0();
                  cpu_copy_1();
                } else {
                  // EP node  1119:Else
                  // BDD node 15:if
                  // EP node  1477:Forward
                  // BDD node 103:FORWARD
                  nf_dev = 16w0x0000;
                }
              } else {
                // EP node  325:Else
                // BDD node 13:if
                // EP node  1695:ArithmeticOp
                // BDD node 346:op_xor
                compute_op_xor_346();
                // EP node  1956:If
                // BDD node 104:if
                if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data4[7:0])) & (32w0x00000010))){
                  // EP node  1957:Then
                  // BDD node 104:if
                  // EP node  2244:RotateLeftShifts
                  // BDD node 108:rotate_left
                  compute_rotate_left_108_x();
                  compute_rotate_left_108_shl();
                  compute_rotate_left_108_or();
                  // EP node  2679:VectorTableLookup
                  // BDD node 105:vector_borrow
                  meta.key_32b_0 = 32w0x00000000;
                  vector_table_1073939504_105.apply();
                  // EP node  2955:Ignore
                  // BDD node 106:vector_return
                  // EP node  3239:ArithmeticOp
                  // BDD node 380:op_shl
                  compute_op_shl_380_a();
                  compute_op_add_347();
                  compute_rotate_left_111_x();
                  compute_tdelta();
                  compute_rotate_left_111_shl();
                  compute_rotate_left_111_or();
                  compute_tdelta_move();
                  compute_rotate_left_114_x();
                  compute_rotate_left_114_shl();
                  compute_rotate_left_114_or();
                  compute_rotate_left_116_shl();
                  compute_rotate_left_116_or();
                  compute_rotate_left_117_or();
                  compute_rotate_left_119_shl();
                  compute_rotate_left_119_or();
                  compute_rotate_left_120_shl();
                  compute_rotate_left_120_or();
                  compute_rotate_left_123_x();
                  compute_rotate_left_123_shl();
                  compute_rotate_left_123_or();
                  compute_rotate_left_125_or();
                  // EP node  3497:RotateLeftShifts
                  // BDD node 109:rotate_left
                  // EP node  3726:ArithmeticOp
                  // BDD node 347:op_add
                  // EP node  3998:RotateLeftShifts
                  // BDD node 111:rotate_left
                  // EP node  4239:RotateLeftShifts
                  // BDD node 107:rotate_left
                  // EP node  4525:RotateLeftShifts
                  // BDD node 110:rotate_left
                  // EP node  4778:ArithmeticOp
                  // BDD node 351:op_xor
                  // EP node  5037:ArithmeticOp
                  // BDD node 352:op_add
                  // EP node  5344:ArithmeticOp
                  // BDD node 348:op_xor
                  // EP node  5658:RotateLeftShifts
                  // BDD node 114:rotate_left
                  // EP node  5935:ArithmeticOp
                  // BDD node 349:op_add
                  // EP node  6263:RotateLeftShifts
                  // BDD node 112:rotate_left
                  // EP node  6552:ArithmeticOp
                  // BDD node 381:op_or
                  // EP node  6800:ArithmeticOp
                  // BDD node 355:op_xor
                  // EP node  7053:ArithmeticOp
                  // BDD node 447:op_lshr
                  // EP node  7311:ArithmeticOp
                  // BDD node 350:op_add
                  // EP node  7624:RotateLeftShifts
                  // BDD node 113:rotate_left
                  // EP node  7892:ArithmeticOp
                  // BDD node 353:op_xor
                  // EP node  8217:ArithmeticOp
                  // BDD node 356:op_add
                  // EP node  8601:RotateLeftShifts
                  // BDD node 115:rotate_left
                  // EP node  8938:ArithmeticOp
                  // BDD node 354:op_add
                  // EP node  9336:RotateLeftShifts
                  // BDD node 116:rotate_left
                  // EP node  9685:RotateLeftShifts
                  // BDD node 117:rotate_left
                  // EP node  9983:ArithmeticOp
                  // BDD node 357:op_xor
                  // EP node  10344:ArithmeticOp
                  // BDD node 358:op_add
                  // EP node  10770:RotateLeftShifts
                  // BDD node 118:rotate_left
                  // EP node  11143:ArithmeticOp
                  // BDD node 359:op_xor
                  // EP node  11522:RotateLeftShifts
                  // BDD node 119:rotate_left
                  // EP node  11845:ArithmeticOp
                  // BDD node 360:op_add
                  // EP node  12236:ArithmeticOp
                  // BDD node 361:op_xor
                  // EP node  12697:RotateLeftShifts
                  // BDD node 120:rotate_left
                  // EP node  13100:ArithmeticOp
                  // BDD node 362:op_xor
                  // EP node  13443:ArithmeticOp
                  // BDD node 365:op_xor
                  // EP node  13791:ArithmeticOp
                  // BDD node 366:op_add
                  // EP node  14212:ArithmeticOp
                  // BDD node 363:op_xor
                  // EP node  14708:ArithmeticOp
                  // BDD node 364:op_add
                  // EP node  15281:RotateLeftShifts
                  // BDD node 121:rotate_left
                  // EP node  15791:RotateLeftShifts
                  // BDD node 123:rotate_left
                  // EP node  16236:ArithmeticOp
                  // BDD node 448:op_sub
                  // EP node  16687:ArithmeticOp
                  // BDD node 449:op_lshr
                  // EP node  17070:ArithmeticOp
                  // BDD node 369:op_xor
                  // EP node  17458:RotateLeftShifts
                  // BDD node 122:rotate_left
                  // EP node  17775:ArithmeticOp
                  // BDD node 367:op_xor
                  // EP node  18173:ArithmeticOp
                  // BDD node 368:op_add
                  // EP node  18654:RotateLeftShifts
                  // BDD node 124:rotate_left
                  // EP node  19062:RotateLeftShifts
                  // BDD node 125:rotate_left
                  // EP node  19395:ArithmeticOp
                  // BDD node 370:op_add
                  // EP node  20069:ArithmeticOp
                  // BDD node 371:op_xor
                  // EP node  21092:ArithmeticOp
                  // BDD node 373:op_xor
                  // EP node  22127:ArithmeticOp
                  // BDD node 372:op_add
                  // EP node  22908:Recirculate
                  // BDD node 375:op_xor
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(0);
                  recirc_copy_2();
                  recirc_copy_3();
                } else {
                  // EP node  1958:Else
                  // BDD node 104:if
                  // EP node  149308:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  230:Else
              // BDD node 12:if
              // EP node  129647:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data4[7:0])) & (32w0x00000040))){
                // EP node  129648:Then
                // BDD node 188:if
                // EP node  132535:Forward
                // BDD node 192:FORWARD
                nf_dev = (bit<16>)(hdr.hdr1.data5[31:24]);
              } else {
                // EP node  129649:Else
                // BDD node 188:if
                // EP node  132776:BloomFilterSet
                // BDD node 193:bf_set
                meta.key_32b_0 = hdr.hdr1.data4;
                meta.key_32b_1 = hdr.hdr1.data5;
                meta.key_16b_2 = hdr.hdr2.ports[31:16];
                meta.key_16b_3 = hdr.hdr2.ports[15:0];
                bf_1073926928_hash_0_132776_calc_132776();
                bf_1073926928_hash_1_132776_calc_132776();
                bf_1073926928_row_0_set_to_one_execute();
                bf_1073926928_row_1_set_to_one_execute();
                // EP node  135931:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  105:Else
          // BDD node 10:if
          // EP node  146779:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  137382:ParserCondition
          // BDD node 201:if
          // EP node  137383:Then
          // BDD node 201:if
          // EP node  141072:ArithmeticOp
          // BDD node 458:op_lshr
          compute_op_lshr_458();
          // EP node  144545:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  148039:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev)){
              // EP node  148040:Then
              // BDD node 203:if
              // EP node  149818:ParserCondition
              // BDD node 204:if
              // EP node  149819:Then
              // BDD node 204:if
              // EP node  151370:ParserCondition
              // BDD node 205:if
              // EP node  151371:Then
              // BDD node 205:if
              // EP node  161909:Forward
              // BDD node 209:FORWARD
              nf_dev = (bit<16>)(hdr.hdr1.data5[31:24]);
              // EP node  151372:Else
              // BDD node 205:if
              // EP node  154510:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  157661:Ignore
                // BDD node 211:vector_borrow
                // EP node  160831:SendToController
                // BDD node 212:vector_return
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(160831);
                cpu_copy_16();
              }
              // EP node  149820:Else
              // BDD node 204:if
              // EP node  161369:Forward
              // BDD node 221:FORWARD
              nf_dev = (bit<16>)(hdr.hdr1.data5[31:24]);
            } else {
              // EP node  148041:Else
              // BDD node 203:if
              // EP node  159505:Forward
              // BDD node 225:FORWARD
              nf_dev = (bit<16>)(hdr.hdr1.data5[31:24]);
            }
          }
          // EP node  137384:Else
          // BDD node 201:if
          // EP node  148800:Forward
          // BDD node 228:FORWARD
          nf_dev = (bit<16>)(hdr.hdr1.data5[31:24]);
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  140335:ParserReject
        // BDD node 230:DROP
      }

    }

    forwarding_tbl.apply();
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
