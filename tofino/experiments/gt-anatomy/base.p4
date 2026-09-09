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
  bit<32> rotate_left_27_out;
  bit<32> op_add_285_out;
  bit<32> op_or_266_out;
  bit<32> op_lshr_333_out;
  bit<32> bf_1073926928_estimate;
  bit<32> rotate_left_31_out;
  bit<32> rotate_left_29_out;
  bit<32> rotate_left_31_x_out;
  bit<32> rotate_left_30_out;
  bit<32> op_xor_246_out;
  bit<32> op_add_336_out;
  bit<32> op_lshr_458_out;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> f32_0;
  bit<32> f32_1;
  bit<32> f32_2;
  bit<32> f32_3;
  bit<32> f32_4;
  bit<32> f32_5;
  bit<32> f32_6;
  bit<32> f32_7;


};

header egress_state_h {
  bit<32> dev;
  bit<32> rotate_left_108_x_out;
  bit<32> op_shl_380_a_out;
  bit<32> op_or_381_b_out;
  bit<32> op_or_381_out;
  bit<32> rotate_left_120_x_out;
  bit<32> op_lshr_449_out;
  bit<32> rotate_left_131_out;
  bit<32> rotate_left_132_x_out;
  bit<32> rotate_left_132_out;
  bit<32> rotate_left_133_out;
  bit<32> rotate_left_133_x_out;
  bit<32> op_add_386_out;
  bit<32> op_xor_387_out;
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
  bit<96> data0;
  bit<16> data1;
}
header hdr1_h {
  bit<32> data0;
  bit<40> data1;
  bit<24> data2;
  bit<32> data3;
  bit<32> data4;
}
header hdr2_h {
  bit<32> data0;
  bit<32> data1;
  bit<24> data2;
  bit<24> data3;
  bit<48> data4;
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
  egress_state_h egress_state;

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
  bit<32> bf_1073926928_estimate;
  bit<24> key_24b_0;
  bit<8> key_8b_1;
  bit<24> key_24b_2;
  bit<8> key_8b_3;
  bit<16> key_16b_4;
  bit<16> key_16b_5;
  bit<32> rotate_left_17_x_out;
  bit<32> rotate_left_17_out;
  bit<32> rotate_left_18_out;
  bit<32> rotate_left_19_x_out;
  bit<32> rotate_left_19_out;
  bit<32> op_add_232_out;
  bit<32> op_add_234_out;
  bit<32> rotate_left_20_x_out;
  bit<32> rotate_left_20_out;
  bit<32> op_shl_265_a_out;
  bit<32> op_shl_265_out;
  bit<32> op_add_235_out;
  bit<32> rotate_left_22_x_out;
  bit<32> rotate_left_22_out;
  bit<32> rotate_left_21_x_out;
  bit<32> rotate_left_21_out;
  bit<32> op_add_237_out;
  bit<32> rotate_left_23_x_out;
  bit<32> rotate_left_23_out;
  bit<32> rotate_left_24_x_out;
  bit<32> rotate_left_24_out;
  bit<32> op_add_285_out;
  bit<32> rotate_left_25_x_out;
  bit<32> rotate_left_25_out;
  bit<32> op_add_241_out;
  bit<32> op_or_266_b_out;
  bit<32> op_or_266_out;
  bit<32> rotate_left_26_x_out;
  bit<32> rotate_left_26_out;
  bit<32> op_add_243_out;
  bit<32> rotate_left_27_x_out;
  bit<32> rotate_left_27_out;
  bit<32> op_xor_248_out;
  bit<32> rotate_left_28_x_out;
  bit<32> rotate_left_28_out;
  bit<32> op_add_336_out;
  bit<32> op_add_245_out;
  bit<32> op_xor_246_out;
  bit<32> op_xor_247_out;
  bit<32> rotate_left_29_x_out;
  bit<32> rotate_left_29_out;
  bit<32> rotate_left_30_x_out;
  bit<32> rotate_left_30_out;
  bit<32> rotate_left_31_x_out;
  bit<32> rotate_left_31_out;
  bit<32> op_lshr_333_out;
  bit<32> rotate_left_107_out;
  bit<32> rotate_left_108_x_out;
  bit<32> rotate_left_108_out;
  bit<32> rotate_left_109_out;
  bit<32> op_shl_380_a_out;
  bit<32> op_shl_380_out;
  bit<32> op_xor_348_out;
  bit<32> key_32b_0;
  bit<32> rotate_left_110_x_out;
  bit<32> rotate_left_110_out;
  bit<32> op_or_381_b_out;
  bit<32> op_or_381_out;
  bit<32> op_add_347_out;
  bit<32> op_xor_351_out;
  bit<32> rotate_left_111_x_out;
  bit<32> rotate_left_111_out;
  bit<32> op_add_352_out;
  bit<32> op_lshr_447_out;
  bit<32> op_add_349_out;
  bit<32> rotate_left_114_x_out;
  bit<32> rotate_left_114_out;
  bit<32> rotate_left_112_x_out;
  bit<32> rotate_left_112_out;
  bit<32> op_sub_448_out;
  bit<32> rotate_left_113_x_out;
  bit<32> rotate_left_113_out;
  bit<32> rotate_left_115_x_out;
  bit<32> rotate_left_115_out;
  bit<32> op_add_356_out;
  bit<32> op_xor_357_out;
  bit<32> rotate_left_118_x_out;
  bit<32> rotate_left_118_out;
  bit<32> op_xor_359_out;
  bit<32> rotate_left_116_x_out;
  bit<32> rotate_left_116_out;
  bit<32> rotate_left_117_x_out;
  bit<32> rotate_left_117_out;
  bit<32> rotate_left_119_x_out;
  bit<32> rotate_left_119_out;
  bit<32> op_add_360_out;
  bit<32> op_xor_361_out;
  bit<32> op_xor_362_out;
  bit<32> op_xor_365_out;
  bit<32> rotate_left_120_x_out;
  bit<32> rotate_left_120_out;
  bit<32> op_lshr_449_out;
  bit<32> op_add_366_out;
  bit<32> rotate_left_121_x_out;
  bit<32> rotate_left_121_out;
  bit<32> op_xor_367_out;
  bit<32> rotate_left_123_x_out;
  bit<32> rotate_left_123_out;
  bit<32> rotate_left_122_x_out;
  bit<32> rotate_left_122_out;
  bit<32> rotate_left_124_x_out;
  bit<32> rotate_left_124_out;
  bit<32> op_add_370_out;
  bit<32> rotate_left_126_x_out;
  bit<32> rotate_left_126_out;
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
  bit<32> op_add_378_out;
  bit<32> rotate_left_131_x_out;
  bit<32> rotate_left_131_out;
  bit<32> op_xor_379_out;
  bit<32> rotate_left_132_x_out;
  bit<32> rotate_left_132_out;
  bit<32> op_xor_382_out;
  bit<32> rotate_left_133_x_out;
  bit<32> rotate_left_133_out;
  bit<32> op_add_386_out;
  bit<32> op_xor_387_out;
  bit<32> rotate_left_160_x_out;
  bit<32> rotate_left_160_out;
  bit<32> rotate_left_161_x_out;
  bit<32> rotate_left_161_out;
  bit<32> rotate_left_162_x_out;
  bit<32> rotate_left_162_out;
  bit<32> rotate_left_163_x_out;
  bit<32> rotate_left_163_out;
  bit<32> rotate_left_164_x_out;
  bit<32> rotate_left_164_shl_out;
  bit<32> rotate_left_164_shr_out;
  bit<32> rotate_left_164_or_out;
  bit<32> op_add_429_out;
  bit<32> rotate_left_166_x_out;
  bit<32> rotate_left_166_out;
  bit<32> op_xor_432_out;
  bit<32> rotate_left_165_x_out;
  bit<32> rotate_left_165_out;
  bit<32> op_xor_434_out;
  bit<32> rotate_left_167_x_out;
  bit<32> rotate_left_167_out;
  bit<32> op_add_433_out;
  bit<32> rotate_left_168_x_out;
  bit<32> rotate_left_168_out;
  bit<32> op_add_435_out;
  bit<32> rotate_left_169_x_out;
  bit<32> rotate_left_169_out;
  bit<32> rotate_left_170_x_out;
  bit<32> rotate_left_170_shl_out;
  bit<32> rotate_left_170_shr_out;
  bit<32> rotate_left_170_or_out;
  bit<32> op_add_437_out;
  bit<32> op_xor_440_out;
  bit<32> rotate_left_171_x_out;
  bit<32> rotate_left_171_shl_out;
  bit<32> rotate_left_171_shr_out;
  bit<32> rotate_left_171_or_out;
  bit<32> rotate_left_172_x_out;
  bit<32> rotate_left_172_out;
  bit<32> rotate_left_173_x_out;
  bit<32> rotate_left_173_shl_out;
  bit<32> rotate_left_173_shr_out;
  bit<32> rotate_left_173_or_out;
  bit<32> op_add_441_out;
  bit<32> rotate_left_174_x_out;
  bit<32> rotate_left_174_out;
  bit<32> op_add_445_out;
  bit<32> rotate_left_177_x_out;
  bit<32> rotate_left_177_out;
  bit<32> op_add_443_out;
  bit<32> op_xor_446_out;
  bit<32> rotate_left_178_x_out;
  bit<32> rotate_left_178_out;
  bit<32> rotate_left_175_x_out;
  bit<32> rotate_left_175_out;
  bit<32> rotate_left_176_x_out;
  bit<32> rotate_left_176_out;
  bit<32> op_add_451_out;
  bit<32> op_xor_456_out;
  bit<32> op_xor_453_out;
  bit<32> op_xor_454_out;
  bit<32> op_xor_455_out;
  bit<32> op_xor_457_out;
  bit<32> hdr_val0;
  bit<32> hdr_val1;
  bit<32> hdr_val2;
  bit<32> op_lshr_458_out;
  bit<1> to_egress;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  egress_state_h egress_state;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

}

struct synapse_egress_metadata_t {
  // The egress reads the clock itself rather than having it carried across the crossing: the
  // ingress keeps time as ingress_mac_tstamp[47:16], and the backend rewrites shifts of it to
  // match, a convention a value travelling in the state header would not carry with it.
  bit<32> time;
  bit<32> rotate_left_136_x_out;
  bit<32> rotate_left_136_out;
  bit<32> rotate_left_134_x_out;
  bit<32> rotate_left_134_out;
  bit<32> op_xor_389_out;
  bit<32> rotate_left_135_x_out;
  bit<32> rotate_left_135_out;
  bit<32> op_xor_391_out;
  bit<32> rotate_left_137_x_out;
  bit<32> rotate_left_137_out;
  bit<32> op_add_390_out;
  bit<32> op_xor_393_out;
  bit<32> rotate_left_138_x_out;
  bit<32> rotate_left_138_out;
  bit<32> rotate_left_139_x_out;
  bit<32> rotate_left_139_out;
  bit<32> rotate_left_140_x_out;
  bit<32> rotate_left_140_shl_out;
  bit<32> rotate_left_140_shr_out;
  bit<32> rotate_left_140_or_out;
  bit<32> op_add_394_out;
  bit<32> rotate_left_141_x_out;
  bit<32> rotate_left_141_out;
  bit<32> op_add_396_out;
  bit<32> rotate_left_142_x_out;
  bit<32> rotate_left_142_out;
  bit<32> rotate_left_143_x_out;
  bit<32> rotate_left_143_out;
  bit<32> op_add_398_out;
  bit<32> op_xor_400_out;
  bit<32> op_xor_399_out;
  bit<32> rotate_left_144_x_out;
  bit<32> rotate_left_144_out;
  bit<32> rotate_left_145_x_out;
  bit<32> rotate_left_145_out;
  bit<32> rotate_left_146_x_out;
  bit<32> rotate_left_146_shl_out;
  bit<32> rotate_left_146_shr_out;
  bit<32> rotate_left_146_or_out;
  bit<32> op_add_404_out;
  bit<32> rotate_left_147_x_out;
  bit<32> rotate_left_147_out;
  bit<32> op_add_406_out;
  bit<32> rotate_left_148_x_out;
  bit<32> rotate_left_148_out;
  bit<32> op_xor_409_out;
  bit<32> rotate_left_149_x_out;
  bit<32> rotate_left_149_out;
  bit<32> op_add_408_out;
  bit<32> rotate_left_151_x_out;
  bit<32> rotate_left_151_out;
  bit<32> op_xor_411_out;
  bit<32> op_add_412_out;
  bit<32> rotate_left_150_x_out;
  bit<32> rotate_left_150_out;
  bit<32> rotate_left_152_x_out;
  bit<32> rotate_left_152_out;
  bit<32> op_xor_415_out;
  bit<32> rotate_left_153_x_out;
  bit<32> rotate_left_153_out;
  bit<32> rotate_left_154_x_out;
  bit<32> rotate_left_154_out;
  bit<32> op_xor_418_out;
  bit<32> op_add_416_out;
  bit<32> op_xor_420_out;
  bit<32> rotate_left_155_x_out;
  bit<32> rotate_left_155_out;
  bit<32> rotate_left_156_x_out;
  bit<32> rotate_left_156_out;
  bit<32> op_xor_417_out;
  bit<32> rotate_left_157_x_out;
  bit<32> rotate_left_157_out;
  bit<32> op_xor_422_out;
  bit<32> rotate_left_158_x_out;
  bit<32> rotate_left_158_out;
  bit<32> op_add_421_out;
  bit<32> op_add_423_out;
  bit<32> rotate_left_159_x_out;
  bit<32> rotate_left_159_out;
  bit<32> op_add_425_out;

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
    transition select (hdr.hdr1.data2[23:16]) {
      8w0x11: parser_6;
      default: parser_201;
    }
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr1.data2[23:16]) {
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

  action compute_rotate_left_16() {
    meta.rotate_left_16_out = 32w2225785509;
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

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_1030;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_1030;

  action bf_1073926928_hash_0_1030_calc_1030() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_1030.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_1030_calc_1030() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_1030.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0x2681580b
    });
  }
  action compute_rotate_left_17_x() {
    meta.rotate_left_17_x_out = meta.rotate_left_17_x_out;
    meta.rotate_left_18_out = 32w3399118710;
    meta.op_shl_265_a_out = meta.op_shl_265_a_out;
    meta.op_add_285_out = 32w0xffffffff + hdr.hdr2.data1;
    meta.op_or_266_b_out = meta.op_or_266_b_out;
    meta.op_add_336_out = 32w0xffffffff + (hdr.hdr2.data2 ++ hdr.hdr2.data3[23:16]);
    meta.op_lshr_333_out = meta.time;
  }

  action compute_rotate_left_17() {
    meta.rotate_left_17_out = meta.rotate_left_17_x_out[23:0] ++ meta.rotate_left_17_x_out[31:24];
    meta.rotate_left_19_x_out = meta.rotate_left_19_x_out;
    meta.op_add_232_out = 32w0x5d476351 + meta.rotate_left_17_x_out;
    meta.op_shl_265_out = meta.op_shl_265_a_out << 32w0x00000010;
  }

  action compute_rotate_left_19() {
    @in_hash { meta.rotate_left_19_out = meta.rotate_left_19_x_out[18:0] ++ meta.rotate_left_19_x_out[31:19]; }
    meta.op_add_234_out = meta.rotate_left_17_x_out + meta.rotate_left_19_x_out;
    meta.rotate_left_20_x_out = meta.rotate_left_20_x_out;
    meta.op_or_266_out = meta.op_shl_265_out | meta.op_or_266_b_out;
  }

  action compute_rotate_left_20() {
    @in_hash { meta.rotate_left_20_out = meta.rotate_left_20_x_out[24:0] ++ meta.rotate_left_20_x_out[31:25]; }
    meta.op_add_235_out = 32w0x5d476351 + meta.op_add_234_out;
    meta.rotate_left_21_x_out = meta.rotate_left_21_x_out;
    meta.op_add_237_out = meta.rotate_left_18_out + meta.rotate_left_20_x_out;
  }

  action compute_rotate_left_22_x() {
    meta.rotate_left_22_x_out = meta.rotate_left_22_x_out;
    meta.rotate_left_21_out = meta.rotate_left_21_x_out[15:0] ++ meta.rotate_left_21_x_out[31:16];
    meta.rotate_left_23_x_out = meta.rotate_left_23_x_out;
  }

  action compute_rotate_left_22() {
    @in_hash { meta.rotate_left_22_out = meta.rotate_left_22_x_out[26:0] ++ meta.rotate_left_22_x_out[31:27]; }
    meta.rotate_left_23_out = meta.rotate_left_23_x_out[23:0] ++ meta.rotate_left_23_x_out[31:24];
    meta.rotate_left_24_x_out = meta.rotate_left_24_x_out;
    meta.op_add_241_out = meta.rotate_left_21_out + meta.rotate_left_23_x_out;
  }

  action compute_rotate_left_24() {
    meta.rotate_left_24_out = meta.rotate_left_24_x_out[15:0] ++ meta.rotate_left_24_x_out[31:16];
    meta.rotate_left_25_x_out = meta.rotate_left_25_x_out;
    meta.rotate_left_26_x_out = meta.rotate_left_26_x_out;
  }

  action compute_rotate_left_25() {
    @in_hash { meta.rotate_left_25_out = meta.rotate_left_25_x_out[18:0] ++ meta.rotate_left_25_x_out[31:19]; }
    meta.op_add_243_out = meta.op_add_241_out + meta.rotate_left_25_x_out;
    meta.rotate_left_27_x_out = meta.rotate_left_27_x_out;
    meta.op_add_245_out = meta.rotate_left_24_out + meta.rotate_left_26_x_out;
  }

  action compute_rotate_left_26() {
    @in_hash { meta.rotate_left_26_out = meta.rotate_left_26_x_out[24:0] ++ meta.rotate_left_26_x_out[31:25]; }
    meta.rotate_left_27_out = meta.rotate_left_27_x_out[15:0] ++ meta.rotate_left_27_x_out[31:16];
    meta.op_xor_248_out = meta.rotate_left_25_out ^ meta.op_add_243_out;
    meta.rotate_left_28_x_out = meta.rotate_left_28_x_out;
    meta.op_xor_247_out = meta.op_add_245_out ^ hdr.hdr1.data3;
  }

  action compute_rotate_left_28() {
    @in_hash { meta.rotate_left_28_out = meta.rotate_left_28_x_out[26:0] ++ meta.rotate_left_28_x_out[31:27]; }
    meta.op_xor_246_out = meta.rotate_left_26_out ^ meta.op_add_245_out;
    meta.rotate_left_30_x_out = meta.rotate_left_30_x_out;
  }

  action compute_rotate_left_29_x() {
    meta.rotate_left_29_x_out = meta.rotate_left_29_x_out;
    meta.rotate_left_30_out = meta.rotate_left_30_x_out[15:0] ++ meta.rotate_left_30_x_out[31:16];
    meta.rotate_left_31_x_out = meta.rotate_left_31_x_out;
  }

  action compute_rotate_left_29() {
    meta.rotate_left_29_out = meta.rotate_left_29_x_out[23:0] ++ meta.rotate_left_29_x_out[31:24];
    @in_hash { meta.rotate_left_31_out = meta.rotate_left_31_x_out[18:0] ++ meta.rotate_left_31_x_out[31:19]; }
  }

  action compute_rotate_left_107() {
    meta.rotate_left_107_out = 32w2225785509;
    meta.rotate_left_108_x_out = meta.rotate_left_108_x_out;
    meta.rotate_left_109_out = 32w3399118710;
  }

  action compute_rotate_left_108() {
    meta.rotate_left_108_out = meta.rotate_left_108_x_out[23:0] ++ meta.rotate_left_108_x_out[31:24];
  }

  action compute_op_xor_346() {
    meta.op_shl_380_a_out = meta.op_shl_380_a_out;
  }

  action compute_op_shl_380() {
    meta.op_shl_380_out = meta.op_shl_380_a_out << 32w0x00000010;
    meta.op_xor_348_out = 32w0x6f76ca9a ^ meta.rotate_left_107_out;
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

  action compute_op_or_381_b() {
    meta.op_or_381_b_out = meta.op_or_381_b_out;
    meta.op_lshr_447_out = meta.time;
  }

  action compute_rotate_left_110_x() {
    meta.rotate_left_110_x_out = meta.rotate_left_110_x_out;
    meta.op_add_347_out = 32w0x5d476351 + meta.rotate_left_108_x_out;
  }

  action compute_rotate_left_110() {
    @in_hash { meta.rotate_left_110_out = meta.rotate_left_110_x_out[18:0] ++ meta.rotate_left_110_x_out[31:19]; }
    meta.op_or_381_out = meta.op_shl_380_out | meta.op_or_381_b_out;
    meta.op_xor_351_out = meta.rotate_left_108_out ^ meta.op_add_347_out;
    meta.rotate_left_111_x_out = meta.rotate_left_111_x_out;
    meta.op_add_349_out = meta.rotate_left_108_x_out + meta.op_xor_348_out;
  }

  action compute_rotate_left_111() {
    @in_hash { meta.rotate_left_111_out = meta.rotate_left_111_x_out[24:0] ++ meta.rotate_left_111_x_out[31:25]; }
    meta.op_add_352_out = meta.rotate_left_109_out + meta.op_xor_351_out;
    meta.rotate_left_112_x_out = meta.rotate_left_112_x_out;
    meta.op_sub_448_out = meta.op_lshr_447_out[31:0] - vector_table_1073939504_105_get_value_param0;
  }

  action compute_rotate_left_114_x() {
    meta.rotate_left_114_x_out = meta.rotate_left_114_x_out;
    meta.rotate_left_112_out = meta.rotate_left_112_x_out[15:0] ++ meta.rotate_left_112_x_out[31:16];
    meta.rotate_left_113_x_out = meta.rotate_left_113_x_out;
    meta.op_lshr_449_out = meta.op_sub_448_out >> 32w0x0000000c;
  }

  action compute_rotate_left_114() {
    meta.rotate_left_114_out = meta.rotate_left_114_x_out[23:0] ++ meta.rotate_left_114_x_out[31:24];
    @in_hash { meta.rotate_left_113_out = meta.rotate_left_113_x_out[26:0] ++ meta.rotate_left_113_x_out[31:27]; }
    meta.rotate_left_115_x_out = meta.rotate_left_115_x_out;
    meta.op_add_356_out = meta.rotate_left_112_out + meta.rotate_left_114_x_out;
  }

  action compute_rotate_left_115() {
    meta.rotate_left_115_out = meta.rotate_left_115_x_out[15:0] ++ meta.rotate_left_115_x_out[31:16];
    meta.op_xor_357_out = meta.rotate_left_113_out ^ meta.rotate_left_115_x_out;
    meta.op_xor_359_out = meta.rotate_left_114_out ^ meta.op_add_356_out;
    meta.rotate_left_116_x_out = meta.rotate_left_116_x_out;
    meta.rotate_left_117_x_out = meta.rotate_left_117_x_out;
  }

  action compute_rotate_left_118_x() {
    meta.rotate_left_118_x_out = meta.rotate_left_118_x_out;
    @in_hash { meta.rotate_left_116_out = meta.rotate_left_116_x_out[18:0] ++ meta.rotate_left_116_x_out[31:19]; }
  }

  action compute_rotate_left_117() {
    @in_hash { meta.rotate_left_117_out = meta.rotate_left_117_x_out[24:0] ++ meta.rotate_left_117_x_out[31:25]; }
    meta.op_add_360_out = meta.rotate_left_115_out + meta.op_xor_359_out;
  }

  action compute_rotate_left_118() {
    meta.rotate_left_118_out = meta.rotate_left_118_x_out[15:0] ++ meta.rotate_left_118_x_out[31:16];
    meta.rotate_left_119_x_out = meta.rotate_left_119_x_out;
    meta.op_xor_361_out = meta.rotate_left_117_out ^ meta.op_add_360_out;
    meta.op_xor_362_out = meta.op_add_360_out ^ hdr.hdr1.data3;
  }

  action compute_rotate_left_119() {
    @in_hash { meta.rotate_left_119_out = meta.rotate_left_119_x_out[26:0] ++ meta.rotate_left_119_x_out[31:27]; }
    meta.op_xor_365_out = meta.op_xor_361_out ^ hdr.hdr1.data4;
    meta.rotate_left_120_x_out = meta.rotate_left_120_x_out;
    meta.rotate_left_121_x_out = meta.rotate_left_121_x_out;
  }

  action compute_rotate_left_120() {
    meta.rotate_left_120_out = meta.rotate_left_120_x_out[23:0] ++ meta.rotate_left_120_x_out[31:24];
    meta.op_add_366_out = meta.rotate_left_118_out + meta.op_xor_365_out;
    meta.rotate_left_121_out = meta.rotate_left_121_x_out[15:0] ++ meta.rotate_left_121_x_out[31:16];
    meta.op_xor_367_out = meta.rotate_left_119_out ^ meta.rotate_left_121_x_out;
    meta.rotate_left_122_x_out = meta.rotate_left_122_x_out;
  }

  action compute_rotate_left_123_x() {
    meta.rotate_left_123_x_out = meta.rotate_left_123_x_out;
    @in_hash { meta.rotate_left_122_out = meta.rotate_left_122_x_out[18:0] ++ meta.rotate_left_122_x_out[31:19]; }
    meta.rotate_left_124_x_out = meta.rotate_left_124_x_out;
  }

  action compute_rotate_left_123() {
    @in_hash { meta.rotate_left_123_out = meta.rotate_left_123_x_out[24:0] ++ meta.rotate_left_123_x_out[31:25]; }
    meta.rotate_left_124_out = meta.rotate_left_124_x_out[15:0] ++ meta.rotate_left_124_x_out[31:16];
    meta.op_add_370_out = meta.rotate_left_121_out + meta.rotate_left_123_x_out;
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
    meta.op_add_378_out = meta.rotate_left_127_out + meta.rotate_left_129_x_out;
    meta.rotate_left_131_x_out = meta.rotate_left_131_x_out;
  }

  action compute_rotate_left_131() {
    @in_hash { meta.rotate_left_131_out = meta.rotate_left_131_x_out[26:0] ++ meta.rotate_left_131_x_out[31:27]; }
    meta.op_xor_379_out = meta.rotate_left_129_out ^ meta.op_add_378_out;
    meta.op_xor_382_out = meta.op_add_378_out ^ hdr.hdr1.data4;
  }

  action compute_rotate_left_132_x() {
    meta.rotate_left_132_x_out = meta.rotate_left_132_x_out;
    meta.rotate_left_133_x_out = meta.rotate_left_133_x_out;
  }

  action compute_rotate_left_132() {
    meta.rotate_left_132_out = meta.rotate_left_132_x_out[23:0] ++ meta.rotate_left_132_x_out[31:24];
    meta.rotate_left_133_out = meta.rotate_left_133_x_out[15:0] ++ meta.rotate_left_133_x_out[31:16];
    meta.op_add_386_out = meta.rotate_left_130_out + meta.rotate_left_132_x_out;
    meta.op_xor_387_out = meta.rotate_left_131_out ^ meta.rotate_left_133_x_out;
  }

  action compute_rotate_left_160_x() {
    meta.rotate_left_160_x_out = meta.rotate_left_160_x_out;
    meta.rotate_left_161_x_out = meta.rotate_left_161_x_out;
    meta.rotate_left_162_x_out = meta.rotate_left_162_x_out;
  }

  action compute_rotate_left_160() {
    meta.rotate_left_160_out = meta.rotate_left_160_x_out[15:0] ++ meta.rotate_left_160_x_out[31:16];
    @in_hash { meta.rotate_left_161_out = meta.rotate_left_161_x_out[26:0] ++ meta.rotate_left_161_x_out[31:27]; }
    meta.rotate_left_162_out = meta.rotate_left_162_x_out[23:0] ++ meta.rotate_left_162_x_out[31:24];
    meta.rotate_left_163_x_out = meta.rotate_left_163_x_out;
  }

  action compute_rotate_left_163() {
    meta.rotate_left_163_out = meta.rotate_left_163_x_out[15:0] ++ meta.rotate_left_163_x_out[31:16];
    meta.rotate_left_164_x_out = meta.rotate_left_164_x_out;
    meta.op_add_429_out = meta.rotate_left_160_out + meta.rotate_left_162_x_out;
  }

  action compute_rotate_left_164_shl() {
    meta.rotate_left_164_shl_out = meta.rotate_left_164_x_out << 13;
    meta.rotate_left_164_shr_out = meta.rotate_left_164_x_out >> 19;
    meta.rotate_left_166_x_out = meta.rotate_left_166_x_out;
    meta.op_xor_432_out = meta.rotate_left_162_out ^ meta.op_add_429_out;
    meta.rotate_left_165_x_out = meta.rotate_left_165_x_out;
  }

  action compute_rotate_left_164_or() {
    meta.rotate_left_164_or_out = meta.rotate_left_164_shl_out | meta.rotate_left_164_shr_out;
    meta.rotate_left_166_out = meta.rotate_left_166_x_out[15:0] ++ meta.rotate_left_166_x_out[31:16];
    @in_hash { meta.rotate_left_165_out = meta.rotate_left_165_x_out[24:0] ++ meta.rotate_left_165_x_out[31:25]; }
    meta.op_add_433_out = meta.rotate_left_163_out + meta.op_xor_432_out;
  }

  action compute_op_xor_434() {
    meta.op_xor_434_out = meta.rotate_left_164_or_out ^ meta.rotate_left_166_x_out;
    meta.rotate_left_167_x_out = meta.rotate_left_167_x_out;
    meta.rotate_left_168_x_out = meta.rotate_left_168_x_out;
  }

  action compute_rotate_left_167() {
    @in_hash { meta.rotate_left_167_out = meta.rotate_left_167_x_out[26:0] ++ meta.rotate_left_167_x_out[31:27]; }
    meta.rotate_left_168_out = meta.rotate_left_168_x_out[23:0] ++ meta.rotate_left_168_x_out[31:24];
    meta.op_add_435_out = meta.op_add_433_out + meta.op_xor_434_out;
    meta.rotate_left_169_x_out = meta.rotate_left_169_x_out;
    meta.op_add_437_out = meta.rotate_left_166_out + meta.rotate_left_168_x_out;
  }

  action compute_rotate_left_169() {
    meta.rotate_left_169_out = meta.rotate_left_169_x_out[15:0] ++ meta.rotate_left_169_x_out[31:16];
    meta.rotate_left_170_x_out = meta.rotate_left_170_x_out;
    meta.op_xor_440_out = meta.rotate_left_168_out ^ meta.op_add_437_out;
    meta.rotate_left_171_x_out = meta.rotate_left_171_x_out;
  }

  action compute_rotate_left_170_shl() {
    meta.rotate_left_170_shl_out = meta.rotate_left_170_x_out << 13;
    meta.rotate_left_170_shr_out = meta.rotate_left_170_x_out >> 19;
    meta.rotate_left_171_shl_out = meta.rotate_left_171_x_out << 7;
    meta.rotate_left_171_shr_out = meta.rotate_left_171_x_out >> 25;
    meta.rotate_left_172_x_out = meta.rotate_left_172_x_out;
    meta.op_add_441_out = meta.rotate_left_169_out + meta.op_xor_440_out;
  }

  action compute_rotate_left_170_or() {
    meta.rotate_left_170_or_out = meta.rotate_left_170_shl_out | meta.rotate_left_170_shr_out;
    meta.rotate_left_171_or_out = meta.rotate_left_171_shl_out | meta.rotate_left_171_shr_out;
    meta.rotate_left_172_out = meta.rotate_left_172_x_out[15:0] ++ meta.rotate_left_172_x_out[31:16];
  }

  action compute_rotate_left_173_x() {
    meta.rotate_left_173_x_out = meta.rotate_left_173_x_out;
    meta.rotate_left_174_x_out = meta.rotate_left_174_x_out;
  }

  action compute_rotate_left_173_shl() {
    meta.rotate_left_173_shl_out = meta.rotate_left_173_x_out << 5;
    meta.rotate_left_173_shr_out = meta.rotate_left_173_x_out >> 27;
    meta.rotate_left_174_out = meta.rotate_left_174_x_out[23:0] ++ meta.rotate_left_174_x_out[31:24];
    meta.op_add_445_out = meta.rotate_left_172_out + meta.rotate_left_174_x_out;
    meta.op_add_443_out = meta.op_add_441_out + meta.rotate_left_173_x_out;
    meta.rotate_left_175_x_out = meta.rotate_left_175_x_out;
  }

  action compute_rotate_left_173_or() {
    meta.rotate_left_173_or_out = meta.rotate_left_173_shl_out | meta.rotate_left_173_shr_out;
    meta.rotate_left_177_x_out = meta.rotate_left_177_x_out;
    meta.rotate_left_175_out = meta.rotate_left_175_x_out[15:0] ++ meta.rotate_left_175_x_out[31:16];
  }

  action compute_rotate_left_177() {
    @in_hash { meta.rotate_left_177_out = meta.rotate_left_177_x_out[24:0] ++ meta.rotate_left_177_x_out[31:25]; }
    meta.op_xor_446_out = meta.rotate_left_173_or_out ^ meta.op_add_443_out;
    meta.rotate_left_176_x_out = meta.rotate_left_176_x_out;
    meta.op_add_451_out = meta.rotate_left_175_out + meta.rotate_left_177_x_out;
  }

  action compute_rotate_left_178_x() {
    meta.rotate_left_178_x_out = meta.rotate_left_178_x_out;
    @in_hash { meta.rotate_left_176_out = meta.rotate_left_176_x_out[18:0] ++ meta.rotate_left_176_x_out[31:19]; }
    meta.op_xor_456_out = meta.rotate_left_177_out ^ meta.op_add_451_out;
  }

  action compute_rotate_left_178() {
    meta.rotate_left_178_out = meta.rotate_left_178_x_out[15:0] ++ meta.rotate_left_178_x_out[31:16];
    meta.op_xor_453_out = meta.rotate_left_176_out ^ meta.rotate_left_178_x_out;
  }

  action compute_op_xor_454() {
    meta.op_xor_454_out = meta.op_add_451_out ^ meta.op_xor_453_out;
  }

  action compute_op_xor_455() {
    meta.op_xor_455_out = meta.op_xor_454_out ^ meta.rotate_left_178_out;
  }

  action compute_op_xor_457() {
    meta.op_xor_457_out = meta.op_xor_455_out ^ meta.op_xor_456_out;
  }

  action swap_action_180() {
    hdr.hdr2.data0 = hdr.hdr2.data0[15:0] ++ hdr.hdr2.data0[31:16];
  }
  action swap_action_181() {
    swap32(hdr.hdr1.data3, hdr.hdr1.data4);
  }
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_241578;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_241578;

  action bf_1073926928_hash_0_241578_calc_241578() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_241578.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_241578_calc_241578() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_241578.get({
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
        // EP node  83141:RotateLeft
        // BDD node 160:rotate_left
        compute_rotate_left_160_x();
        compute_rotate_left_160();
        compute_rotate_left_163();
        compute_rotate_left_164_shl();
        compute_rotate_left_164_or();
        compute_op_xor_434();
        compute_rotate_left_167();
        compute_rotate_left_169();
        compute_rotate_left_170_shl();
        compute_rotate_left_170_or();
        compute_rotate_left_173_x();
        compute_rotate_left_173_shl();
        compute_rotate_left_173_or();
        compute_rotate_left_177();
        compute_rotate_left_178_x();
        compute_rotate_left_178();
        compute_op_xor_454();
        compute_op_xor_455();
        compute_op_xor_457();
        // EP node  84180:RotateLeft
        // BDD node 161:rotate_left
        // EP node  85569:ArithmeticOp
        // BDD node 426:op_xor
        // EP node  86620:RotateLeft
        // BDD node 162:rotate_left
        // EP node  87503:RotateLeft
        // BDD node 163:rotate_left
        // EP node  88216:ArithmeticOp
        // BDD node 427:op_add
        // EP node  89463:RotateLeftShifts
        // BDD node 164:rotate_left
        // EP node  90184:ArithmeticOp
        // BDD node 428:op_xor
        // EP node  91088:ArithmeticOp
        // BDD node 430:op_xor
        // EP node  91818:ArithmeticOp
        // BDD node 429:op_add
        // EP node  93092:RotateLeft
        // BDD node 166:rotate_left
        // EP node  94192:ArithmeticOp
        // BDD node 432:op_xor
        // EP node  95298:RotateLeft
        // BDD node 165:rotate_left
        // EP node  96593:ArithmeticOp
        // BDD node 431:op_add
        // EP node  97711:ArithmeticOp
        // BDD node 434:op_xor
        // EP node  98650:RotateLeft
        // BDD node 167:rotate_left
        // EP node  99594:ArithmeticOp
        // BDD node 433:op_add
        // EP node  100917:RotateLeft
        // BDD node 168:rotate_left
        // EP node  102059:ArithmeticOp
        // BDD node 435:op_add
        // EP node  103396:RotateLeft
        // BDD node 169:rotate_left
        // EP node  104550:ArithmeticOp
        // BDD node 436:op_xor
        // EP node  106094:RotateLeftShifts
        // BDD node 170:rotate_left
        // EP node  107068:ArithmeticOp
        // BDD node 437:op_add
        // EP node  108240:ArithmeticOp
        // BDD node 440:op_xor
        // EP node  109418:ArithmeticOp
        // BDD node 438:op_xor
        // EP node  111385:RotateLeftShifts
        // BDD node 171:rotate_left
        // EP node  112575:RotateLeft
        // BDD node 172:rotate_left
        // EP node  113574:ArithmeticOp
        // BDD node 439:op_add
        // EP node  115174:RotateLeftShifts
        // BDD node 173:rotate_left
        // EP node  116183:ArithmeticOp
        // BDD node 442:op_xor
        // EP node  116997:ArithmeticOp
        // BDD node 441:op_add
        // EP node  118418:RotateLeft
        // BDD node 174:rotate_left
        // EP node  119644:ArithmeticOp
        // BDD node 444:op_xor
        // EP node  120876:ArithmeticOp
        // BDD node 445:op_add
        // EP node  122318:RotateLeft
        // BDD node 177:rotate_left
        // EP node  124177:ArithmeticOp
        // BDD node 450:op_xor
        // EP node  125221:ArithmeticOp
        // BDD node 443:op_add
        // EP node  126477:ArithmeticOp
        // BDD node 446:op_xor
        // EP node  127947:RotateLeft
        // BDD node 178:rotate_left
        // EP node  129215:RotateLeft
        // BDD node 175:rotate_left
        // EP node  130489:RotateLeft
        // BDD node 176:rotate_left
        // EP node  132191:Ignore
        // BDD node 179:nf_set_rte_ipv4_udptcp_checksum
        // EP node  133476:ArithmeticOp
        // BDD node 451:op_add
        // EP node  134555:ArithmeticOp
        // BDD node 456:op_xor
        // EP node  135425:ArithmeticOp
        // BDD node 452:op_add
        // EP node  136299:ArithmeticOp
        // BDD node 453:op_xor
        // EP node  137177:ArithmeticOp
        // BDD node 454:op_xor
        // EP node  138059:ArithmeticOp
        // BDD node 455:op_xor
        // EP node  138945:ArithmeticOp
        // BDD node 457:op_xor
        // EP node  139835:ModifyHeader
        // BDD node 180:packet_return_chunk
        swap_action_180();
        meta.hdr_val0 = (hdr.egress_state.op_lshr_449_out) ^ (meta.op_xor_457_out);
        meta.hdr_val1 = (32w0x00000001) + (hdr.hdr2.data1);
        meta.hdr_val2 = ((bit<32>)(hdr.hdr2.data3[7:0])) | (32w0x00000012);
        hdr.hdr2.data1 = meta.hdr_val0[31:24] ++ meta.hdr_val0[23:16] ++ meta.hdr_val0[15:8] ++ meta.hdr_val0[7:0];
        hdr.hdr2.data2 = meta.hdr_val1[31:24] ++ meta.hdr_val1[23:16] ++ meta.hdr_val1[15:8];
        hdr.hdr2.data3 = meta.hdr_val1[7:0] ++ 8w0x50 ++ meta.hdr_val2[7:0];
        // EP node  140729:ModifyHeader
        // BDD node 181:packet_return_chunk
        swap_action_181();
        hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;
        // EP node  142303:Forward
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
          // EP node  259208:Forward
          // BDD node 9:FORWARD
          nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
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
                // EP node  559:RotateLeft
                // BDD node 16:rotate_left
                compute_rotate_left_16();
                // EP node  1030:BloomFilterQuery
                // BDD node 14:bf_query
                meta.key_24b_0 = hdr.hdr1.data3[31:8];
                meta.key_8b_1 = hdr.hdr1.data3[7:0];
                meta.key_24b_2 = hdr.hdr1.data4[31:8];
                meta.key_8b_3 = hdr.hdr1.data4[7:0];
                meta.key_16b_4 = hdr.hdr2.data0[31:16];
                meta.key_16b_5 = hdr.hdr2.data0[15:0];
                bf_1073926928_hash_0_1030_calc_1030();
                bf_1073926928_hash_1_1030_calc_1030();
                meta.bf_1073926928_estimate = 0;
                bf_1073926928_row_0_read_execute();
                bf_1073926928_row_1_read_execute();
                // EP node  1361:If
                // BDD node 15:if
                if ((32w0x00000000) == (meta.bf_1073926928_estimate)){
                  // EP node  1362:Then
                  // BDD node 15:if
                  // EP node  144747:RotateLeft
                  // BDD node 17:rotate_left
                  compute_rotate_left_17_x();
                  compute_rotate_left_17();
                  compute_rotate_left_19();
                  compute_rotate_left_20();
                  compute_rotate_left_22_x();
                  compute_rotate_left_22();
                  compute_rotate_left_24();
                  compute_rotate_left_25();
                  compute_rotate_left_26();
                  compute_rotate_left_28();
                  compute_rotate_left_29_x();
                  compute_rotate_left_29();
                  // EP node  147213:RotateLeft
                  // BDD node 18:rotate_left
                  // EP node  149466:RotateLeft
                  // BDD node 19:rotate_left
                  // EP node  153079:ArithmeticOp
                  // BDD node 231:op_xor
                  // EP node  155126:ArithmeticOp
                  // BDD node 233:op_xor
                  // EP node  157182:ArithmeticOp
                  // BDD node 232:op_add
                  // EP node  159475:ArithmeticOp
                  // BDD node 234:op_add
                  // EP node  162007:RotateLeft
                  // BDD node 20:rotate_left
                  // EP node  166160:ArithmeticOp
                  // BDD node 265:op_shl
                  // EP node  168483:ArithmeticOp
                  // BDD node 235:op_add
                  // EP node  171048:RotateLeft
                  // BDD node 22:rotate_left
                  // EP node  175255:ArithmeticOp
                  // BDD node 236:op_xor
                  // EP node  177608:RotateLeft
                  // BDD node 21:rotate_left
                  // EP node  179736:ArithmeticOp
                  // BDD node 237:op_add
                  // EP node  182109:ArithmeticOp
                  // BDD node 238:op_xor
                  // EP node  184729:RotateLeft
                  // BDD node 23:rotate_left
                  // EP node  187122:RotateLeft
                  // BDD node 24:rotate_left
                  // EP node  189286:ArithmeticOp
                  // BDD node 240:op_xor
                  // EP node  191459:ArithmeticOp
                  // BDD node 285:op_add
                  // EP node  193400:ArithmeticOp
                  // BDD node 239:op_add
                  // EP node  195591:RotateLeft
                  // BDD node 25:rotate_left
                  // EP node  199006:ArithmeticOp
                  // BDD node 242:op_xor
                  // EP node  200727:ArithmeticOp
                  // BDD node 241:op_add
                  // EP node  203190:ArithmeticOp
                  // BDD node 266:op_or
                  // EP node  205417:RotateLeft
                  // BDD node 26:rotate_left
                  // EP node  208888:ArithmeticOp
                  // BDD node 243:op_add
                  // EP node  211133:RotateLeft
                  // BDD node 27:rotate_left
                  // EP node  213138:ArithmeticOp
                  // BDD node 248:op_xor
                  // EP node  214901:RotateLeft
                  // BDD node 28:rotate_left
                  // EP node  217939:ArithmeticOp
                  // BDD node 244:op_xor
                  // EP node  219464:ArithmeticOp
                  // BDD node 336:op_add
                  // EP node  221261:ArithmeticOp
                  // BDD node 245:op_add
                  // EP node  222798:ArithmeticOp
                  // BDD node 246:op_xor
                  // EP node  224596:ArithmeticOp
                  // BDD node 247:op_xor
                  // EP node  226657:RotateLeft
                  // BDD node 29:rotate_left
                  // EP node  228469:RotateLeft
                  // BDD node 30:rotate_left
                  // EP node  230030:ArithmeticOp
                  // BDD node 249:op_add
                  // EP node  231856:RotateLeft
                  // BDD node 31:rotate_left
                  // EP node  235002:ArithmeticOp
                  // BDD node 252:op_xor
                  // EP node  236320:ArithmeticOp
                  // BDD node 333:op_lshr
                  // EP node  237392:SendToController
                  // BDD node 88:vector_borrow
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(237392);
                  hdr.cpu.rotate_left_27_out = meta.rotate_left_27_out;
                  hdr.cpu.op_add_285_out = meta.op_add_285_out;
                  hdr.cpu.op_or_266_out = meta.op_or_266_out;
                  hdr.cpu.op_lshr_333_out = meta.op_lshr_333_out;
                  hdr.cpu.bf_1073926928_estimate = meta.bf_1073926928_estimate;
                  hdr.cpu.rotate_left_31_out = meta.rotate_left_31_out;
                  hdr.cpu.rotate_left_29_out = meta.rotate_left_29_out;
                  hdr.cpu.rotate_left_31_x_out = meta.rotate_left_31_x_out;
                  hdr.cpu.rotate_left_30_out = meta.rotate_left_30_out;
                  hdr.cpu.op_xor_246_out = meta.op_xor_246_out;
                  hdr.cpu.op_add_336_out = meta.op_add_336_out;
                } else {
                  // EP node  1363:Else
                  // BDD node 15:if
                  // EP node  1802:Forward
                  // BDD node 103:FORWARD
                  nf_dev[15:0] = 16w0x0000;
                }
              } else {
                // EP node  325:Else
                // BDD node 13:if
                // EP node  2048:RotateLeft
                // BDD node 107:rotate_left
                compute_rotate_left_107();
                compute_rotate_left_108();
                // EP node  2590:RotateLeft
                // BDD node 108:rotate_left
                // EP node  2890:RotateLeft
                // BDD node 109:rotate_left
                // EP node  3170:If
                // BDD node 104:if
                if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000010))){
                  // EP node  3171:Then
                  // BDD node 104:if
                  // EP node  3443:ArithmeticOp
                  // BDD node 346:op_xor
                  compute_op_xor_346();
                  compute_op_shl_380();
                  // EP node  3728:ArithmeticOp
                  // BDD node 380:op_shl
                  // EP node  4021:ArithmeticOp
                  // BDD node 348:op_xor
                  // EP node  4508:VectorTableLookup
                  // BDD node 105:vector_borrow
                  meta.key_32b_0 = 32w0x00000000;
                  vector_table_1073939504_105.apply();
                  // EP node  4816:Ignore
                  // BDD node 106:vector_return
                  // EP node  5171:RotateLeft
                  // BDD node 110:rotate_left
                  compute_op_or_381_b();
                  compute_rotate_left_110_x();
                  compute_rotate_left_110();
                  compute_rotate_left_111();
                  compute_rotate_left_114_x();
                  compute_rotate_left_114();
                  compute_rotate_left_115();
                  compute_rotate_left_118_x();
                  compute_rotate_left_117();
                  compute_rotate_left_118();
                  compute_rotate_left_119();
                  compute_rotate_left_120();
                  compute_rotate_left_123_x();
                  compute_rotate_left_123();
                  compute_rotate_left_126_x();
                  compute_rotate_left_126();
                  compute_rotate_left_128();
                  compute_rotate_left_129();
                  compute_rotate_left_131();
                  compute_rotate_left_132_x();
                  compute_rotate_left_132();
                  // EP node  5614:ArithmeticOp
                  // BDD node 381:op_or
                  // EP node  5868:ArithmeticOp
                  // BDD node 347:op_add
                  // EP node  6169:ArithmeticOp
                  // BDD node 351:op_xor
                  // EP node  6477:RotateLeft
                  // BDD node 111:rotate_left
                  // EP node  6878:ArithmeticOp
                  // BDD node 352:op_add
                  // EP node  7200:ArithmeticOp
                  // BDD node 447:op_lshr
                  // EP node  7529:ArithmeticOp
                  // BDD node 349:op_add
                  // EP node  7911:RotateLeft
                  // BDD node 114:rotate_left
                  // EP node  8254:RotateLeft
                  // BDD node 112:rotate_left
                  // EP node  8556:ArithmeticOp
                  // BDD node 448:op_sub
                  // EP node  8864:ArithmeticOp
                  // BDD node 350:op_add
                  // EP node  9228:RotateLeft
                  // BDD node 113:rotate_left
                  // EP node  9701:ArithmeticOp
                  // BDD node 353:op_xor
                  // EP node  10079:RotateLeft
                  // BDD node 115:rotate_left
                  // EP node  10411:ArithmeticOp
                  // BDD node 354:op_add
                  // EP node  10803:ArithmeticOp
                  // BDD node 355:op_xor
                  // EP node  11202:ArithmeticOp
                  // BDD node 356:op_add
                  // EP node  11664:ArithmeticOp
                  // BDD node 357:op_xor
                  // EP node  12191:RotateLeft
                  // BDD node 118:rotate_left
                  // EP node  12669:ArithmeticOp
                  // BDD node 359:op_xor
                  // EP node  13155:RotateLeft
                  // BDD node 116:rotate_left
                  // EP node  13829:RotateLeft
                  // BDD node 117:rotate_left
                  // EP node  14392:ArithmeticOp
                  // BDD node 358:op_add
                  // EP node  14840:RotateLeft
                  // BDD node 119:rotate_left
                  // EP node  15421:ArithmeticOp
                  // BDD node 360:op_add
                  // EP node  15883:ArithmeticOp
                  // BDD node 361:op_xor
                  // EP node  16417:ArithmeticOp
                  // BDD node 362:op_xor
                  // EP node  16893:ArithmeticOp
                  // BDD node 365:op_xor
                  // EP node  17376:RotateLeft
                  // BDD node 120:rotate_left
                  // EP node  17798:ArithmeticOp
                  // BDD node 449:op_lshr
                  // EP node  18157:ArithmeticOp
                  // BDD node 366:op_add
                  // EP node  18591:ArithmeticOp
                  // BDD node 363:op_xor
                  // EP node  19102:RotateLeft
                  // BDD node 121:rotate_left
                  // EP node  19548:ArithmeticOp
                  // BDD node 364:op_add
                  // EP node  20073:ArithmeticOp
                  // BDD node 367:op_xor
                  // EP node  20679:RotateLeft
                  // BDD node 123:rotate_left
                  // EP node  21518:RotateLeft
                  // BDD node 122:rotate_left
                  // EP node  22216:RotateLeft
                  // BDD node 124:rotate_left
                  // EP node  22615:ArithmeticOp
                  // BDD node 369:op_xor
                  // EP node  23019:ArithmeticOp
                  // BDD node 370:op_add
                  // EP node  23507:RotateLeft
                  // BDD node 126:rotate_left
                  // EP node  23921:ArithmeticOp
                  // BDD node 368:op_add
                  // EP node  24421:RotateLeft
                  // BDD node 125:rotate_left
                  // EP node  25009:ArithmeticOp
                  // BDD node 371:op_xor
                  // EP node  25521:RotateLeft
                  // BDD node 127:rotate_left
                  // EP node  25955:ArithmeticOp
                  // BDD node 372:op_add
                  // EP node  26479:RotateLeft
                  // BDD node 128:rotate_left
                  // EP node  27095:ArithmeticOp
                  // BDD node 373:op_xor
                  // EP node  27544:ArithmeticOp
                  // BDD node 375:op_xor
                  // EP node  27910:ArithmeticOp
                  // BDD node 374:op_add
                  // EP node  28547:RotateLeft
                  // BDD node 129:rotate_left
                  // EP node  29371:RotateLeft
                  // BDD node 130:rotate_left
                  // EP node  29840:ArithmeticOp
                  // BDD node 376:op_add
                  // EP node  30406:ArithmeticOp
                  // BDD node 377:op_xor
                  // EP node  30978:ArithmeticOp
                  // BDD node 378:op_add
                  // EP node  31650:RotateLeft
                  // BDD node 131:rotate_left
                  // EP node  32519:ArithmeticOp
                  // BDD node 379:op_xor
                  // EP node  33205:RotateLeft
                  // BDD node 132:rotate_left
                  // EP node  33801:ArithmeticOp
                  // BDD node 383:op_xor
                  // EP node  34305:ArithmeticOp
                  // BDD node 382:op_xor
                  // EP node  34913:RotateLeft
                  // BDD node 133:rotate_left
                  // EP node  35427:ArithmeticOp
                  // BDD node 384:op_add
                  // EP node  36566:ArithmeticOp
                  // BDD node 385:op_xor
                  // EP node  37192:ArithmeticOp
                  // BDD node 386:op_add
                  // EP node  39411:ArithmeticOp
                  // BDD node 387:op_xor
                  // EP node  41220:SendToEgress
                  // BDD node 136:rotate_left
                  meta.to_egress = 1;
                  hdr.egress_state.setValid();
                  hdr.egress_state.dev = meta.dev;
                  hdr.egress_state.rotate_left_108_x_out = meta.rotate_left_108_x_out;
                  hdr.egress_state.op_shl_380_a_out = meta.op_shl_380_a_out;
                  hdr.egress_state.op_or_381_b_out = meta.op_or_381_b_out;
                  hdr.egress_state.op_or_381_out = meta.op_or_381_out;
                  hdr.egress_state.rotate_left_120_x_out = meta.rotate_left_120_x_out;
                  hdr.egress_state.op_lshr_449_out = meta.op_lshr_449_out;
                  hdr.egress_state.rotate_left_131_out = meta.rotate_left_131_out;
                  hdr.egress_state.rotate_left_132_x_out = meta.rotate_left_132_x_out;
                  hdr.egress_state.rotate_left_132_out = meta.rotate_left_132_out;
                  hdr.egress_state.rotate_left_133_out = meta.rotate_left_133_out;
                  hdr.egress_state.rotate_left_133_x_out = meta.rotate_left_133_x_out;
                  hdr.egress_state.op_add_386_out = meta.op_add_386_out;
                  hdr.egress_state.op_xor_387_out = meta.op_xor_387_out;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(0);
                } else {
                  // EP node  3172:Else
                  // BDD node 104:if
                  // EP node  262992:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  230:Else
              // BDD node 12:if
              // EP node  237942:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000040))){
                // EP node  237943:Then
                // BDD node 188:if
                // EP node  241298:Forward
                // BDD node 192:FORWARD
                nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
              } else {
                // EP node  237944:Else
                // BDD node 188:if
                // EP node  241578:BloomFilterSet
                // BDD node 193:bf_set
                meta.key_24b_0 = hdr.hdr1.data3[31:8];
                meta.key_8b_1 = hdr.hdr1.data3[7:0];
                meta.key_24b_2 = hdr.hdr1.data4[31:8];
                meta.key_8b_3 = hdr.hdr1.data4[7:0];
                meta.key_16b_4 = hdr.hdr2.data0[31:16];
                meta.key_16b_5 = hdr.hdr2.data0[15:0];
                bf_1073926928_hash_0_241578_calc_241578();
                bf_1073926928_hash_1_241578_calc_241578();
                bf_1073926928_row_0_set_to_one_execute();
                bf_1073926928_row_1_set_to_one_execute();
                // EP node  246080:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  105:Else
          // BDD node 10:if
          // EP node  259784:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  247765:ParserCondition
          // BDD node 201:if
          // EP node  247766:Then
          // BDD node 201:if
          // EP node  252608:ArithmeticOp
          // BDD node 458:op_lshr
          compute_op_lshr_458();
          // EP node  257199:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  261528:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev[15:0])){
              // EP node  261529:Then
              // BDD node 203:if
              // EP node  263580:ParserCondition
              // BDD node 204:if
              // EP node  263581:Then
              // BDD node 204:if
              // EP node  265366:ParserCondition
              // BDD node 205:if
              // EP node  265367:Then
              // BDD node 205:if
              // EP node  277464:Forward
              // BDD node 209:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
              // EP node  265368:Else
              // BDD node 205:if
              // EP node  268974:Ignore
              // BDD node 211:vector_borrow
              // EP node  272593:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  276230:SendToController
                // BDD node 212:vector_return
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(276230);
                hdr.cpu.op_lshr_458_out = meta.op_lshr_458_out;
              }
              // EP node  263582:Else
              // BDD node 204:if
              // EP node  276846:Forward
              // BDD node 221:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
            } else {
              // EP node  261530:Else
              // BDD node 203:if
              // EP node  274709:Forward
              // BDD node 225:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
            }
          }
          // EP node  247767:Else
          // BDD node 201:if
          // EP node  262406:Forward
          // BDD node 228:FORWARD
          nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  251754:ParserReject
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
    pkt.extract(hdr.egress_state);
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

  action compute_rotate_left_136_x() {
    eg_md.rotate_left_136_x_out = eg_md.rotate_left_136_x_out;
    eg_md.rotate_left_134_x_out = eg_md.rotate_left_134_x_out;
    eg_md.op_xor_389_out = hdr.egress_state.rotate_left_132_out ^ hdr.egress_state.op_add_386_out;
    eg_md.rotate_left_135_x_out = eg_md.rotate_left_135_x_out;
  }

  action compute_rotate_left_136() {
    eg_md.rotate_left_136_out = eg_md.rotate_left_136_x_out[15:0] ++ eg_md.rotate_left_136_x_out[31:16];
    @in_hash { eg_md.rotate_left_134_out = eg_md.rotate_left_134_x_out[18:0] ++ eg_md.rotate_left_134_x_out[31:19]; }
  }

  action compute_rotate_left_135() {
    @in_hash { eg_md.rotate_left_135_out = eg_md.rotate_left_135_x_out[24:0] ++ eg_md.rotate_left_135_x_out[31:25]; }
    eg_md.op_add_390_out = hdr.egress_state.rotate_left_133_out + eg_md.op_xor_389_out;
  }

  action compute_op_xor_391() {
    eg_md.op_xor_391_out = eg_md.rotate_left_134_out ^ eg_md.rotate_left_136_x_out;
    eg_md.rotate_left_137_x_out = eg_md.rotate_left_137_x_out;
    eg_md.op_xor_393_out = eg_md.rotate_left_135_out ^ eg_md.op_add_390_out;
    eg_md.rotate_left_138_x_out = eg_md.rotate_left_138_x_out;
  }

  action compute_rotate_left_137() {
    @in_hash { eg_md.rotate_left_137_out = eg_md.rotate_left_137_x_out[26:0] ++ eg_md.rotate_left_137_x_out[31:27]; }
    eg_md.rotate_left_138_out = eg_md.rotate_left_138_x_out[23:0] ++ eg_md.rotate_left_138_x_out[31:24];
    eg_md.rotate_left_139_x_out = eg_md.rotate_left_139_x_out;
    eg_md.op_add_394_out = eg_md.rotate_left_136_out + eg_md.op_xor_393_out;
  }

  action compute_rotate_left_139() {
    eg_md.rotate_left_139_out = eg_md.rotate_left_139_x_out[15:0] ++ eg_md.rotate_left_139_x_out[31:16];
    eg_md.rotate_left_140_x_out = eg_md.rotate_left_140_x_out;
    eg_md.rotate_left_141_x_out = eg_md.rotate_left_141_x_out;
  }

  action compute_rotate_left_140_shl() {
    eg_md.rotate_left_140_shl_out = eg_md.rotate_left_140_x_out << 13;
    eg_md.rotate_left_140_shr_out = eg_md.rotate_left_140_x_out >> 19;
    @in_hash { eg_md.rotate_left_141_out = eg_md.rotate_left_141_x_out[24:0] ++ eg_md.rotate_left_141_x_out[31:25]; }
    eg_md.op_add_396_out = eg_md.op_add_394_out + eg_md.rotate_left_140_x_out;
    eg_md.rotate_left_142_x_out = eg_md.rotate_left_142_x_out;
    eg_md.op_add_398_out = eg_md.rotate_left_139_out + eg_md.rotate_left_141_x_out;
  }

  action compute_rotate_left_140_or() {
    eg_md.rotate_left_140_or_out = eg_md.rotate_left_140_shl_out | eg_md.rotate_left_140_shr_out;
    eg_md.rotate_left_142_out = eg_md.rotate_left_142_x_out[15:0] ++ eg_md.rotate_left_142_x_out[31:16];
    eg_md.op_xor_400_out = eg_md.op_add_398_out ^ hdr.egress_state.op_or_381_out;
    eg_md.op_xor_399_out = eg_md.rotate_left_141_out ^ eg_md.op_add_398_out;
  }

  action compute_rotate_left_143_x() {
    eg_md.rotate_left_143_x_out = eg_md.rotate_left_143_x_out;
    eg_md.rotate_left_144_x_out = eg_md.rotate_left_144_x_out;
  }

  action compute_rotate_left_143() {
    @in_hash { eg_md.rotate_left_143_out = eg_md.rotate_left_143_x_out[26:0] ++ eg_md.rotate_left_143_x_out[31:27]; }
    eg_md.rotate_left_144_out = eg_md.rotate_left_144_x_out[23:0] ++ eg_md.rotate_left_144_x_out[31:24];
    eg_md.rotate_left_145_x_out = eg_md.rotate_left_145_x_out;
    eg_md.op_add_404_out = eg_md.rotate_left_142_out + eg_md.rotate_left_144_x_out;
  }

  action compute_rotate_left_145() {
    eg_md.rotate_left_145_out = eg_md.rotate_left_145_x_out[15:0] ++ eg_md.rotate_left_145_x_out[31:16];
    eg_md.rotate_left_146_x_out = eg_md.rotate_left_146_x_out;
    eg_md.rotate_left_147_x_out = eg_md.rotate_left_147_x_out;
  }

  action compute_rotate_left_146_shl() {
    eg_md.rotate_left_146_shl_out = eg_md.rotate_left_146_x_out << 13;
    eg_md.rotate_left_146_shr_out = eg_md.rotate_left_146_x_out >> 19;
    @in_hash { eg_md.rotate_left_147_out = eg_md.rotate_left_147_x_out[24:0] ++ eg_md.rotate_left_147_x_out[31:25]; }
    eg_md.op_add_406_out = eg_md.op_add_404_out + eg_md.rotate_left_146_x_out;
    eg_md.rotate_left_148_x_out = eg_md.rotate_left_148_x_out;
    eg_md.op_add_408_out = eg_md.rotate_left_145_out + eg_md.rotate_left_147_x_out;
  }

  action compute_rotate_left_146_or() {
    eg_md.rotate_left_146_or_out = eg_md.rotate_left_146_shl_out | eg_md.rotate_left_146_shr_out;
    eg_md.rotate_left_148_out = eg_md.rotate_left_148_x_out[15:0] ++ eg_md.rotate_left_148_x_out[31:16];
    eg_md.op_xor_411_out = eg_md.rotate_left_147_out ^ eg_md.op_add_408_out;
    eg_md.rotate_left_150_x_out = eg_md.rotate_left_150_x_out;
  }

  action compute_op_xor_409() {
    eg_md.op_xor_409_out = eg_md.rotate_left_146_or_out ^ eg_md.op_add_406_out;
    eg_md.rotate_left_149_x_out = eg_md.rotate_left_149_x_out;
    eg_md.op_add_412_out = eg_md.rotate_left_148_out + eg_md.op_xor_411_out;
    eg_md.rotate_left_150_out = eg_md.rotate_left_150_x_out[23:0] ++ eg_md.rotate_left_150_x_out[31:24];
  }

  action compute_rotate_left_149() {
    @in_hash { eg_md.rotate_left_149_out = eg_md.rotate_left_149_x_out[26:0] ++ eg_md.rotate_left_149_x_out[31:27]; }
    eg_md.rotate_left_151_x_out = eg_md.rotate_left_151_x_out;
    eg_md.op_xor_415_out = eg_md.rotate_left_150_out ^ eg_md.op_add_412_out;
    eg_md.rotate_left_153_x_out = eg_md.rotate_left_153_x_out;
  }

  action compute_rotate_left_151() {
    eg_md.rotate_left_151_out = eg_md.rotate_left_151_x_out[15:0] ++ eg_md.rotate_left_151_x_out[31:16];
    eg_md.rotate_left_152_x_out = eg_md.rotate_left_152_x_out;
    @in_hash { eg_md.rotate_left_153_out = eg_md.rotate_left_153_x_out[24:0] ++ eg_md.rotate_left_153_x_out[31:25]; }
  }

  action compute_rotate_left_152() {
    @in_hash { eg_md.rotate_left_152_out = eg_md.rotate_left_152_x_out[18:0] ++ eg_md.rotate_left_152_x_out[31:19]; }
    eg_md.rotate_left_154_x_out = eg_md.rotate_left_154_x_out;
    eg_md.op_add_416_out = eg_md.rotate_left_151_out + eg_md.op_xor_415_out;
  }

  action compute_rotate_left_154() {
    eg_md.rotate_left_154_out = eg_md.rotate_left_154_x_out[15:0] ++ eg_md.rotate_left_154_x_out[31:16];
    eg_md.op_xor_418_out = eg_md.rotate_left_152_out ^ eg_md.rotate_left_154_x_out;
    eg_md.op_xor_420_out = eg_md.rotate_left_153_out ^ eg_md.op_add_416_out;
    eg_md.rotate_left_155_x_out = eg_md.rotate_left_155_x_out;
    eg_md.rotate_left_156_x_out = eg_md.rotate_left_156_x_out;
    eg_md.op_xor_417_out = eg_md.op_add_416_out ^ hdr.hdr2.data1;
  }

  action compute_rotate_left_155() {
    @in_hash { eg_md.rotate_left_155_out = eg_md.rotate_left_155_x_out[26:0] ++ eg_md.rotate_left_155_x_out[31:27]; }
    eg_md.rotate_left_156_out = eg_md.rotate_left_156_x_out[23:0] ++ eg_md.rotate_left_156_x_out[31:24];
    eg_md.rotate_left_157_x_out = eg_md.rotate_left_157_x_out;
    eg_md.op_add_421_out = eg_md.rotate_left_154_out + eg_md.op_xor_420_out;
  }

  action compute_rotate_left_157() {
    eg_md.rotate_left_157_out = eg_md.rotate_left_157_x_out[15:0] ++ eg_md.rotate_left_157_x_out[31:16];
    eg_md.op_xor_422_out = eg_md.rotate_left_155_out ^ eg_md.rotate_left_157_x_out;
    eg_md.rotate_left_158_x_out = eg_md.rotate_left_158_x_out;
    eg_md.rotate_left_159_x_out = eg_md.rotate_left_159_x_out;
  }

  action compute_rotate_left_158() {
    @in_hash { eg_md.rotate_left_158_out = eg_md.rotate_left_158_x_out[18:0] ++ eg_md.rotate_left_158_x_out[31:19]; }
    eg_md.op_add_423_out = eg_md.op_add_421_out + eg_md.op_xor_422_out;
  }

  action compute_rotate_left_159() {
    @in_hash { eg_md.rotate_left_159_out = eg_md.rotate_left_159_x_out[24:0] ++ eg_md.rotate_left_159_x_out[31:25]; }
    eg_md.op_add_425_out = eg_md.rotate_left_157_out + eg_md.rotate_left_159_x_out;
  }


  apply {
    eg_md.time = eg_intr_md_from_prsr.global_tstamp[47:16];
    // EP node  42512:RotateLeft
    // BDD node 136:rotate_left
    compute_rotate_left_136_x();
    compute_rotate_left_136();
    compute_rotate_left_135();
    compute_op_xor_391();
    compute_rotate_left_137();
    compute_rotate_left_139();
    compute_rotate_left_140_shl();
    compute_rotate_left_140_or();
    compute_rotate_left_143_x();
    compute_rotate_left_143();
    compute_rotate_left_145();
    compute_rotate_left_146_shl();
    compute_rotate_left_146_or();
    compute_op_xor_409();
    compute_rotate_left_149();
    compute_rotate_left_151();
    compute_rotate_left_152();
    compute_rotate_left_154();
    compute_rotate_left_155();
    compute_rotate_left_157();
    compute_rotate_left_158();
    compute_rotate_left_159();
    // EP node  43044:RotateLeft
    // BDD node 134:rotate_left
    // EP node  43795:ArithmeticOp
    // BDD node 388:op_add
    // EP node  44337:ArithmeticOp
    // BDD node 389:op_xor
    // EP node  44884:RotateLeft
    // BDD node 135:rotate_left
    // EP node  45656:ArithmeticOp
    // BDD node 391:op_xor
    // EP node  45991:RotateLeft
    // BDD node 137:rotate_left
    // EP node  46329:ArithmeticOp
    // BDD node 390:op_add
    // EP node  46896:ArithmeticOp
    // BDD node 393:op_xor
    // EP node  47468:RotateLeft
    // BDD node 138:rotate_left
    // EP node  47930:RotateLeft
    // BDD node 139:rotate_left
    // EP node  48280:ArithmeticOp
    // BDD node 392:op_add
    // EP node  48986:RotateLeftShifts
    // BDD node 140:rotate_left
    // EP node  49342:ArithmeticOp
    // BDD node 394:op_add
    // EP node  49820:RotateLeft
    // BDD node 141:rotate_left
    // EP node  50422:ArithmeticOp
    // BDD node 395:op_xor
    // EP node  50908:ArithmeticOp
    // BDD node 396:op_add
    // EP node  51520:RotateLeft
    // BDD node 142:rotate_left
    // EP node  52014:RotateLeft
    // BDD node 143:rotate_left
    // EP node  52636:ArithmeticOp
    // BDD node 401:op_xor
    // EP node  52888:ArithmeticOp
    // BDD node 397:op_xor
    // EP node  53142:ArithmeticOp
    // BDD node 398:op_add
    // EP node  53525:ArithmeticOp
    // BDD node 400:op_xor
    // EP node  54039:ArithmeticOp
    // BDD node 399:op_xor
    // EP node  54686:RotateLeft
    // BDD node 144:rotate_left
    // EP node  55208:RotateLeft
    // BDD node 145:rotate_left
    // EP node  55603:ArithmeticOp
    // BDD node 403:op_xor
    // EP node  56001:ArithmeticOp
    // BDD node 402:op_add
    // EP node  56803:RotateLeftShifts
    // BDD node 146:rotate_left
    // EP node  57207:ArithmeticOp
    // BDD node 404:op_add
    // EP node  57749:RotateLeft
    // BDD node 147:rotate_left
    // EP node  58431:ArithmeticOp
    // BDD node 405:op_xor
    // EP node  58981:ArithmeticOp
    // BDD node 406:op_add
    // EP node  59673:RotateLeft
    // BDD node 148:rotate_left
    // EP node  60231:ArithmeticOp
    // BDD node 409:op_xor
    // EP node  60653:RotateLeft
    // BDD node 149:rotate_left
    // EP node  61078:ArithmeticOp
    // BDD node 407:op_xor
    // EP node  61364:ArithmeticOp
    // BDD node 408:op_add
    // EP node  62081:RotateLeft
    // BDD node 151:rotate_left
    // EP node  62659:ArithmeticOp
    // BDD node 410:op_add
    // EP node  63386:ArithmeticOp
    // BDD node 411:op_xor
    // EP node  64118:ArithmeticOp
    // BDD node 412:op_add
    // EP node  64708:RotateLeft
    // BDD node 150:rotate_left
    // EP node  65450:RotateLeft
    // BDD node 152:rotate_left
    // EP node  66495:ArithmeticOp
    // BDD node 415:op_xor
    // EP node  67097:RotateLeft
    // BDD node 153:rotate_left
    // EP node  67854:ArithmeticOp
    // BDD node 413:op_xor
    // EP node  68464:RotateLeft
    // BDD node 154:rotate_left
    // EP node  68925:ArithmeticOp
    // BDD node 414:op_add
    // EP node  69543:ArithmeticOp
    // BDD node 418:op_xor
    // EP node  70010:ArithmeticOp
    // BDD node 416:op_add
    // EP node  70792:ArithmeticOp
    // BDD node 420:op_xor
    // EP node  71579:RotateLeft
    // BDD node 155:rotate_left
    // EP node  72687:RotateLeft
    // BDD node 156:rotate_left
    // EP node  73166:ArithmeticOp
    // BDD node 417:op_xor
    // EP node  73808:RotateLeft
    // BDD node 157:rotate_left
    // EP node  74293:ArithmeticOp
    // BDD node 419:op_add
    // EP node  74943:ArithmeticOp
    // BDD node 422:op_xor
    // EP node  75434:RotateLeft
    // BDD node 158:rotate_left
    // EP node  75927:ArithmeticOp
    // BDD node 421:op_add
    // EP node  76754:ArithmeticOp
    // BDD node 423:op_add
    // EP node  77752:RotateLeft
    // BDD node 159:rotate_left
    // EP node  79927:ArithmeticOp
    // BDD node 424:op_xor
    // EP node  80769:ArithmeticOp
    // BDD node 425:op_add
    // EP node  81615:Recirculate
    // BDD node 160:rotate_left
    hdr.recirc.f32_0 = eg_md.rotate_left_144_x_out;
    hdr.recirc.f32_1 = eg_md.op_xor_422_out;
    hdr.recirc.f32_2 = eg_md.rotate_left_158_out;
    hdr.recirc.f32_3 = eg_md.op_add_421_out;
    hdr.recirc.f32_4 = eg_md.op_add_423_out;
    hdr.recirc.f32_5 = eg_md.rotate_left_159_x_out;
    hdr.recirc.f32_6 = eg_md.rotate_left_159_out;
    hdr.recirc.f32_7 = eg_md.op_add_425_out;
    hdr.egress_state.setInvalid();

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
