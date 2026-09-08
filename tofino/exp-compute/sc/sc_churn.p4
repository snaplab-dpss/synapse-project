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
  bit<32> rotate_left_18_out;
  bit<32> rotate_left_17_out;
  bit<32> rotate_left_16_out;
  bit<32> op_lshr_458_out;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> op_or_381_out;
  bit<32> op_lshr_449_out;
  bit<32> rotate_left_130_out;
  bit<32> rotate_left_131_out;
  bit<32> rotate_left_131_x_out;
  bit<32> op_xor_382_out;
  bit<32> op_add_384_out;
  bit<32> op_xor_379_out;
  bit<32> rotate_left_155_out;
  bit<32> rotate_left_156_out;
  bit<32> rotate_left_157_out;
  bit<32> rotate_left_157_x_out;
  bit<32> op_xor_422_out;
  bit<32> op_add_421_out;
  bit<32> op_xor_424_out;
  bit<32> op_xor_456_out;
  bit<32> op_xor_455_out;

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
  bit<24> data4;
  bit<8> data5;
  bit<24> data6;
  bit<8> data7;
}
header hdr2_h {
  bit<16> data0;
  bit<16> data1;
  bit<24> data2;
  bit<8> data3;
  bit<24> data4;
  bit<24> data5;
  bit<48> data6;
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
  cuckoo_h cuckoo;
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
  bit<32> rotate_left_16_out;
  bit<32> rotate_left_17_x_out;
  bit<32> rotate_left_17_out;
  bit<32> rotate_left_18_out;
  bit<32> bf_1073926928_estimate;
  bit<24> key_24b_0;
  bit<8> key_8b_1;
  bit<24> key_24b_2;
  bit<8> key_8b_3;
  bit<16> key_16b_4;
  bit<16> key_16b_5;
  bit<32> rotate_left_107_shl_out;
  bit<32> rotate_left_107_shr_out;
  bit<32> rotate_left_107_or_out;
  bit<32> op_shl_380_a_out;
  bit<32> op_shl_380_out;
  bit<32> rotate_left_108_x_out;
  bit<32> rotate_left_108_out;
  bit<32> rotate_left_110_x_out;
  bit<32> rotate_left_110_out;
  bit<32> key_32b_0;
  bit<32> rotate_left_109_out;
  bit<32> op_add_347_out;
  bit<32> rotate_left_111_x_out;
  bit<32> rotate_left_111_out;
  bit<32> op_lshr_447_out;
  bit<32> op_or_381_b_out;
  bit<32> op_or_381_out;
  bit<32> op_sub_448_out;
  bit<32> op_add_349_out;
  bit<32> rotate_left_112_x_out;
  bit<32> rotate_left_112_out;
  bit<32> op_xor_353_out;
  bit<32> rotate_left_113_x_out;
  bit<32> rotate_left_113_out;
  bit<32> op_add_352_out;
  bit<32> rotate_left_114_x_out;
  bit<32> rotate_left_114_out;
  bit<32> op_add_354_out;
  bit<32> rotate_left_115_x_out;
  bit<32> rotate_left_115_out;
  bit<32> op_add_356_out;
  bit<32> rotate_left_116_x_out;
  bit<32> rotate_left_116_out;
  bit<32> rotate_left_117_x_out;
  bit<32> rotate_left_117_out;
  bit<32> op_lshr_449_out;
  bit<32> op_add_358_out;
  bit<32> rotate_left_118_x_out;
  bit<32> rotate_left_118_out;
  bit<32> rotate_left_119_x_out;
  bit<32> rotate_left_119_out;
  bit<32> op_add_360_out;
  bit<32> op_xor_362_out;
  bit<32> op_xor_361_out;
  bit<32> rotate_left_121_x_out;
  bit<32> rotate_left_121_out;
  bit<32> op_xor_365_out;
  bit<32> rotate_left_120_x_out;
  bit<32> rotate_left_120_out;
  bit<32> op_add_366_out;
  bit<32> rotate_left_122_x_out;
  bit<32> rotate_left_122_out;
  bit<32> rotate_left_123_x_out;
  bit<32> rotate_left_123_out;
  bit<32> op_add_370_out;
  bit<32> rotate_left_126_x_out;
  bit<32> rotate_left_126_out;
  bit<32> rotate_left_124_x_out;
  bit<32> rotate_left_124_out;
  bit<32> rotate_left_125_x_out;
  bit<32> rotate_left_125_out;
  bit<32> rotate_left_127_x_out;
  bit<32> rotate_left_127_out;
  bit<32> rotate_left_128_x_out;
  bit<32> rotate_left_128_out;
  bit<32> op_add_374_out;
  bit<32> rotate_left_129_x_out;
  bit<32> rotate_left_129_out;
  bit<32> rotate_left_130_x_out;
  bit<32> rotate_left_130_out;
  bit<32> rotate_left_131_x_out;
  bit<32> rotate_left_131_out;
  bit<32> op_add_378_out;
  bit<32> op_xor_382_out;
  bit<32> op_add_384_out;
  bit<32> op_xor_379_out;
  bit<32> rotate_left_133_x_out;
  bit<32> rotate_left_133_out;
  bit<32> rotate_left_132_x_out;
  bit<32> rotate_left_132_out;
  bit<32> rotate_left_134_x_out;
  bit<32> rotate_left_134_shl_out;
  bit<32> rotate_left_134_shr_out;
  bit<32> rotate_left_134_or_out;
  bit<32> op_add_386_out;
  bit<32> rotate_left_135_x_out;
  bit<32> rotate_left_135_out;
  bit<32> rotate_left_136_x_out;
  bit<32> rotate_left_136_out;
  bit<32> op_add_390_out;
  bit<32> rotate_left_137_x_out;
  bit<32> rotate_left_137_out;
  bit<32> rotate_left_138_x_out;
  bit<32> rotate_left_138_out;
  bit<32> rotate_left_139_x_out;
  bit<32> rotate_left_139_out;
  bit<32> op_add_394_out;
  bit<32> op_xor_397_out;
  bit<32> rotate_left_140_x_out;
  bit<32> rotate_left_140_out;
  bit<32> rotate_left_141_x_out;
  bit<32> rotate_left_141_out;
  bit<32> rotate_left_142_x_out;
  bit<32> rotate_left_142_out;
  bit<32> op_add_398_out;
  bit<32> op_xor_399_out;
  bit<32> op_xor_400_out;
  bit<32> rotate_left_143_x_out;
  bit<32> rotate_left_143_out;
  bit<32> rotate_left_144_x_out;
  bit<32> rotate_left_144_out;
  bit<32> rotate_left_145_x_out;
  bit<32> rotate_left_145_out;
  bit<32> rotate_left_146_x_out;
  bit<32> rotate_left_146_out;
  bit<32> op_add_404_out;
  bit<32> rotate_left_147_x_out;
  bit<32> rotate_left_147_out;
  bit<32> rotate_left_148_x_out;
  bit<32> rotate_left_148_out;
  bit<32> rotate_left_149_x_out;
  bit<32> rotate_left_149_out;
  bit<32> op_add_408_out;
  bit<32> rotate_left_151_x_out;
  bit<32> rotate_left_151_out;
  bit<32> rotate_left_150_x_out;
  bit<32> rotate_left_150_out;
  bit<32> rotate_left_152_x_out;
  bit<32> rotate_left_152_out;
  bit<32> op_add_412_out;
  bit<32> rotate_left_153_x_out;
  bit<32> rotate_left_153_out;
  bit<32> rotate_left_154_x_out;
  bit<32> rotate_left_154_out;
  bit<32> rotate_left_155_x_out;
  bit<32> rotate_left_155_out;
  bit<32> op_add_416_out;
  bit<32> rotate_left_156_x_out;
  bit<32> rotate_left_156_out;
  bit<32> op_xor_417_out;
  bit<32> rotate_left_157_x_out;
  bit<32> rotate_left_157_out;
  bit<32> op_xor_422_out;
  bit<32> op_add_421_out;
  bit<32> op_xor_424_out;
  bit<32> rotate_left_160_x_out;
  bit<32> rotate_left_160_out;
  bit<32> rotate_left_159_x_out;
  bit<32> rotate_left_159_shl_out;
  bit<32> rotate_left_159_shr_out;
  bit<32> rotate_left_159_or_out;
  bit<32> rotate_left_158_x_out;
  bit<32> rotate_left_158_out;
  bit<32> op_add_425_out;
  bit<32> op_xor_428_out;
  bit<32> rotate_left_162_x_out;
  bit<32> rotate_left_162_out;
  bit<32> rotate_left_161_x_out;
  bit<32> rotate_left_161_shl_out;
  bit<32> rotate_left_161_shr_out;
  bit<32> rotate_left_161_or_out;
  bit<32> rotate_left_163_x_out;
  bit<32> rotate_left_163_out;
  bit<32> rotate_left_164_x_out;
  bit<32> rotate_left_164_out;
  bit<32> op_add_429_out;
  bit<32> rotate_left_165_x_out;
  bit<32> rotate_left_165_shl_out;
  bit<32> rotate_left_165_shr_out;
  bit<32> rotate_left_165_or_out;
  bit<32> op_add_433_out;
  bit<32> rotate_left_166_x_out;
  bit<32> rotate_left_166_out;
  bit<32> rotate_left_167_x_out;
  bit<32> rotate_left_167_shl_out;
  bit<32> rotate_left_167_shr_out;
  bit<32> rotate_left_167_or_out;
  bit<32> rotate_left_169_x_out;
  bit<32> rotate_left_169_out;
  bit<32> rotate_left_168_x_out;
  bit<32> rotate_left_168_out;
  bit<32> op_add_437_out;
  bit<32> rotate_left_170_x_out;
  bit<32> rotate_left_170_out;
  bit<32> rotate_left_171_x_out;
  bit<32> rotate_left_171_out;
  bit<32> op_add_439_out;
  bit<32> rotate_left_173_x_out;
  bit<32> rotate_left_173_shl_out;
  bit<32> rotate_left_173_shr_out;
  bit<32> rotate_left_173_or_out;
  bit<32> rotate_left_172_x_out;
  bit<32> rotate_left_172_out;
  bit<32> op_add_441_out;
  bit<32> rotate_left_174_x_out;
  bit<32> rotate_left_174_out;
  bit<32> op_add_443_out;
  bit<32> rotate_left_175_x_out;
  bit<32> rotate_left_175_out;
  bit<32> rotate_left_176_x_out;
  bit<32> rotate_left_176_out;
  bit<32> op_add_445_out;
  bit<32> rotate_left_177_x_out;
  bit<32> rotate_left_177_shl_out;
  bit<32> rotate_left_177_shr_out;
  bit<32> rotate_left_177_or_out;
  bit<32> op_add_451_out;
  bit<32> op_add_452_out;
  bit<32> rotate_left_178_x_out;
  bit<32> rotate_left_178_out;
  bit<32> op_xor_456_out;
  bit<32> op_xor_453_out;
  bit<32> op_xor_454_out;
  bit<32> op_xor_455_out;
  bit<32> op_xor_457_out;
  bit<32> hdr_val0;
  bit<32> hdr_val1;
  bit<32> hdr_val2;
  bit<32> op_lshr_458_out;

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

  action compute_rotate_left_16() {
    meta.rotate_left_16_out = 32w2225785509;
    meta.rotate_left_17_x_out = meta.rotate_left_17_x_out;
    meta.rotate_left_18_out = 32w3399118710;
  }

  action compute_rotate_left_17() {
    meta.rotate_left_17_out = meta.rotate_left_17_x_out[23:0] ++ meta.rotate_left_17_x_out[31:24];
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
    meta.bf_1073926928_estimate[0:0] = bf_1073926928_row_0_read_and_set_value[0:0];
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
    meta.bf_1073926928_estimate[1:1] = bf_1073926928_row_1_read_value[0:0];
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_1539;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_1539;

  action bf_1073926928_hash_0_1539_calc_1539() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_1539.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_1539_calc_1539() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_1539.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0x2681580b
    });
  }
  action compute_rotate_left_107_shl() {
    meta.rotate_left_107_shl_out = 32w0x2c255655 << 5;
    meta.rotate_left_107_shr_out = 32w0x2c255655 >> 27;
    meta.op_shl_380_a_out = meta.op_shl_380_a_out;
    meta.rotate_left_108_x_out = meta.rotate_left_108_x_out;
  }

  action compute_rotate_left_107_or() {
    meta.rotate_left_107_or_out = meta.rotate_left_107_shl_out | meta.rotate_left_107_shr_out;
    meta.op_shl_380_out = meta.op_shl_380_a_out << 32w0x00000010;
    meta.rotate_left_108_out = meta.rotate_left_108_x_out[23:0] ++ meta.rotate_left_108_x_out[31:24];
  }

  action compute_rotate_left_110_x() {
    meta.rotate_left_110_x_out = meta.rotate_left_110_x_out;
  }

  action compute_rotate_left_110() {
    @in_hash { meta.rotate_left_110_out = meta.rotate_left_110_x_out[18:0] ++ meta.rotate_left_110_x_out[31:19]; }
  }

  bit<32> vector_table_1073939504_105_get_value_param0 = 32w0;
  action vector_table_1073939504_105_get_value(bit<32> _vector_table_1073939504_105_get_value_param0) {
    vector_table_1073939504_105_get_value_param0 = _vector_table_1073939504_105_get_value_param0;
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

  action compute_rotate_left_109() {
    meta.rotate_left_109_out = 32w3399118710;
    meta.op_lshr_447_out = meta.time;
    meta.op_or_381_b_out = meta.op_or_381_b_out;
  }

  action compute_op_add_347() {
    meta.op_add_347_out = 32w0x5d476351 + meta.rotate_left_108_x_out;
  }

  action compute_rotate_left_111_x() {
    meta.rotate_left_111_x_out = meta.rotate_left_111_x_out;
    meta.op_or_381_out = meta.op_shl_380_out | meta.op_or_381_b_out;
  }

  action compute_rotate_left_111() {
    @in_hash { meta.rotate_left_111_out = meta.rotate_left_111_x_out[24:0] ++ meta.rotate_left_111_x_out[31:25]; }
    meta.op_add_349_out = meta.rotate_left_108_x_out + meta.rotate_left_110_x_out;
    meta.op_add_352_out = meta.rotate_left_109_out + meta.rotate_left_111_x_out;
  }

  action compute_rotate_left_112_x() {
    meta.rotate_left_112_x_out = meta.rotate_left_112_x_out;
    meta.rotate_left_114_x_out = meta.rotate_left_114_x_out;
  }

  action compute_op_sub_448() {
    meta.op_sub_448_out = meta.op_lshr_447_out[31:0] - vector_table_1073939504_105_get_value_param0;
    meta.rotate_left_112_out = meta.rotate_left_112_x_out[15:0] ++ meta.rotate_left_112_x_out[31:16];
    meta.op_xor_353_out = meta.rotate_left_110_out ^ meta.rotate_left_112_x_out;
    meta.rotate_left_113_x_out = meta.rotate_left_113_x_out;
    meta.rotate_left_114_out = meta.rotate_left_114_x_out[23:0] ++ meta.rotate_left_114_x_out[31:24];
  }

  action compute_rotate_left_113() {
    @in_hash { meta.rotate_left_113_out = meta.rotate_left_113_x_out[26:0] ++ meta.rotate_left_113_x_out[31:27]; }
    meta.op_add_354_out = meta.op_add_352_out + meta.op_xor_353_out;
    meta.rotate_left_115_x_out = meta.rotate_left_115_x_out;
    meta.op_add_356_out = meta.rotate_left_112_out + meta.rotate_left_114_x_out;
    meta.op_lshr_449_out = meta.op_sub_448_out >> 32w0x0000000c;
  }

  action compute_rotate_left_115() {
    meta.rotate_left_115_out = meta.rotate_left_115_x_out[15:0] ++ meta.rotate_left_115_x_out[31:16];
    meta.rotate_left_116_x_out = meta.rotate_left_116_x_out;
    meta.rotate_left_117_x_out = meta.rotate_left_117_x_out;
  }

  action compute_rotate_left_116() {
    @in_hash { meta.rotate_left_116_out = meta.rotate_left_116_x_out[18:0] ++ meta.rotate_left_116_x_out[31:19]; }
  }

  action compute_rotate_left_117() {
    @in_hash { meta.rotate_left_117_out = meta.rotate_left_117_x_out[24:0] ++ meta.rotate_left_117_x_out[31:25]; }
    meta.op_add_358_out = meta.op_add_356_out + meta.rotate_left_116_x_out;
    meta.rotate_left_118_x_out = meta.rotate_left_118_x_out;
    meta.op_add_360_out = meta.rotate_left_115_out + meta.rotate_left_117_x_out;
  }

  action compute_rotate_left_118() {
    meta.rotate_left_118_out = meta.rotate_left_118_x_out[15:0] ++ meta.rotate_left_118_x_out[31:16];
    meta.rotate_left_119_x_out = meta.rotate_left_119_x_out;
    meta.op_xor_362_out = meta.op_add_360_out ^ (hdr.hdr1.data4 ++ hdr.hdr1.data5);
    meta.op_xor_361_out = meta.rotate_left_117_out ^ meta.op_add_360_out;
  }

  action compute_rotate_left_119() {
    @in_hash { meta.rotate_left_119_out = meta.rotate_left_119_x_out[26:0] ++ meta.rotate_left_119_x_out[31:27]; }
    meta.rotate_left_121_x_out = meta.rotate_left_121_x_out;
    meta.op_xor_365_out = meta.op_xor_361_out ^ (hdr.hdr1.data6 ++ hdr.hdr1.data7);
    meta.rotate_left_120_x_out = meta.rotate_left_120_x_out;
  }

  action compute_rotate_left_121() {
    meta.rotate_left_121_out = meta.rotate_left_121_x_out[15:0] ++ meta.rotate_left_121_x_out[31:16];
    meta.rotate_left_120_out = meta.rotate_left_120_x_out[23:0] ++ meta.rotate_left_120_x_out[31:24];
    meta.op_add_366_out = meta.rotate_left_118_out + meta.op_xor_365_out;
    meta.rotate_left_122_x_out = meta.rotate_left_122_x_out;
  }

  action compute_rotate_left_122() {
    @in_hash { meta.rotate_left_122_out = meta.rotate_left_122_x_out[18:0] ++ meta.rotate_left_122_x_out[31:19]; }
    meta.rotate_left_123_x_out = meta.rotate_left_123_x_out;
    meta.rotate_left_124_x_out = meta.rotate_left_124_x_out;
  }

  action compute_rotate_left_123() {
    @in_hash { meta.rotate_left_123_out = meta.rotate_left_123_x_out[24:0] ++ meta.rotate_left_123_x_out[31:25]; }
    meta.op_add_370_out = meta.rotate_left_121_out + meta.rotate_left_123_x_out;
    meta.rotate_left_124_out = meta.rotate_left_124_x_out[15:0] ++ meta.rotate_left_124_x_out[31:16];
    meta.rotate_left_125_x_out = meta.rotate_left_125_x_out;
  }

  action compute_rotate_left_126_x() {
    meta.rotate_left_126_x_out = meta.rotate_left_126_x_out;
    @in_hash { meta.rotate_left_125_out = meta.rotate_left_125_x_out[26:0] ++ meta.rotate_left_125_x_out[31:27]; }
    meta.rotate_left_127_x_out = meta.rotate_left_127_x_out;
  }

  action compute_rotate_left_126() {
    meta.rotate_left_126_out = meta.rotate_left_126_x_out[23:0] ++ meta.rotate_left_126_x_out[31:24];
    meta.rotate_left_127_out = meta.rotate_left_127_x_out[15:0] ++ meta.rotate_left_127_x_out[31:16];
    meta.rotate_left_128_x_out = meta.rotate_left_128_x_out;
    meta.op_add_374_out = meta.rotate_left_124_out + meta.rotate_left_126_x_out;
  }

  action compute_rotate_left_128() {
    @in_hash { meta.rotate_left_128_out = meta.rotate_left_128_x_out[18:0] ++ meta.rotate_left_128_x_out[31:19]; }
    meta.rotate_left_129_x_out = meta.rotate_left_129_x_out;
    meta.rotate_left_130_x_out = meta.rotate_left_130_x_out;
  }

  action compute_rotate_left_129() {
    @in_hash { meta.rotate_left_129_out = meta.rotate_left_129_x_out[24:0] ++ meta.rotate_left_129_x_out[31:25]; }
    meta.rotate_left_130_out = meta.rotate_left_130_x_out[15:0] ++ meta.rotate_left_130_x_out[31:16];
    meta.rotate_left_131_x_out = meta.rotate_left_131_x_out;
    meta.op_add_378_out = meta.rotate_left_127_out + meta.rotate_left_129_x_out;
  }

  action compute_rotate_left_131() {
    @in_hash { meta.rotate_left_131_out = meta.rotate_left_131_x_out[26:0] ++ meta.rotate_left_131_x_out[31:27]; }
    meta.op_xor_382_out = meta.op_add_378_out ^ (hdr.hdr1.data6 ++ hdr.hdr1.data7);
    meta.op_xor_379_out = meta.rotate_left_129_out ^ meta.op_add_378_out;
  }

  action compute_op_add_384() {
    meta.op_add_384_out = meta.op_xor_382_out + meta.rotate_left_131_x_out;
  }

  action compute_rotate_left_133_x() {
    meta.rotate_left_133_x_out = meta.rotate_left_133_x_out;
    meta.rotate_left_132_x_out = meta.rotate_left_132_x_out;
    meta.rotate_left_134_x_out = meta.rotate_left_134_x_out;
  }

  action compute_rotate_left_133() {
    meta.rotate_left_133_out = meta.rotate_left_133_x_out[15:0] ++ meta.rotate_left_133_x_out[31:16];
    meta.rotate_left_132_out = meta.rotate_left_132_x_out[23:0] ++ meta.rotate_left_132_x_out[31:24];
    meta.rotate_left_134_shl_out = meta.rotate_left_134_x_out << 13;
    meta.rotate_left_134_shr_out = meta.rotate_left_134_x_out >> 19;
    meta.op_add_386_out = hdr.recirc.rotate_left_130_out + meta.rotate_left_132_x_out;
  }

  action compute_rotate_left_134_or() {
    meta.rotate_left_134_or_out = meta.rotate_left_134_shl_out | meta.rotate_left_134_shr_out;
    meta.rotate_left_135_x_out = meta.rotate_left_135_x_out;
    meta.rotate_left_136_x_out = meta.rotate_left_136_x_out;
  }

  action compute_rotate_left_135() {
    @in_hash { meta.rotate_left_135_out = meta.rotate_left_135_x_out[24:0] ++ meta.rotate_left_135_x_out[31:25]; }
    meta.rotate_left_136_out = meta.rotate_left_136_x_out[15:0] ++ meta.rotate_left_136_x_out[31:16];
    meta.op_add_390_out = meta.rotate_left_133_out + meta.rotate_left_135_x_out;
    meta.rotate_left_137_x_out = meta.rotate_left_137_x_out;
  }

  action compute_rotate_left_137() {
    @in_hash { meta.rotate_left_137_out = meta.rotate_left_137_x_out[26:0] ++ meta.rotate_left_137_x_out[31:27]; }
    meta.rotate_left_138_x_out = meta.rotate_left_138_x_out;
    meta.rotate_left_139_x_out = meta.rotate_left_139_x_out;
  }

  action compute_rotate_left_138() {
    meta.rotate_left_138_out = meta.rotate_left_138_x_out[23:0] ++ meta.rotate_left_138_x_out[31:24];
    meta.rotate_left_139_out = meta.rotate_left_139_x_out[15:0] ++ meta.rotate_left_139_x_out[31:16];
    meta.op_add_394_out = meta.rotate_left_136_out + meta.rotate_left_138_x_out;
    meta.rotate_left_140_x_out = meta.rotate_left_140_x_out;
  }

  action compute_op_xor_397() {
    meta.op_xor_397_out = meta.rotate_left_138_out ^ meta.op_add_394_out;
    @in_hash { meta.rotate_left_140_out = meta.rotate_left_140_x_out[18:0] ++ meta.rotate_left_140_x_out[31:19]; }
    meta.rotate_left_141_x_out = meta.rotate_left_141_x_out;
    meta.rotate_left_142_x_out = meta.rotate_left_142_x_out;
  }

  action compute_rotate_left_141() {
    @in_hash { meta.rotate_left_141_out = meta.rotate_left_141_x_out[24:0] ++ meta.rotate_left_141_x_out[31:25]; }
    meta.rotate_left_142_out = meta.rotate_left_142_x_out[15:0] ++ meta.rotate_left_142_x_out[31:16];
    meta.op_add_398_out = meta.rotate_left_139_out + meta.op_xor_397_out;
    meta.rotate_left_143_x_out = meta.rotate_left_143_x_out;
  }

  action compute_op_xor_399() {
    meta.op_xor_399_out = meta.rotate_left_141_out ^ meta.op_add_398_out;
    meta.op_xor_400_out = meta.op_add_398_out ^ hdr.recirc.op_or_381_out;
    @in_hash { meta.rotate_left_143_out = meta.rotate_left_143_x_out[26:0] ++ meta.rotate_left_143_x_out[31:27]; }
  }

  action compute_rotate_left_144_x() {
    meta.rotate_left_144_x_out = meta.rotate_left_144_x_out;
    meta.rotate_left_145_x_out = meta.rotate_left_145_x_out;
  }

  action compute_rotate_left_144() {
    meta.rotate_left_144_out = meta.rotate_left_144_x_out[23:0] ++ meta.rotate_left_144_x_out[31:24];
    meta.rotate_left_145_out = meta.rotate_left_145_x_out[15:0] ++ meta.rotate_left_145_x_out[31:16];
    meta.rotate_left_146_x_out = meta.rotate_left_146_x_out;
    meta.op_add_404_out = meta.rotate_left_142_out + meta.rotate_left_144_x_out;
  }

  action compute_rotate_left_146() {
    @in_hash { meta.rotate_left_146_out = meta.rotate_left_146_x_out[18:0] ++ meta.rotate_left_146_x_out[31:19]; }
    meta.rotate_left_147_x_out = meta.rotate_left_147_x_out;
    meta.rotate_left_148_x_out = meta.rotate_left_148_x_out;
  }

  action compute_rotate_left_147() {
    @in_hash { meta.rotate_left_147_out = meta.rotate_left_147_x_out[24:0] ++ meta.rotate_left_147_x_out[31:25]; }
    meta.rotate_left_148_out = meta.rotate_left_148_x_out[15:0] ++ meta.rotate_left_148_x_out[31:16];
    meta.rotate_left_149_x_out = meta.rotate_left_149_x_out;
    meta.op_add_408_out = meta.rotate_left_145_out + meta.rotate_left_147_x_out;
  }

  action compute_rotate_left_149() {
    @in_hash { meta.rotate_left_149_out = meta.rotate_left_149_x_out[26:0] ++ meta.rotate_left_149_x_out[31:27]; }
    meta.rotate_left_151_x_out = meta.rotate_left_151_x_out;
    meta.rotate_left_150_x_out = meta.rotate_left_150_x_out;
  }

  action compute_rotate_left_151() {
    meta.rotate_left_151_out = meta.rotate_left_151_x_out[15:0] ++ meta.rotate_left_151_x_out[31:16];
    meta.rotate_left_150_out = meta.rotate_left_150_x_out[23:0] ++ meta.rotate_left_150_x_out[31:24];
    meta.rotate_left_152_x_out = meta.rotate_left_152_x_out;
    meta.op_add_412_out = meta.rotate_left_148_out + meta.rotate_left_150_x_out;
  }

  action compute_rotate_left_152() {
    @in_hash { meta.rotate_left_152_out = meta.rotate_left_152_x_out[18:0] ++ meta.rotate_left_152_x_out[31:19]; }
    meta.rotate_left_153_x_out = meta.rotate_left_153_x_out;
    meta.rotate_left_154_x_out = meta.rotate_left_154_x_out;
  }

  action compute_rotate_left_153() {
    @in_hash { meta.rotate_left_153_out = meta.rotate_left_153_x_out[24:0] ++ meta.rotate_left_153_x_out[31:25]; }
    meta.rotate_left_154_out = meta.rotate_left_154_x_out[15:0] ++ meta.rotate_left_154_x_out[31:16];
    meta.rotate_left_155_x_out = meta.rotate_left_155_x_out;
    meta.op_add_416_out = meta.rotate_left_151_out + meta.rotate_left_153_x_out;
  }

  action compute_rotate_left_155() {
    @in_hash { meta.rotate_left_155_out = meta.rotate_left_155_x_out[26:0] ++ meta.rotate_left_155_x_out[31:27]; }
    meta.rotate_left_156_x_out = meta.rotate_left_156_x_out;
    meta.op_xor_417_out = meta.op_add_416_out ^ (hdr.hdr2.data2 ++ hdr.hdr2.data3);
  }

  action compute_rotate_left_156() {
    meta.rotate_left_156_out = meta.rotate_left_156_x_out[23:0] ++ meta.rotate_left_156_x_out[31:24];
    meta.rotate_left_157_x_out = meta.rotate_left_157_x_out;
    meta.op_add_421_out = meta.rotate_left_154_out + meta.rotate_left_156_x_out;
  }

  action compute_rotate_left_157() {
    meta.rotate_left_157_out = meta.rotate_left_157_x_out[15:0] ++ meta.rotate_left_157_x_out[31:16];
    meta.op_xor_422_out = meta.rotate_left_155_out ^ meta.rotate_left_157_x_out;
    meta.op_xor_424_out = meta.rotate_left_156_out ^ meta.op_add_421_out;
  }

  action compute_rotate_left_160_x() {
    meta.rotate_left_160_x_out = meta.rotate_left_160_x_out;
    meta.rotate_left_159_x_out = meta.rotate_left_159_x_out;
    meta.rotate_left_158_x_out = meta.rotate_left_158_x_out;
    meta.op_add_425_out = hdr.recirc.rotate_left_157_out + hdr.recirc.op_xor_424_out;
  }

  action compute_rotate_left_160() {
    meta.rotate_left_160_out = meta.rotate_left_160_x_out[15:0] ++ meta.rotate_left_160_x_out[31:16];
    meta.rotate_left_159_shl_out = meta.rotate_left_159_x_out << 7;
    meta.rotate_left_159_shr_out = meta.rotate_left_159_x_out >> 25;
    @in_hash { meta.rotate_left_158_out = meta.rotate_left_158_x_out[18:0] ++ meta.rotate_left_158_x_out[31:19]; }
  }

  action compute_rotate_left_159_or() {
    meta.rotate_left_159_or_out = meta.rotate_left_159_shl_out | meta.rotate_left_159_shr_out;
    meta.rotate_left_161_x_out = meta.rotate_left_161_x_out;
  }

  action compute_op_xor_428() {
    meta.op_xor_428_out = meta.rotate_left_159_or_out ^ meta.op_add_425_out;
    meta.rotate_left_162_x_out = meta.rotate_left_162_x_out;
    meta.rotate_left_161_shl_out = meta.rotate_left_161_x_out << 5;
    meta.rotate_left_161_shr_out = meta.rotate_left_161_x_out >> 27;
    meta.rotate_left_163_x_out = meta.rotate_left_163_x_out;
  }

  action compute_rotate_left_162() {
    meta.rotate_left_162_out = meta.rotate_left_162_x_out[23:0] ++ meta.rotate_left_162_x_out[31:24];
    meta.rotate_left_161_or_out = meta.rotate_left_161_shl_out | meta.rotate_left_161_shr_out;
    meta.rotate_left_163_out = meta.rotate_left_163_x_out[15:0] ++ meta.rotate_left_163_x_out[31:16];
    meta.op_add_429_out = meta.rotate_left_160_out + meta.op_xor_428_out;
  }

  action compute_rotate_left_164_x() {
    meta.rotate_left_164_x_out = meta.rotate_left_164_x_out;
    meta.rotate_left_165_x_out = meta.rotate_left_165_x_out;
  }

  action compute_rotate_left_164() {
    @in_hash { meta.rotate_left_164_out = meta.rotate_left_164_x_out[18:0] ++ meta.rotate_left_164_x_out[31:19]; }
    meta.rotate_left_165_shl_out = meta.rotate_left_165_x_out << 7;
    meta.rotate_left_165_shr_out = meta.rotate_left_165_x_out >> 25;
    meta.op_add_433_out = meta.rotate_left_163_out + meta.rotate_left_165_x_out;
    meta.rotate_left_166_x_out = meta.rotate_left_166_x_out;
  }

  action compute_rotate_left_165_or() {
    meta.rotate_left_165_or_out = meta.rotate_left_165_shl_out | meta.rotate_left_165_shr_out;
    meta.rotate_left_166_out = meta.rotate_left_166_x_out[15:0] ++ meta.rotate_left_166_x_out[31:16];
    meta.rotate_left_167_x_out = meta.rotate_left_167_x_out;
  }

  action compute_rotate_left_167_shl() {
    meta.rotate_left_167_shl_out = meta.rotate_left_167_x_out << 5;
    meta.rotate_left_167_shr_out = meta.rotate_left_167_x_out >> 27;
    meta.rotate_left_169_x_out = meta.rotate_left_169_x_out;
    meta.rotate_left_168_x_out = meta.rotate_left_168_x_out;
  }

  action compute_rotate_left_167_or() {
    meta.rotate_left_167_or_out = meta.rotate_left_167_shl_out | meta.rotate_left_167_shr_out;
    meta.rotate_left_169_out = meta.rotate_left_169_x_out[15:0] ++ meta.rotate_left_169_x_out[31:16];
    meta.rotate_left_168_out = meta.rotate_left_168_x_out[23:0] ++ meta.rotate_left_168_x_out[31:24];
    meta.op_add_437_out = meta.rotate_left_166_out + meta.rotate_left_168_x_out;
  }

  action compute_rotate_left_170_x() {
    meta.rotate_left_170_x_out = meta.rotate_left_170_x_out;
    meta.rotate_left_171_x_out = meta.rotate_left_171_x_out;
  }

  action compute_rotate_left_170() {
    @in_hash { meta.rotate_left_170_out = meta.rotate_left_170_x_out[18:0] ++ meta.rotate_left_170_x_out[31:19]; }
  }

  action compute_rotate_left_171() {
    @in_hash { meta.rotate_left_171_out = meta.rotate_left_171_x_out[24:0] ++ meta.rotate_left_171_x_out[31:25]; }
    meta.op_add_439_out = meta.op_add_437_out + meta.rotate_left_170_x_out;
    meta.rotate_left_172_x_out = meta.rotate_left_172_x_out;
    meta.op_add_441_out = meta.rotate_left_169_out + meta.rotate_left_171_x_out;
  }

  action compute_rotate_left_173_x() {
    meta.rotate_left_173_x_out = meta.rotate_left_173_x_out;
    meta.rotate_left_172_out = meta.rotate_left_172_x_out[15:0] ++ meta.rotate_left_172_x_out[31:16];
    meta.rotate_left_174_x_out = meta.rotate_left_174_x_out;
  }

  action compute_rotate_left_173_shl() {
    meta.rotate_left_173_shl_out = meta.rotate_left_173_x_out << 5;
    meta.rotate_left_173_shr_out = meta.rotate_left_173_x_out >> 27;
    meta.rotate_left_174_out = meta.rotate_left_174_x_out[23:0] ++ meta.rotate_left_174_x_out[31:24];
    meta.op_add_443_out = meta.op_add_441_out + meta.rotate_left_173_x_out;
    meta.rotate_left_175_x_out = meta.rotate_left_175_x_out;
    meta.op_add_445_out = meta.rotate_left_172_out + meta.rotate_left_174_x_out;
  }

  action compute_rotate_left_173_or() {
    meta.rotate_left_173_or_out = meta.rotate_left_173_shl_out | meta.rotate_left_173_shr_out;
    meta.rotate_left_175_out = meta.rotate_left_175_x_out[15:0] ++ meta.rotate_left_175_x_out[31:16];
    meta.rotate_left_177_x_out = meta.rotate_left_177_x_out;
  }

  action compute_rotate_left_176_x() {
    meta.rotate_left_176_x_out = meta.rotate_left_176_x_out;
    meta.rotate_left_177_shl_out = meta.rotate_left_177_x_out << 7;
    meta.rotate_left_177_shr_out = meta.rotate_left_177_x_out >> 25;
    meta.op_add_451_out = meta.rotate_left_175_out + meta.rotate_left_177_x_out;
  }

  action compute_rotate_left_176() {
    @in_hash { meta.rotate_left_176_out = meta.rotate_left_176_x_out[18:0] ++ meta.rotate_left_176_x_out[31:19]; }
    meta.rotate_left_177_or_out = meta.rotate_left_177_shl_out | meta.rotate_left_177_shr_out;
    meta.op_add_452_out = meta.op_add_445_out + meta.rotate_left_176_x_out;
    meta.rotate_left_178_x_out = meta.rotate_left_178_x_out;
  }

  action compute_rotate_left_178() {
    meta.rotate_left_178_out = meta.rotate_left_178_x_out[15:0] ++ meta.rotate_left_178_x_out[31:16];
    meta.op_xor_456_out = meta.rotate_left_177_or_out ^ meta.op_add_451_out;
    meta.op_xor_453_out = meta.rotate_left_176_out ^ meta.op_add_452_out;
  }

  action compute_op_xor_454() {
    meta.op_xor_454_out = meta.op_add_451_out ^ meta.op_xor_453_out;
  }

  action compute_op_xor_455() {
    meta.op_xor_455_out = meta.op_xor_454_out ^ meta.rotate_left_178_out;
  }

  action compute_op_xor_457() {
    meta.op_xor_457_out = hdr.recirc.op_xor_455_out ^ hdr.recirc.op_xor_456_out;
  }

  action swap_action_180() {
    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
  }
  action swap_action_181() {
    swap(hdr.hdr1.data4[23:16], hdr.hdr1.data6[23:16]);
    swap(hdr.hdr1.data4[15:8], hdr.hdr1.data6[15:8]);
    swap(hdr.hdr1.data4[7:0], hdr.hdr1.data6[7:0]);
    swap(hdr.hdr1.data5, hdr.hdr1.data7);
  }
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_144902;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_144902;

  action bf_1073926928_hash_0_144902_calc_144902() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_144902.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_144902_calc_144902() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_144902.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0x2681580b
    });
  }
  action compute_op_lshr_458() {
    meta.op_lshr_458_out = meta.time;
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  31646:RotateLeft
        // BDD node 133:rotate_left
        compute_rotate_left_133_x();
        compute_rotate_left_133();
        compute_rotate_left_134_or();
        compute_rotate_left_135();
        compute_rotate_left_137();
        compute_rotate_left_138();
        compute_op_xor_397();
        compute_rotate_left_141();
        compute_op_xor_399();
        compute_rotate_left_144_x();
        compute_rotate_left_144();
        compute_rotate_left_146();
        compute_rotate_left_147();
        compute_rotate_left_149();
        compute_rotate_left_151();
        compute_rotate_left_152();
        compute_rotate_left_153();
        compute_rotate_left_155();
        compute_rotate_left_156();
        compute_rotate_left_157();
        // EP node  32277:RotateLeft
        // BDD node 132:rotate_left
        // EP node  32810:ArithmeticOp
        // BDD node 385:op_xor
        // EP node  33560:RotateLeftShifts
        // BDD node 134:rotate_left
        // EP node  33997:ArithmeticOp
        // BDD node 386:op_add
        // EP node  34545:ArithmeticOp
        // BDD node 387:op_xor
        // EP node  35206:RotateLeft
        // BDD node 135:rotate_left
        // EP node  36091:RotateLeft
        // BDD node 136:rotate_left
        // EP node  36544:ArithmeticOp
        // BDD node 388:op_add
        // EP node  37112:ArithmeticOp
        // BDD node 389:op_xor
        // EP node  37685:ArithmeticOp
        // BDD node 390:op_add
        // EP node  38376:RotateLeft
        // BDD node 137:rotate_left
        // EP node  39301:RotateLeft
        // BDD node 138:rotate_left
        // EP node  39774:ArithmeticOp
        // BDD node 393:op_xor
        // EP node  40251:ArithmeticOp
        // BDD node 391:op_xor
        // EP node  40849:RotateLeft
        // BDD node 139:rotate_left
        // EP node  41334:ArithmeticOp
        // BDD node 392:op_add
        // EP node  41942:ArithmeticOp
        // BDD node 394:op_add
        // EP node  42675:ArithmeticOp
        // BDD node 397:op_xor
        // EP node  43414:RotateLeft
        // BDD node 140:rotate_left
        // EP node  44403:RotateLeft
        // BDD node 141:rotate_left
        // EP node  45154:ArithmeticOp
        // BDD node 395:op_xor
        // EP node  45787:RotateLeft
        // BDD node 142:rotate_left
        // EP node  46300:ArithmeticOp
        // BDD node 398:op_add
        // EP node  46943:ArithmeticOp
        // BDD node 399:op_xor
        // EP node  47718:ArithmeticOp
        // BDD node 400:op_xor
        // EP node  48371:ArithmeticOp
        // BDD node 396:op_add
        // EP node  49158:RotateLeft
        // BDD node 143:rotate_left
        // EP node  50211:ArithmeticOp
        // BDD node 401:op_xor
        // EP node  51010:RotateLeft
        // BDD node 144:rotate_left
        // EP node  51683:RotateLeft
        // BDD node 145:rotate_left
        // EP node  52228:ArithmeticOp
        // BDD node 402:op_add
        // EP node  52911:RotateLeft
        // BDD node 146:rotate_left
        // EP node  53734:ArithmeticOp
        // BDD node 403:op_xor
        // EP node  54291:ArithmeticOp
        // BDD node 404:op_add
        // EP node  54989:ArithmeticOp
        // BDD node 405:op_xor
        // EP node  55830:RotateLeft
        // BDD node 147:rotate_left
        // EP node  56955:RotateLeft
        // BDD node 148:rotate_left
        // EP node  57528:ArithmeticOp
        // BDD node 407:op_xor
        // EP node  58105:ArithmeticOp
        // BDD node 406:op_add
        // EP node  58828:RotateLeft
        // BDD node 149:rotate_left
        // EP node  59699:ArithmeticOp
        // BDD node 409:op_xor
        // EP node  60144:ArithmeticOp
        // BDD node 408:op_add
        // EP node  61027:RotateLeft
        // BDD node 151:rotate_left
        // EP node  61770:RotateLeft
        // BDD node 150:rotate_left
        // EP node  62371:ArithmeticOp
        // BDD node 410:op_add
        // EP node  63124:RotateLeft
        // BDD node 152:rotate_left
        // EP node  64031:ArithmeticOp
        // BDD node 413:op_xor
        // EP node  64494:ArithmeticOp
        // BDD node 411:op_xor
        // EP node  64960:ArithmeticOp
        // BDD node 412:op_add
        // EP node  65885:RotateLeft
        // BDD node 153:rotate_left
        // EP node  67122:ArithmeticOp
        // BDD node 415:op_xor
        // EP node  67905:RotateLeft
        // BDD node 154:rotate_left
        // EP node  68538:ArithmeticOp
        // BDD node 414:op_add
        // EP node  69331:RotateLeft
        // BDD node 155:rotate_left
        // EP node  70286:ArithmeticOp
        // BDD node 418:op_xor
        // EP node  70773:ArithmeticOp
        // BDD node 416:op_add
        // EP node  71581:RotateLeft
        // BDD node 156:rotate_left
        // EP node  72234:ArithmeticOp
        // BDD node 417:op_xor
        // EP node  73052:RotateLeft
        // BDD node 157:rotate_left
        // EP node  73713:ArithmeticOp
        // BDD node 419:op_add
        // EP node  75206:ArithmeticOp
        // BDD node 420:op_xor
        // EP node  76039:ArithmeticOp
        // BDD node 422:op_xor
        // EP node  77385:ArithmeticOp
        // BDD node 421:op_add
        // EP node  80936:ArithmeticOp
        // BDD node 424:op_xor
        // EP node  81950:Recirculate
        // BDD node 160:rotate_left
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(1);
        hdr.recirc.rotate_left_155_out = meta.rotate_left_155_out;
        hdr.recirc.rotate_left_156_out = meta.rotate_left_156_out;
        hdr.recirc.rotate_left_157_out = meta.rotate_left_157_out;
        hdr.recirc.rotate_left_157_x_out = meta.rotate_left_157_x_out;
        hdr.recirc.op_xor_422_out = meta.op_xor_422_out;
        hdr.recirc.op_add_421_out = meta.op_add_421_out;
        hdr.recirc.op_xor_424_out = meta.op_xor_424_out;
        if (hdr.recirc.code_path == 1) {
          // EP node  83660:RotateLeft
          // BDD node 160:rotate_left
          compute_rotate_left_160_x();
          compute_rotate_left_160();
          compute_rotate_left_159_or();
          compute_op_xor_428();
          compute_rotate_left_162();
          compute_rotate_left_164_x();
          compute_rotate_left_164();
          compute_rotate_left_165_or();
          compute_rotate_left_167_shl();
          compute_rotate_left_167_or();
          compute_rotate_left_170_x();
          compute_rotate_left_170();
          compute_rotate_left_171();
          compute_rotate_left_173_x();
          compute_rotate_left_173_shl();
          compute_rotate_left_173_or();
          compute_rotate_left_176_x();
          compute_rotate_left_176();
          compute_rotate_left_178();
          compute_op_xor_454();
          compute_op_xor_455();
          // EP node  85197:RotateLeftShifts
          // BDD node 159:rotate_left
          // EP node  86060:RotateLeft
          // BDD node 158:rotate_left
          // EP node  87099:ArithmeticOp
          // BDD node 425:op_add
          // EP node  87972:ArithmeticOp
          // BDD node 428:op_xor
          // EP node  88850:ArithmeticOp
          // BDD node 423:op_add
          // EP node  89907:RotateLeft
          // BDD node 162:rotate_left
          // EP node  91147:RotateLeftShifts
          // BDD node 161:rotate_left
          // EP node  91864:ArithmeticOp
          // BDD node 426:op_xor
          // EP node  92762:RotateLeft
          // BDD node 163:rotate_left
          // EP node  93487:ArithmeticOp
          // BDD node 427:op_add
          // EP node  94395:RotateLeft
          // BDD node 164:rotate_left
          // EP node  95488:ArithmeticOp
          // BDD node 429:op_add
          // EP node  96770:RotateLeftShifts
          // BDD node 165:rotate_left
          // EP node  97511:ArithmeticOp
          // BDD node 432:op_xor
          // EP node  98256:ArithmeticOp
          // BDD node 433:op_add
          // EP node  99189:ArithmeticOp
          // BDD node 430:op_xor
          // EP node  100312:RotateLeft
          // BDD node 166:rotate_left
          // EP node  101255:ArithmeticOp
          // BDD node 431:op_add
          // EP node  102954:RotateLeftShifts
          // BDD node 167:rotate_left
          // EP node  103907:ArithmeticOp
          // BDD node 434:op_xor
          // EP node  105054:RotateLeft
          // BDD node 169:rotate_left
          // EP node  106017:RotateLeft
          // BDD node 168:rotate_left
          // EP node  106794:ArithmeticOp
          // BDD node 436:op_xor
          // EP node  107575:ArithmeticOp
          // BDD node 435:op_add
          // EP node  108553:ArithmeticOp
          // BDD node 437:op_add
          // EP node  109730:RotateLeft
          // BDD node 170:rotate_left
          // EP node  111303:RotateLeft
          // BDD node 171:rotate_left
          // EP node  112492:ArithmeticOp
          // BDD node 440:op_xor
          // EP node  113293:ArithmeticOp
          // BDD node 438:op_xor
          // EP node  114296:ArithmeticOp
          // BDD node 439:op_add
          // EP node  116103:RotateLeftShifts
          // BDD node 173:rotate_left
          // EP node  117116:RotateLeft
          // BDD node 172:rotate_left
          // EP node  117933:ArithmeticOp
          // BDD node 441:op_add
          // EP node  118956:RotateLeft
          // BDD node 174:rotate_left
          // EP node  119781:ArithmeticOp
          // BDD node 442:op_xor
          // EP node  120814:ArithmeticOp
          // BDD node 443:op_add
          // EP node  122057:RotateLeft
          // BDD node 175:rotate_left
          // EP node  123100:RotateLeft
          // BDD node 176:rotate_left
          // EP node  124355:ArithmeticOp
          // BDD node 444:op_xor
          // EP node  125200:ArithmeticOp
          // BDD node 445:op_add
          // EP node  126678:RotateLeftShifts
          // BDD node 177:rotate_left
          // EP node  127531:ArithmeticOp
          // BDD node 450:op_xor
          // EP node  128388:ArithmeticOp
          // BDD node 451:op_add
          // EP node  129249:ArithmeticOp
          // BDD node 446:op_xor
          // EP node  130327:ArithmeticOp
          // BDD node 452:op_add
          // EP node  131410:RotateLeft
          // BDD node 178:rotate_left
          // EP node  132283:Ignore
          // BDD node 179:nf_set_rte_ipv4_udptcp_checksum
          // EP node  133375:ArithmeticOp
          // BDD node 456:op_xor
          // EP node  134039:ArithmeticOp
          // BDD node 453:op_xor
          // EP node  134706:ArithmeticOp
          // BDD node 454:op_xor
          // EP node  135376:ArithmeticOp
          // BDD node 455:op_xor
          // EP node  135828:Recirculate
          // BDD node 457:op_xor
          fwd_op = fwd_op_t.RECIRCULATE;
          build_recirc_hdr(2);
          hdr.recirc.op_xor_456_out = meta.op_xor_456_out;
          hdr.recirc.op_xor_455_out = meta.op_xor_455_out;
          if (hdr.recirc.code_path == 2) {
            // EP node  136502:ArithmeticOp
            // BDD node 457:op_xor
            compute_op_xor_457();
            // EP node  137181:ModifyHeader
            // BDD node 180:packet_return_chunk
            swap_action_180();
            meta.hdr_val0 = (hdr.recirc.op_lshr_449_out) ^ (meta.op_xor_457_out);
            meta.hdr_val1 = (32w0x00000001) + ((hdr.hdr2.data2 ++ hdr.hdr2.data3));
            meta.hdr_val2 = ((bit<32>)(hdr.hdr2.data5[7:0])) | (32w0x00000012);
            hdr.hdr2.data2[23:16] = meta.hdr_val0[31:24];
            hdr.hdr2.data4[23:16] = meta.hdr_val1[31:24];
            hdr.hdr2.data5[15:8] = 8w0x50;
            hdr.hdr2.data2[15:8] = meta.hdr_val0[23:16];
            hdr.hdr2.data4[15:8] = meta.hdr_val1[23:16];
            hdr.hdr2.data5[7:0] = meta.hdr_val2[7:0];
            hdr.hdr2.data2[7:0] = meta.hdr_val0[15:8];
            hdr.hdr2.data4[7:0] = meta.hdr_val1[15:8];
            hdr.hdr2.data3 = meta.hdr_val0[7:0];
            hdr.hdr2.data5[23:16] = meta.hdr_val1[7:0];
            // EP node  137863:ModifyHeader
            // BDD node 181:packet_return_chunk
            swap_action_181();
            hdr.hdr1.data0 = 8w0x45;
            hdr.hdr1.data1[15:8] = 8w0x00;
            hdr.hdr1.data1[7:0] = 8w0x28;
            // EP node  139233:Forward
            // BDD node 183:FORWARD
            nf_dev[15:0] = meta.dev[15:0];
          }
        }
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
          // EP node  158517:Forward
          // BDD node 9:FORWARD
          nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[23:16]);
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
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data5[7:0])) & (32w0x00000002))){
                // EP node  324:Then
                // BDD node 13:if
                // EP node  559:RotateLeft
                // BDD node 16:rotate_left
                compute_rotate_left_16();
                compute_rotate_left_17();
                // EP node  1052:RotateLeft
                // BDD node 17:rotate_left
                // EP node  1314:RotateLeft
                // BDD node 18:rotate_left
                // EP node  1539:BloomFilterQuery
                // BDD node 14:bf_query
                meta.key_24b_0 = hdr.hdr1.data4;
                meta.key_8b_1 = hdr.hdr1.data5;
                meta.key_24b_2 = hdr.hdr1.data6;
                meta.key_8b_3 = hdr.hdr1.data7;
                meta.key_16b_4 = hdr.hdr2.data0;
                meta.key_16b_5 = hdr.hdr2.data1;
                bf_1073926928_hash_0_1539_calc_1539();
                bf_1073926928_hash_1_1539_calc_1539();
                meta.bf_1073926928_estimate = 0;
                bf_1073926928_row_0_read_execute();
                bf_1073926928_row_1_read_execute();
                // EP node  1822:If
                // BDD node 15:if
                if ((32w0x00000000) == (meta.bf_1073926928_estimate)){
                  // EP node  1823:Then
                  // BDD node 15:if
                  // EP node  141271:SendToController
                  // BDD node 88:vector_borrow
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(141271);
                  hdr.cpu.bf_1073926928_estimate = meta.bf_1073926928_estimate;
                  hdr.cpu.rotate_left_18_out = meta.rotate_left_18_out;
                  hdr.cpu.rotate_left_17_out = meta.rotate_left_17_out;
                  hdr.cpu.rotate_left_16_out = meta.rotate_left_16_out;
                } else {
                  // EP node  1824:Else
                  // BDD node 15:if
                  // EP node  2206:Forward
                  // BDD node 103:FORWARD
                  nf_dev[15:0] = 16w0x0000;
                }
              } else {
                // EP node  325:Else
                // BDD node 13:if
                // EP node  2680:RotateLeftShifts
                // BDD node 107:rotate_left
                compute_rotate_left_107_shl();
                compute_rotate_left_107_or();
                compute_rotate_left_110_x();
                compute_rotate_left_110();
                // EP node  2989:ArithmeticOp
                // BDD node 380:op_shl
                // EP node  3308:RotateLeft
                // BDD node 108:rotate_left
                // EP node  3605:ArithmeticOp
                // BDD node 346:op_xor
                // EP node  3911:RotateLeft
                // BDD node 110:rotate_left
                // EP node  4396:If
                // BDD node 104:if
                if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data5[7:0])) & (32w0x00000010))){
                  // EP node  4397:Then
                  // BDD node 104:if
                  // EP node  4892:VectorTableLookup
                  // BDD node 105:vector_borrow
                  meta.key_32b_0 = 32w0x00000000;
                  vector_table_1073939504_105.apply();
                  // EP node  5246:RotateLeft
                  // BDD node 109:rotate_left
                  compute_rotate_left_109();
                  compute_op_add_347();
                  compute_rotate_left_111_x();
                  compute_rotate_left_111();
                  compute_rotate_left_112_x();
                  compute_op_sub_448();
                  compute_rotate_left_113();
                  compute_rotate_left_115();
                  compute_rotate_left_116();
                  compute_rotate_left_117();
                  compute_rotate_left_118();
                  compute_rotate_left_119();
                  compute_rotate_left_121();
                  compute_rotate_left_122();
                  compute_rotate_left_123();
                  compute_rotate_left_126_x();
                  compute_rotate_left_126();
                  compute_rotate_left_128();
                  compute_rotate_left_129();
                  compute_rotate_left_131();
                  compute_op_add_384();
                  // EP node  5493:Ignore
                  // BDD node 106:vector_return
                  // EP node  5785:ArithmeticOp
                  // BDD node 347:op_add
                  // EP node  6085:RotateLeft
                  // BDD node 111:rotate_left
                  // EP node  6518:ArithmeticOp
                  // BDD node 447:op_lshr
                  // EP node  6789:ArithmeticOp
                  // BDD node 381:op_or
                  // EP node  7022:ArithmeticOp
                  // BDD node 351:op_xor
                  // EP node  7260:ArithmeticOp
                  // BDD node 448:op_sub
                  // EP node  7503:ArithmeticOp
                  // BDD node 348:op_xor
                  // EP node  7751:ArithmeticOp
                  // BDD node 349:op_add
                  // EP node  8052:RotateLeft
                  // BDD node 112:rotate_left
                  // EP node  8310:ArithmeticOp
                  // BDD node 350:op_add
                  // EP node  8623:ArithmeticOp
                  // BDD node 353:op_xor
                  // EP node  8891:RotateLeft
                  // BDD node 113:rotate_left
                  // EP node  9216:ArithmeticOp
                  // BDD node 352:op_add
                  // EP node  9600:RotateLeft
                  // BDD node 114:rotate_left
                  // EP node  9937:ArithmeticOp
                  // BDD node 355:op_xor
                  // EP node  10280:ArithmeticOp
                  // BDD node 354:op_add
                  // EP node  10685:RotateLeft
                  // BDD node 115:rotate_left
                  // EP node  11040:ArithmeticOp
                  // BDD node 356:op_add
                  // EP node  11459:RotateLeft
                  // BDD node 116:rotate_left
                  // EP node  12062:ArithmeticOp
                  // BDD node 357:op_xor
                  // EP node  12495:RotateLeft
                  // BDD node 117:rotate_left
                  // EP node  13118:ArithmeticOp
                  // BDD node 449:op_lshr
                  // EP node  13441:ArithmeticOp
                  // BDD node 358:op_add
                  // EP node  13832:RotateLeft
                  // BDD node 118:rotate_left
                  // EP node  14165:RotateLeft
                  // BDD node 119:rotate_left
                  // EP node  14568:ArithmeticOp
                  // BDD node 359:op_xor
                  // EP node  14845:ArithmeticOp
                  // BDD node 360:op_add
                  // EP node  15193:ArithmeticOp
                  // BDD node 362:op_xor
                  // EP node  15478:ArithmeticOp
                  // BDD node 363:op_xor
                  // EP node  15836:ArithmeticOp
                  // BDD node 361:op_xor
                  // EP node  16269:RotateLeft
                  // BDD node 121:rotate_left
                  // EP node  16637:ArithmeticOp
                  // BDD node 365:op_xor
                  // EP node  17010:RotateLeft
                  // BDD node 120:rotate_left
                  // EP node  17315:ArithmeticOp
                  // BDD node 364:op_add
                  // EP node  17698:ArithmeticOp
                  // BDD node 366:op_add
                  // EP node  18161:RotateLeft
                  // BDD node 122:rotate_left
                  // EP node  18782:RotateLeft
                  // BDD node 123:rotate_left
                  // EP node  19257:ArithmeticOp
                  // BDD node 369:op_xor
                  // EP node  19582:ArithmeticOp
                  // BDD node 370:op_add
                  // EP node  19990:RotateLeft
                  // BDD node 126:rotate_left
                  // EP node  20323:ArithmeticOp
                  // BDD node 367:op_xor
                  // EP node  20741:RotateLeft
                  // BDD node 124:rotate_left
                  // EP node  21082:ArithmeticOp
                  // BDD node 368:op_add
                  // EP node  21510:RotateLeft
                  // BDD node 125:rotate_left
                  // EP node  22027:ArithmeticOp
                  // BDD node 371:op_xor
                  // EP node  22465:RotateLeft
                  // BDD node 127:rotate_left
                  // EP node  22822:ArithmeticOp
                  // BDD node 373:op_xor
                  // EP node  23183:ArithmeticOp
                  // BDD node 372:op_add
                  // EP node  23636:RotateLeft
                  // BDD node 128:rotate_left
                  // EP node  24183:ArithmeticOp
                  // BDD node 375:op_xor
                  // EP node  24466:ArithmeticOp
                  // BDD node 374:op_add
                  // EP node  25025:RotateLeft
                  // BDD node 129:rotate_left
                  // EP node  25774:RotateLeft
                  // BDD node 130:rotate_left
                  // EP node  26159:ArithmeticOp
                  // BDD node 376:op_add
                  // EP node  26642:RotateLeft
                  // BDD node 131:rotate_left
                  // EP node  27225:ArithmeticOp
                  // BDD node 383:op_xor
                  // EP node  27526:ArithmeticOp
                  // BDD node 377:op_xor
                  // EP node  27830:ArithmeticOp
                  // BDD node 378:op_add
                  // EP node  28235:ArithmeticOp
                  // BDD node 382:op_xor
                  // EP node  29053:ArithmeticOp
                  // BDD node 384:op_add
                  // EP node  30079:ArithmeticOp
                  // BDD node 379:op_xor
                  // EP node  31013:Recirculate
                  // BDD node 133:rotate_left
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(0);
                  hdr.recirc.op_or_381_out = meta.op_or_381_out;
                  hdr.recirc.op_lshr_449_out = meta.op_lshr_449_out;
                  hdr.recirc.rotate_left_130_out = meta.rotate_left_130_out;
                  hdr.recirc.rotate_left_131_out = meta.rotate_left_131_out;
                  hdr.recirc.rotate_left_131_x_out = meta.rotate_left_131_x_out;
                  hdr.recirc.op_xor_382_out = meta.op_xor_382_out;
                  hdr.recirc.op_add_384_out = meta.op_add_384_out;
                  hdr.recirc.op_xor_379_out = meta.op_xor_379_out;
                } else {
                  // EP node  4398:Else
                  // BDD node 104:if
                  // EP node  161568:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  230:Else
              // BDD node 12:if
              // EP node  141747:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data5[7:0])) & (32w0x00000040))){
                // EP node  141748:Then
                // BDD node 188:if
                // EP node  144659:Forward
                // BDD node 192:FORWARD
                nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[23:16]);
              } else {
                // EP node  141749:Else
                // BDD node 188:if
                // EP node  144902:BloomFilterSet
                // BDD node 193:bf_set
                meta.key_24b_0 = hdr.hdr1.data4;
                meta.key_8b_1 = hdr.hdr1.data5;
                meta.key_24b_2 = hdr.hdr1.data6;
                meta.key_8b_3 = hdr.hdr1.data7;
                meta.key_16b_4 = hdr.hdr2.data0;
                meta.key_16b_5 = hdr.hdr2.data1;
                bf_1073926928_hash_0_144902_calc_144902();
                bf_1073926928_hash_1_144902_calc_144902();
                bf_1073926928_row_0_set_to_one_execute();
                bf_1073926928_row_1_set_to_one_execute();
                // EP node  148083:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  105:Else
          // BDD node 10:if
          // EP node  159019:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  149546:ParserCondition
          // BDD node 201:if
          // EP node  149547:Then
          // BDD node 201:if
          // EP node  153266:ArithmeticOp
          // BDD node 458:op_lshr
          compute_op_lshr_458();
          // EP node  156767:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  160289:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev[15:0])){
              // EP node  160290:Then
              // BDD node 203:if
              // EP node  162082:ParserCondition
              // BDD node 204:if
              // EP node  162083:Then
              // BDD node 204:if
              // EP node  163646:ParserCondition
              // BDD node 205:if
              // EP node  163647:Then
              // BDD node 205:if
              // EP node  174265:Forward
              // BDD node 209:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[23:16]);
              // EP node  163648:Else
              // BDD node 205:if
              // EP node  166810:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  169985:Ignore
                // BDD node 211:vector_borrow
                // EP node  173179:SendToController
                // BDD node 212:vector_return
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(173179);
                hdr.cpu.op_lshr_458_out = meta.op_lshr_458_out;
              }
              // EP node  162084:Else
              // BDD node 204:if
              // EP node  173721:Forward
              // BDD node 221:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[23:16]);
            } else {
              // EP node  160291:Else
              // BDD node 203:if
              // EP node  171843:Forward
              // BDD node 225:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[23:16]);
            }
          }
          // EP node  149548:Else
          // BDD node 201:if
          // EP node  161056:Forward
          // BDD node 228:FORWARD
          nf_dev[15:0] = (bit<16>)(hdr.hdr1.data6[23:16]);
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  152523:ParserReject
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
