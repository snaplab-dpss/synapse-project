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
  bit<48> data0;
  bit<48> data1;
  bit<16> data2;
}
header hdr1_h {
  bit<96> data0;
  bit<64> data1;
}
struct vector_register_1073917816_0_pair_t {
  bit<32> lo;
  bit<32> hi;
}



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  bit<32> find_first_set_bit_7_key;
  bit<32> find_first_set_bit_7_out;
  bit<32> vector_reg_shadow0;
  bit<6> vector_reg_index0;
  bit<32> power_of_two_13_key;
  bit<32> power_of_two_13_out;
  bit<32> power_of_two_12_key;
  bit<32> power_of_two_12_out;
  bit<32> reg_incr0;
  bit<32> divide_16_denom;
  bit<32> vector_reg_value0;
  bit<32> ln_35_key;
  bit<32> ln_35_out;
  bit<32> ln_22_key;
  bit<32> ln_22_out;
  bit<32> hdr_val0;
  bit<32> hdr_val1;

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
    transition parser_4;
  }
  state parser_4 {
    transition parser_4_0;
  }
  state parser_4_0 {
    transition select (hdr.hdr0.data2) {
      16w0x0800: parser_5;
      default: parser_81;
    }
  }
  state parser_5 {
    pkt.extract(hdr.hdr1);
    transition parser_31;
  }
  state parser_81 {
    transition reject;
  }
  state parser_31 {
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
    fwd(CPU_PCIE_PORT);
  }

  action fwd_nf_dev(bit<16> port) {
    hdr.cpu.setInvalid();
    hdr.recirc.setInvalid();
    hdr.cuckoo.setInvalid();
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

  Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_6;
  bit<32> hash_6_value;
  action hash_6_calc() {
    hash_6_value = hash_6.get({
      hdr.hdr1.data1
      });
  }
  action find_first_set_bit_7_get_value(bit<32> v) {
    meta.find_first_set_bit_7_out = v;
  }
  table find_first_set_bit_7 {
    key = { meta.find_first_set_bit_7_key: ternary; }
    actions = { find_first_set_bit_7_get_value; }
    size = 37;
    default_action = find_first_set_bit_7_get_value(0);
    const entries = {
      32w1 &&& 32w1 : find_first_set_bit_7_get_value(1);
      32w2 &&& 32w3 : find_first_set_bit_7_get_value(2);
      32w4 &&& 32w7 : find_first_set_bit_7_get_value(3);
      32w8 &&& 32w15 : find_first_set_bit_7_get_value(4);
      32w16 &&& 32w31 : find_first_set_bit_7_get_value(5);
      32w32 &&& 32w63 : find_first_set_bit_7_get_value(6);
      32w64 &&& 32w127 : find_first_set_bit_7_get_value(7);
      32w128 &&& 32w255 : find_first_set_bit_7_get_value(8);
      32w256 &&& 32w511 : find_first_set_bit_7_get_value(9);
      32w512 &&& 32w1023 : find_first_set_bit_7_get_value(10);
      32w1024 &&& 32w2047 : find_first_set_bit_7_get_value(11);
      32w2048 &&& 32w4095 : find_first_set_bit_7_get_value(12);
      32w4096 &&& 32w8191 : find_first_set_bit_7_get_value(13);
      32w8192 &&& 32w16383 : find_first_set_bit_7_get_value(14);
      32w16384 &&& 32w32767 : find_first_set_bit_7_get_value(15);
      32w32768 &&& 32w65535 : find_first_set_bit_7_get_value(16);
      32w65536 &&& 32w131071 : find_first_set_bit_7_get_value(17);
      32w131072 &&& 32w262143 : find_first_set_bit_7_get_value(18);
      32w262144 &&& 32w524287 : find_first_set_bit_7_get_value(19);
      32w524288 &&& 32w1048575 : find_first_set_bit_7_get_value(20);
      32w1048576 &&& 32w2097151 : find_first_set_bit_7_get_value(21);
      32w2097152 &&& 32w4194303 : find_first_set_bit_7_get_value(22);
      32w4194304 &&& 32w8388607 : find_first_set_bit_7_get_value(23);
      32w8388608 &&& 32w16777215 : find_first_set_bit_7_get_value(24);
      32w16777216 &&& 32w33554431 : find_first_set_bit_7_get_value(25);
      32w33554432 &&& 32w67108863 : find_first_set_bit_7_get_value(26);
      32w67108864 &&& 32w134217727 : find_first_set_bit_7_get_value(27);
      32w134217728 &&& 32w268435455 : find_first_set_bit_7_get_value(28);
      32w268435456 &&& 32w536870911 : find_first_set_bit_7_get_value(29);
      32w536870912 &&& 32w1073741823 : find_first_set_bit_7_get_value(30);
      32w1073741824 &&& 32w2147483647 : find_first_set_bit_7_get_value(31);
      32w2147483648 &&& 32w4294967295 : find_first_set_bit_7_get_value(32);
    }
  }

  Register<vector_register_1073917816_0_pair_t,_>(64) vector_register_1073917816_0;

  RegisterAction<vector_register_1073917816_0_pair_t, bit<6>, bit<32>>(vector_register_1073917816_0) vector_register_1073917816_0_read_conditional_write_return_other_107 = {
    void apply(inout vector_register_1073917816_0_pair_t value, out bit<32> out_value) {
      vector_register_1073917816_0_pair_t in_value = value;
      if ((in_value.lo) < (meta.find_first_set_bit_7_out)) {
        value.lo = meta.find_first_set_bit_7_out;
        value.hi = in_value.lo;
      } else {
        value.lo = in_value.lo;
        value.hi = meta.find_first_set_bit_7_out;
      }
      out_value = value.hi;
    }
  };

  action regexec_vector_register_1073917816_0_read_conditional_write_return_other_107() {
    meta.vector_reg_shadow0 = vector_register_1073917816_0_read_conditional_write_return_other_107.execute(meta.vector_reg_index0);
  }
  action power_of_two_13_get_value(bit<32> v) {
    meta.power_of_two_13_out = v;
  }
  table power_of_two_13 {
    key = { meta.power_of_two_13_key: exact; }
    actions = { power_of_two_13_get_value; }
    size = 37;
    default_action = power_of_two_13_get_value(0);
    const entries = {
      32w0 : power_of_two_13_get_value(1);
      32w1 : power_of_two_13_get_value(2);
      32w2 : power_of_two_13_get_value(4);
      32w3 : power_of_two_13_get_value(8);
      32w4 : power_of_two_13_get_value(16);
      32w5 : power_of_two_13_get_value(32);
      32w6 : power_of_two_13_get_value(64);
      32w7 : power_of_two_13_get_value(128);
      32w8 : power_of_two_13_get_value(256);
      32w9 : power_of_two_13_get_value(512);
      32w10 : power_of_two_13_get_value(1024);
      32w11 : power_of_two_13_get_value(2048);
      32w12 : power_of_two_13_get_value(4096);
      32w13 : power_of_two_13_get_value(8192);
      32w14 : power_of_two_13_get_value(16384);
      32w15 : power_of_two_13_get_value(32768);
      32w16 : power_of_two_13_get_value(65536);
      32w17 : power_of_two_13_get_value(131072);
      32w18 : power_of_two_13_get_value(262144);
      32w19 : power_of_two_13_get_value(524288);
      32w20 : power_of_two_13_get_value(1048576);
      32w21 : power_of_two_13_get_value(2097152);
      32w22 : power_of_two_13_get_value(4194304);
      32w23 : power_of_two_13_get_value(8388608);
      32w24 : power_of_two_13_get_value(16777216);
      32w25 : power_of_two_13_get_value(33554432);
      32w26 : power_of_two_13_get_value(67108864);
      32w27 : power_of_two_13_get_value(134217728);
      32w28 : power_of_two_13_get_value(268435456);
      32w29 : power_of_two_13_get_value(536870912);
      32w30 : power_of_two_13_get_value(1073741824);
      32w31 : power_of_two_13_get_value(2147483648);
    }
  }

  action power_of_two_12_get_value(bit<32> v) {
    meta.power_of_two_12_out = v;
  }
  table power_of_two_12 {
    key = { meta.power_of_two_12_key: exact; }
    actions = { power_of_two_12_get_value; }
    size = 37;
    default_action = power_of_two_12_get_value(0);
    const entries = {
      32w0 : power_of_two_12_get_value(1);
      32w1 : power_of_two_12_get_value(2);
      32w2 : power_of_two_12_get_value(4);
      32w3 : power_of_two_12_get_value(8);
      32w4 : power_of_two_12_get_value(16);
      32w5 : power_of_two_12_get_value(32);
      32w6 : power_of_two_12_get_value(64);
      32w7 : power_of_two_12_get_value(128);
      32w8 : power_of_two_12_get_value(256);
      32w9 : power_of_two_12_get_value(512);
      32w10 : power_of_two_12_get_value(1024);
      32w11 : power_of_two_12_get_value(2048);
      32w12 : power_of_two_12_get_value(4096);
      32w13 : power_of_two_12_get_value(8192);
      32w14 : power_of_two_12_get_value(16384);
      32w15 : power_of_two_12_get_value(32768);
      32w16 : power_of_two_12_get_value(65536);
      32w17 : power_of_two_12_get_value(131072);
      32w18 : power_of_two_12_get_value(262144);
      32w19 : power_of_two_12_get_value(524288);
      32w20 : power_of_two_12_get_value(1048576);
      32w21 : power_of_two_12_get_value(2097152);
      32w22 : power_of_two_12_get_value(4194304);
      32w23 : power_of_two_12_get_value(8388608);
      32w24 : power_of_two_12_get_value(16777216);
      32w25 : power_of_two_12_get_value(33554432);
      32w26 : power_of_two_12_get_value(67108864);
      32w27 : power_of_two_12_get_value(134217728);
      32w28 : power_of_two_12_get_value(268435456);
      32w29 : power_of_two_12_get_value(536870912);
      32w30 : power_of_two_12_get_value(1073741824);
      32w31 : power_of_two_12_get_value(2147483648);
    }
  }

  Register<bit<32>,_>(1, 0) vector_register_1073935032_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073935032_0) vector_register_1073935032_0_add_value_333 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + meta.reg_incr0;
      out_value = value;
    }
  };

  MathUnit<bit<32>>(MathOp_t.DIV, 3046596202) divide_16_mu;
  Register<bit<32>,_>(1, 0) divide_16;
  RegisterAction<bit<32>, bit<8>, bit<32>>(divide_16) divide_16_calc = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = divide_16_mu.execute(meta.divide_16_denom);
      out_value = value;
    }
  };

  Register<bit<32>,_>(1, 0) vector_register_1073952248_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073952248_0) vector_register_1073952248_0_read_conditional_write_439 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
      if ((32w0x00000000) == (meta.vector_reg_shadow0)) {
        value = (32w0x00000001) + (value);
      }
    }
  };

  action ln_35_get_value(bit<32> v) {
    meta.ln_35_out = v;
  }
  table ln_35 {
    key = { meta.ln_35_key: exact; }
    actions = { ln_35_get_value; }
    size = 73;
    default_action = ln_35_get_value(0);
    const entries = {
      32w1 : ln_35_get_value(0);
      32w2 : ln_35_get_value(44);
      32w3 : ln_35_get_value(70);
      32w4 : ln_35_get_value(88);
      32w5 : ln_35_get_value(103);
      32w6 : ln_35_get_value(114);
      32w7 : ln_35_get_value(124);
      32w8 : ln_35_get_value(133);
      32w9 : ln_35_get_value(140);
      32w10 : ln_35_get_value(147);
      32w11 : ln_35_get_value(153);
      32w12 : ln_35_get_value(159);
      32w13 : ln_35_get_value(164);
      32w14 : ln_35_get_value(168);
      32w15 : ln_35_get_value(173);
      32w16 : ln_35_get_value(177);
      32w17 : ln_35_get_value(181);
      32w18 : ln_35_get_value(184);
      32w19 : ln_35_get_value(188);
      32w20 : ln_35_get_value(191);
      32w21 : ln_35_get_value(194);
      32w22 : ln_35_get_value(197);
      32w23 : ln_35_get_value(200);
      32w24 : ln_35_get_value(203);
      32w25 : ln_35_get_value(206);
      32w26 : ln_35_get_value(208);
      32w27 : ln_35_get_value(210);
      32w28 : ln_35_get_value(213);
      32w29 : ln_35_get_value(215);
      32w30 : ln_35_get_value(217);
      32w31 : ln_35_get_value(219);
      32w32 : ln_35_get_value(221);
      32w33 : ln_35_get_value(223);
      32w34 : ln_35_get_value(225);
      32w35 : ln_35_get_value(227);
      32w36 : ln_35_get_value(229);
      32w37 : ln_35_get_value(231);
      32w38 : ln_35_get_value(232);
      32w39 : ln_35_get_value(234);
      32w40 : ln_35_get_value(236);
      32w41 : ln_35_get_value(237);
      32w42 : ln_35_get_value(239);
      32w43 : ln_35_get_value(240);
      32w44 : ln_35_get_value(242);
      32w45 : ln_35_get_value(243);
      32w46 : ln_35_get_value(245);
      32w47 : ln_35_get_value(246);
      32w48 : ln_35_get_value(247);
      32w49 : ln_35_get_value(249);
      32w50 : ln_35_get_value(250);
      32w51 : ln_35_get_value(251);
      32w52 : ln_35_get_value(252);
      32w53 : ln_35_get_value(254);
      32w54 : ln_35_get_value(255);
      32w55 : ln_35_get_value(256);
      32w56 : ln_35_get_value(257);
      32w57 : ln_35_get_value(258);
      32w58 : ln_35_get_value(259);
      32w59 : ln_35_get_value(260);
      32w60 : ln_35_get_value(262);
      32w61 : ln_35_get_value(263);
      32w62 : ln_35_get_value(264);
      32w63 : ln_35_get_value(265);
      32w64 : ln_35_get_value(266);
    }
  }

  action ln_22_get_value(bit<32> v) {
    meta.ln_22_out = v;
  }
  table ln_22 {
    key = { meta.ln_22_key: exact; }
    actions = { ln_22_get_value; }
    size = 73;
    default_action = ln_22_get_value(0);
    const entries = {
      32w1 : ln_22_get_value(0);
      32w2 : ln_22_get_value(44);
      32w3 : ln_22_get_value(70);
      32w4 : ln_22_get_value(88);
      32w5 : ln_22_get_value(103);
      32w6 : ln_22_get_value(114);
      32w7 : ln_22_get_value(124);
      32w8 : ln_22_get_value(133);
      32w9 : ln_22_get_value(140);
      32w10 : ln_22_get_value(147);
      32w11 : ln_22_get_value(153);
      32w12 : ln_22_get_value(159);
      32w13 : ln_22_get_value(164);
      32w14 : ln_22_get_value(168);
      32w15 : ln_22_get_value(173);
      32w16 : ln_22_get_value(177);
      32w17 : ln_22_get_value(181);
      32w18 : ln_22_get_value(184);
      32w19 : ln_22_get_value(188);
      32w20 : ln_22_get_value(191);
      32w21 : ln_22_get_value(194);
      32w22 : ln_22_get_value(197);
      32w23 : ln_22_get_value(200);
      32w24 : ln_22_get_value(203);
      32w25 : ln_22_get_value(206);
      32w26 : ln_22_get_value(208);
      32w27 : ln_22_get_value(210);
      32w28 : ln_22_get_value(213);
      32w29 : ln_22_get_value(215);
      32w30 : ln_22_get_value(217);
      32w31 : ln_22_get_value(219);
      32w32 : ln_22_get_value(221);
      32w33 : ln_22_get_value(223);
      32w34 : ln_22_get_value(225);
      32w35 : ln_22_get_value(227);
      32w36 : ln_22_get_value(229);
      32w37 : ln_22_get_value(231);
      32w38 : ln_22_get_value(232);
      32w39 : ln_22_get_value(234);
      32w40 : ln_22_get_value(236);
      32w41 : ln_22_get_value(237);
      32w42 : ln_22_get_value(239);
      32w43 : ln_22_get_value(240);
      32w44 : ln_22_get_value(242);
      32w45 : ln_22_get_value(243);
      32w46 : ln_22_get_value(245);
      32w47 : ln_22_get_value(246);
      32w48 : ln_22_get_value(247);
      32w49 : ln_22_get_value(249);
      32w50 : ln_22_get_value(250);
      32w51 : ln_22_get_value(251);
      32w52 : ln_22_get_value(252);
      32w53 : ln_22_get_value(254);
      32w54 : ln_22_get_value(255);
      32w55 : ln_22_get_value(256);
      32w56 : ln_22_get_value(257);
      32w57 : ln_22_get_value(258);
      32w58 : ln_22_get_value(259);
      32w59 : ln_22_get_value(260);
      32w60 : ln_22_get_value(262);
      32w61 : ln_22_get_value(263);
      32w62 : ln_22_get_value(264);
      32w63 : ln_22_get_value(265);
      32w64 : ln_22_get_value(266);
    }
  }

  action swap_action_24() {
  }
  action swap_action_27() {
  }
  action swap_action_30() {
  }
  action swap_action_37() {
  }
  action swap_action_40() {
  }
  action swap_action_43() {
  }

  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:ParserExtraction
      // BDD node 3:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  5:ParserCondition
        // BDD node 4:if
        // EP node  6:Then
        // BDD node 4:if
        // EP node  16:ParserExtraction
        // BDD node 5:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  33:HashObj
          // BDD node 6:hash_obj
          hash_6_calc();
          bit<32> hll_hash0 = hash_6_value;
          // EP node  76:FindFirstSetBit
          // BDD node 7:find_first_set_bit
          meta.find_first_set_bit_7_key = (hll_hash0) & (32w0x000fffff);
          find_first_set_bit_7.apply();
          // EP node  107:VectorRegisterReadConditionalUpdateSingleAction
          // BDD node 8:vector_borrow
          meta.vector_reg_index0 = (bit<6>)((hll_hash0) >> (32w0x0000001a));
          regexec_vector_register_1073917816_0_read_conditional_write_return_other_107();
          // EP node  151:PowerOfTwo
          // BDD node 13:power_of_two
          meta.power_of_two_13_key = (32w0x00000014) - (meta.find_first_set_bit_7_out);
          power_of_two_13.apply();
          // EP node  199:PowerOfTwo
          // BDD node 12:power_of_two
          meta.power_of_two_12_key = (32w0x00000014) - (meta.vector_reg_shadow0);
          power_of_two_12.apply();
          // EP node  242:Ignore
          // BDD node 14:vector_borrow
          // EP node  333:VectorRegisterUpdate
          // BDD node 15:vector_return
          meta.reg_incr0 = (meta.power_of_two_12_out) - (meta.power_of_two_13_out);
          bit<32> reg_new0 = vector_register_1073935032_0_add_value_333.execute(32w0x00000000);
          // EP node  384:Divide
          // BDD node 16:divide
          meta.divide_16_denom = (32w0x04000000) - (reg_new0);
          bit<32> quotient0 = divide_16_calc.execute(0);
          // EP node  439:VectorRegisterReadConditionalIncrement
          // BDD node 17:vector_borrow
          meta.vector_reg_value0 = vector_register_1073952248_0_read_conditional_write_439.execute(32w0x00000000);
          // EP node  499:Ln
          // BDD node 35:ln
          meta.ln_35_key = (32w0x0000003f) - (meta.vector_reg_value0);
          ln_35.apply();
          // EP node  562:If
          // BDD node 18:if
          if ((32w0x00000000) != (meta.vector_reg_shadow0)){
            // EP node  563:Then
            // BDD node 18:if
            // EP node  635:Ln
            // BDD node 22:ln
            meta.ln_22_key = (32w0x00000040) - (meta.vector_reg_value0);
            ln_22.apply();
            // EP node  699:If
            // BDD node 20:if
            bool cond0 = false;
            if ((24w0x000000) == (quotient0[31:8])){
              if ((quotient0[7:0]) < (8w0xa0)){
                cond0 = true;
              }
            }
            if (cond0) {
              // EP node  700:Then
              // BDD node 20:if
              // EP node  977:If
              // BDD node 21:if
              if ((meta.vector_reg_value0) <= (32w0x0000003f)){
                // EP node  978:Then
                // BDD node 21:if
                // EP node  1191:ModifyHeader
                // BDD node 24:packet_return_chunk
                swap_action_24();
                meta.hdr_val0 = (32w0x0000010a) - (meta.ln_22_out);
                hdr.hdr0.data1[47:16] = meta.hdr_val0;
                hdr.hdr0.data1[15:8] = 8w0x00;
                hdr.hdr0.data1[7:0] = 8w0x00;
                // EP node  1291:Forward
                // BDD node 25:FORWARD
                nf_dev[15:0] = meta.dev[15:0];
              } else {
                // EP node  979:Else
                // BDD node 21:if
                // EP node  2690:ModifyHeader
                // BDD node 27:packet_return_chunk
                swap_action_27();
                hdr.hdr0.data1[47:16] = quotient0;
                hdr.hdr0.data1[15:8] = 8w0x00;
                hdr.hdr0.data1[7:0] = 8w0x00;
                // EP node  2971:Forward
                // BDD node 28:FORWARD
                nf_dev[15:0] = meta.dev[15:0];
              }
            } else {
              // EP node  701:Else
              // BDD node 20:if
              // EP node  870:ModifyHeader
              // BDD node 30:packet_return_chunk
              swap_action_30();
              hdr.hdr0.data1[47:16] = quotient0;
              hdr.hdr0.data1[15:8] = 8w0x00;
              hdr.hdr0.data1[7:0] = 8w0x00;
              // EP node  951:Forward
              // BDD node 31:FORWARD
              nf_dev[15:0] = meta.dev[15:0];
            }
          } else {
            // EP node  564:Else
            // BDD node 18:if
            // EP node  1322:If
            // BDD node 33:if
            bool cond1 = false;
            if ((24w0x000000) == (quotient0[31:8])){
              if ((quotient0[7:0]) < (8w0xa0)){
                cond1 = true;
              }
            }
            if (cond1) {
              // EP node  1323:Then
              // BDD node 33:if
              // EP node  1423:If
              // BDD node 34:if
              bool cond2 = false;
              if ((24w0x000000) == (meta.vector_reg_value0[31:8])){
                if ((meta.vector_reg_value0[7:0]) <= (8w0x3e)){
                  cond2 = true;
                }
              }
              if (cond2) {
                // EP node  1424:Then
                // BDD node 34:if
                // EP node  1693:ModifyHeader
                // BDD node 37:packet_return_chunk
                swap_action_37();
                meta.hdr_val1 = (32w0x0000010a) - (meta.ln_35_out);
                hdr.hdr0.data1[47:16] = meta.hdr_val1;
                hdr.hdr0.data1[15:8] = 8w0x00;
                hdr.hdr0.data1[7:0] = 8w0x00;
                // EP node  1817:Forward
                // BDD node 38:FORWARD
                nf_dev[15:0] = meta.dev[15:0];
              } else {
                // EP node  1425:Else
                // BDD node 34:if
                // EP node  2829:ModifyHeader
                // BDD node 40:packet_return_chunk
                swap_action_40();
                hdr.hdr0.data1[47:16] = quotient0;
                hdr.hdr0.data1[15:8] = 8w0x00;
                hdr.hdr0.data1[7:0] = 8w0x00;
                // EP node  3059:Forward
                // BDD node 41:FORWARD
                nf_dev[15:0] = meta.dev[15:0];
              }
            } else {
              // EP node  1324:Else
              // BDD node 33:if
              // EP node  2016:ModifyHeader
              // BDD node 43:packet_return_chunk
              swap_action_43();
              hdr.hdr0.data1[47:16] = quotient0;
              hdr.hdr0.data1[15:8] = 8w0x00;
              hdr.hdr0.data1[7:0] = 8w0x00;
              // EP node  2142:Forward
              // BDD node 44:FORWARD
              nf_dev[15:0] = meta.dev[15:0];
            }
          }
        }
        // EP node  7:Else
        // BDD node 4:if
        // EP node  2608:ParserReject
        // BDD node 81:DROP
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
