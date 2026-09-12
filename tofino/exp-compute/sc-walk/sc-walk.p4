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
  bit<32> f32_0;
  bit<32> f32_1;
  bit<32> f32_2;
  bit<32> f32_3;
  bit<32> f32_4;
  bit<32> f32_5;
  bit<32> f32_6;
  bit<32> f32_7;
  bit<32> f32_8;
  bit<32> f32_9;
  bit<32> f32_10;
  bit<32> f32_11;
  bit<32> f32_12;
  bit<32> f32_13;


};

header egress_state_h {
  bit<32> vector_reg_value0;
  bit<32> rotate_left_108_x_out;
  bit<32> rotate_left_114_out;
  bit<32> rotate_left_115_out;
  bit<32> rotate_left_116_out;
  bit<32> op_add_356_out;
  bit<32> rotate_left_117_out;
  bit<32> op_xor_357_out;
  bit<32> rotate_left_118_out;
  bit<32> f32_0;
  bit<32> f32_1;
  bit<32> f32_2;
  bit<32> rotate_left_162_out;
  bit<32> rotate_left_163_out;
  bit<32> rotate_left_164_out;
  bit<32> op_add_429_out;
  bit<32> rotate_left_165_out;
  bit<32> op_xor_430_out;
  bit<32> rotate_left_166_out;
  bit<32> dev;
  bit<32> vector_reg_value1;
  bit<32> rotate_left_117_x_out;
  bit<32> rotate_left_118_x_out;
  bit<32> rotate_left_144_x_out;
  bit<32> rotate_left_165_x_out;
  bit<32> rotate_left_166_x_out;
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
  bit<32> bf_1073926928_estimate;
  bit<24> key_24b_0;
  bit<8> key_8b_1;
  bit<24> key_24b_2;
  bit<8> key_8b_3;
  bit<16> key_16b_4;
  bit<16> key_16b_5;
  bit<32> rotate_left_107_out;
  bit<32> vector_reg_value0;
  bit<32> rotate_left_108_x_out;
  bit<32> rotate_left_108_out;
  bit<32> rotate_left_109_out;
  bit<32> rotate_left_110_x_out;
  bit<32> rotate_left_110_out;
  bit<32> op_xor_346_out;
  bit<32> op_add_347_out;
  bit<32> rotate_left_111_x_out;
  bit<32> rotate_left_111_out;
  bit<32> op_xor_348_out;
  bit<32> op_add_349_out;
  bit<32> rotate_left_112_x_out;
  bit<32> rotate_left_112_out;
  bit<32> op_add_350_out;
  bit<32> rotate_left_113_x_out;
  bit<32> rotate_left_113_out;
  bit<32> op_xor_351_out;
  bit<32> op_add_352_out;
  bit<32> rotate_left_114_x_out;
  bit<32> rotate_left_114_out;
  bit<32> op_xor_353_out;
  bit<32> rotate_left_115_x_out;
  bit<32> rotate_left_115_out;
  bit<32> op_add_354_out;
  bit<32> rotate_left_116_x_out;
  bit<32> rotate_left_116_out;
  bit<32> op_xor_355_out;
  bit<32> op_add_356_out;
  bit<32> rotate_left_117_x_out;
  bit<32> rotate_left_117_out;
  bit<32> op_xor_357_out;
  bit<32> rotate_left_118_x_out;
  bit<32> rotate_left_118_out;
  bit<32> op_add_396_out;
  bit<32> rotate_left_143_x_out;
  bit<32> rotate_left_143_out;
  bit<32> op_xor_397_out;
  bit<32> op_add_398_out;
  bit<32> op_xor_399_out;
  bit<32> op_add_285_out;
  bit<32> rotate_left_144_x_out;
  bit<32> rotate_left_144_out;
  bit<32> op_xor_400_out;
  bit<32> op_xor_401_out;
  bit<32> rotate_left_145_x_out;
  bit<32> rotate_left_145_out;
  bit<32> op_add_402_out;
  bit<32> rotate_left_146_x_out;
  bit<32> rotate_left_146_out;
  bit<32> op_xor_403_out;
  bit<32> op_add_404_out;
  bit<32> rotate_left_147_x_out;
  bit<32> rotate_left_147_out;
  bit<32> op_xor_405_out;
  bit<32> rotate_left_148_x_out;
  bit<32> rotate_left_148_out;
  bit<32> op_add_406_out;
  bit<32> rotate_left_149_x_out;
  bit<32> rotate_left_149_out;
  bit<32> op_xor_407_out;
  bit<32> op_add_408_out;
  bit<32> rotate_left_150_x_out;
  bit<32> rotate_left_150_out;
  bit<32> op_xor_409_out;
  bit<32> rotate_left_151_x_out;
  bit<32> rotate_left_151_out;
  bit<32> op_add_410_out;
  bit<32> rotate_left_152_x_out;
  bit<32> rotate_left_152_out;
  bit<32> op_xor_411_out;
  bit<32> op_add_412_out;
  bit<32> rotate_left_153_x_out;
  bit<32> rotate_left_153_out;
  bit<32> op_xor_413_out;
  bit<32> rotate_left_154_x_out;
  bit<32> rotate_left_154_out;
  bit<32> op_add_414_out;
  bit<32> rotate_left_155_x_out;
  bit<32> rotate_left_155_out;
  bit<32> op_xor_415_out;
  bit<32> op_add_416_out;
  bit<32> rotate_left_156_x_out;
  bit<32> rotate_left_156_out;
  bit<32> op_xor_417_out;
  bit<32> op_xor_418_out;
  bit<32> rotate_left_157_x_out;
  bit<32> rotate_left_157_out;
  bit<32> op_add_419_out;
  bit<32> rotate_left_158_x_out;
  bit<32> rotate_left_158_out;
  bit<32> op_xor_420_out;
  bit<32> op_add_421_out;
  bit<32> rotate_left_159_x_out;
  bit<32> rotate_left_159_out;
  bit<32> op_xor_422_out;
  bit<32> rotate_left_160_x_out;
  bit<32> rotate_left_160_out;
  bit<32> op_add_423_out;
  bit<32> rotate_left_161_x_out;
  bit<32> rotate_left_161_out;
  bit<32> op_xor_424_out;
  bit<32> op_add_425_out;
  bit<32> rotate_left_162_x_out;
  bit<32> rotate_left_162_out;
  bit<32> op_xor_426_out;
  bit<32> rotate_left_163_x_out;
  bit<32> rotate_left_163_out;
  bit<32> op_add_427_out;
  bit<32> rotate_left_164_x_out;
  bit<32> rotate_left_164_out;
  bit<32> op_xor_428_out;
  bit<32> op_add_429_out;
  bit<32> rotate_left_165_x_out;
  bit<32> rotate_left_165_out;
  bit<32> op_xor_430_out;
  bit<32> rotate_left_166_x_out;
  bit<32> rotate_left_166_out;
  bit<32> vector_reg_value1;
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
  bit<32> op_add_358_out;
  bit<32> rotate_left_119_x_out;
  bit<32> rotate_left_119_out;
  bit<32> op_xor_359_out;
  bit<32> op_add_360_out;
  bit<32> op_xor_361_out;
  bit<32> rotate_left_120_x_out;
  bit<32> rotate_left_120_out;
  bit<32> op_xor_362_out;
  bit<32> op_xor_363_out;
  bit<32> rotate_left_121_x_out;
  bit<32> rotate_left_121_out;
  bit<32> op_add_364_out;
  bit<32> rotate_left_122_x_out;
  bit<32> rotate_left_122_out;
  bit<32> op_xor_365_out;
  bit<32> op_add_366_out;
  bit<32> rotate_left_123_x_out;
  bit<32> rotate_left_123_out;
  bit<32> op_xor_367_out;
  bit<32> rotate_left_124_x_out;
  bit<32> rotate_left_124_out;
  bit<32> op_add_368_out;
  bit<32> rotate_left_125_x_out;
  bit<32> rotate_left_125_out;
  bit<32> op_xor_369_out;
  bit<32> op_add_370_out;
  bit<32> rotate_left_126_x_out;
  bit<32> rotate_left_126_out;
  bit<32> op_xor_371_out;
  bit<32> rotate_left_127_x_out;
  bit<32> rotate_left_127_out;
  bit<32> op_add_372_out;
  bit<32> rotate_left_128_x_out;
  bit<32> rotate_left_128_out;
  bit<32> op_xor_373_out;
  bit<32> op_add_374_out;
  bit<32> rotate_left_129_x_out;
  bit<32> rotate_left_129_out;
  bit<32> op_xor_375_out;
  bit<32> rotate_left_130_x_out;
  bit<32> rotate_left_130_out;
  bit<32> op_add_376_out;
  bit<32> rotate_left_131_x_out;
  bit<32> rotate_left_131_out;
  bit<32> op_xor_377_out;
  bit<32> op_add_378_out;
  bit<32> op_xor_379_out;
  bit<32> op_shl_380_a_out;
  bit<32> op_shl_380_out;
  bit<32> op_or_381_b_out;
  bit<32> op_or_381_out;
  bit<32> rotate_left_132_x_out;
  bit<32> rotate_left_132_out;
  bit<32> op_xor_382_out;
  bit<32> op_xor_383_out;
  bit<32> rotate_left_133_x_out;
  bit<32> rotate_left_133_out;
  bit<32> op_add_384_out;
  bit<32> rotate_left_134_x_out;
  bit<32> rotate_left_134_out;
  bit<32> op_xor_385_out;
  bit<32> op_add_386_out;
  bit<32> rotate_left_135_x_out;
  bit<32> rotate_left_135_out;
  bit<32> op_xor_387_out;
  bit<32> rotate_left_136_x_out;
  bit<32> rotate_left_136_out;
  bit<32> op_add_388_out;
  bit<32> rotate_left_137_x_out;
  bit<32> rotate_left_137_out;
  bit<32> op_xor_389_out;
  bit<32> op_add_390_out;
  bit<32> rotate_left_138_x_out;
  bit<32> rotate_left_138_out;
  bit<32> op_xor_391_out;
  bit<32> rotate_left_139_x_out;
  bit<32> rotate_left_139_out;
  bit<32> op_add_392_out;
  bit<32> rotate_left_140_x_out;
  bit<32> rotate_left_140_out;
  bit<32> op_xor_393_out;
  bit<32> op_add_394_out;
  bit<32> rotate_left_141_x_out;
  bit<32> rotate_left_141_out;
  bit<32> op_xor_395_out;
  bit<32> rotate_left_142_x_out;
  bit<32> rotate_left_142_out;
  bit<32> op_add_431_out;
  bit<32> rotate_left_167_x_out;
  bit<32> rotate_left_167_out;
  bit<32> op_xor_432_out;
  bit<32> op_add_433_out;
  bit<32> rotate_left_168_x_out;
  bit<32> rotate_left_168_out;
  bit<32> op_xor_434_out;
  bit<32> rotate_left_169_x_out;
  bit<32> rotate_left_169_out;
  bit<32> op_add_435_out;
  bit<32> rotate_left_170_x_out;
  bit<32> rotate_left_170_out;
  bit<32> op_xor_436_out;
  bit<32> op_add_437_out;
  bit<32> rotate_left_171_x_out;
  bit<32> rotate_left_171_out;
  bit<32> op_xor_438_out;
  bit<32> rotate_left_172_x_out;
  bit<32> rotate_left_172_out;
  bit<32> op_add_439_out;
  bit<32> rotate_left_173_x_out;
  bit<32> rotate_left_173_out;
  bit<32> op_xor_440_out;
  bit<32> op_add_441_out;
  bit<32> rotate_left_174_x_out;
  bit<32> rotate_left_174_out;
  bit<32> op_xor_442_out;
  bit<32> rotate_left_175_x_out;
  bit<32> rotate_left_175_out;
  bit<32> op_add_443_out;
  bit<32> rotate_left_176_x_out;
  bit<32> rotate_left_176_out;
  bit<32> op_xor_444_out;
  bit<32> op_add_445_out;
  bit<32> rotate_left_177_x_out;
  bit<32> rotate_left_177_out;
  bit<32> op_xor_446_out;
  bit<32> rotate_left_178_x_out;
  bit<32> rotate_left_178_out;
  bit<32> op_lshr_447_out;
  bit<32> op_sub_448_out;
  bit<32> op_lshr_449_out;
  bit<32> op_add_336_out;
  bit<32> op_xor_450_out;
  bit<32> op_add_451_out;
  bit<32> op_add_452_out;
  bit<32> op_xor_453_out;
  bit<32> op_xor_454_out;
  bit<32> op_xor_455_out;
  bit<32> op_xor_456_out;
  bit<32> op_xor_457_out;
  bit<32> op_xor_345_out;
  bit<32> cond_operand_90_0_out;
  bit<32> hdr_val0;
  bit<8> hdr_val1;
  bit<32> hdr_val2;
  bit<32> hdr_val3;
  bit<8> hdr_val4;

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
    pkt.extract(hdr.egress_state);

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

    fwd(CPU_PCIE_PORT);
  }

  action fwd_nf_dev(bit<16> port) {
    hdr.cpu.setInvalid();
    hdr.recirc.setInvalid();
    hdr.cuckoo.setInvalid();
    hdr.egress_state.setInvalid();

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

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_538;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_538;

  action bf_1073926928_hash_0_538_calc_538() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_538.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_538_calc_538() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_538.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0x2681580b
    });
  }
  Register<bit<32>,_>(1, 0) vector_register_1073939504_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073939504_0) vector_register_1073939504_0_read_150400 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action compute_op_add_285() {
    meta.op_add_285_out = 32w0xffffffff + hdr.hdr2.data1;
  }


  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073939504_0) vector_register_1073939504_0_read_1737 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action compute_rotate_left_107() {
    meta.rotate_left_107_out = 32w2225785509;
    meta.rotate_left_108_x_out = (32w0x3b355c4b) ^ (hdr.hdr1.data3);
    meta.rotate_left_109_out = 32w3399118710;
    meta.op_xor_346_out = 32w0x3b355c4b ^ hdr.hdr1.data3;
  }

  action compute_rotate_left_108() {
    @in_hash { meta.rotate_left_108_out = meta.rotate_left_108_x_out[23:0] ++ meta.rotate_left_108_x_out[31:24]; }
    meta.rotate_left_110_x_out = (32w0x6f76ca9a) ^ (meta.rotate_left_107_out);
    meta.op_add_347_out = 32w0x5d476351 + meta.op_xor_346_out;
    meta.op_xor_348_out = 32w0x6f76ca9a ^ meta.rotate_left_107_out;
  }

  action compute_rotate_left_110() {
    @in_hash { meta.rotate_left_110_out = meta.rotate_left_110_x_out[18:0] ++ meta.rotate_left_110_x_out[31:19]; }
    meta.rotate_left_111_x_out = (meta.rotate_left_108_out) ^ (meta.op_add_347_out);
    meta.op_add_349_out = meta.op_xor_346_out + meta.op_xor_348_out;
    meta.op_xor_351_out = meta.rotate_left_108_out ^ meta.op_add_347_out;
  }

  action compute_rotate_left_111() {
    @in_hash { meta.rotate_left_111_out = meta.rotate_left_111_x_out[24:0] ++ meta.rotate_left_111_x_out[31:25]; }
    meta.rotate_left_112_x_out = (32w0x5d476351) + (meta.op_add_349_out);
    meta.op_add_350_out = 32w0x5d476351 + meta.op_add_349_out;
    meta.op_add_352_out = meta.rotate_left_109_out + meta.op_xor_351_out;
  }

  action compute_rotate_left_112() {
    @in_hash { meta.rotate_left_112_out = meta.rotate_left_112_x_out[15:0] ++ meta.rotate_left_112_x_out[31:16]; }
    meta.rotate_left_113_x_out = (meta.rotate_left_110_out) ^ (meta.op_add_350_out);
    meta.rotate_left_114_x_out = (meta.rotate_left_111_out) ^ (meta.op_add_352_out);
    meta.op_xor_353_out = meta.rotate_left_110_out ^ meta.op_add_350_out;
    meta.op_xor_355_out = meta.rotate_left_111_out ^ meta.op_add_352_out;
  }

  action compute_rotate_left_113() {
    @in_hash { meta.rotate_left_113_out = meta.rotate_left_113_x_out[26:0] ++ meta.rotate_left_113_x_out[31:27]; }
    meta.rotate_left_115_x_out = (meta.op_add_352_out) + (meta.op_xor_353_out);
    meta.op_add_354_out = meta.op_add_352_out + meta.op_xor_353_out;
    meta.op_add_356_out = meta.rotate_left_112_out + meta.op_xor_355_out;
  }

  action compute_rotate_left_113_h1() {
    @in_hash { meta.rotate_left_114_out = meta.rotate_left_114_x_out[23:0] ++ meta.rotate_left_114_x_out[31:24]; }
  }

  action compute_rotate_left_115() {
    @in_hash { meta.rotate_left_115_out = meta.rotate_left_115_x_out[15:0] ++ meta.rotate_left_115_x_out[31:16]; }
    meta.rotate_left_116_x_out = (meta.rotate_left_113_out) ^ (meta.op_add_354_out);
    meta.rotate_left_117_x_out = (meta.rotate_left_114_out) ^ (meta.op_add_356_out);
    meta.op_xor_357_out = meta.rotate_left_113_out ^ meta.op_add_354_out;
  }

  action compute_rotate_left_116() {
    @in_hash { meta.rotate_left_116_out = meta.rotate_left_116_x_out[18:0] ++ meta.rotate_left_116_x_out[31:19]; }
  }

  action compute_rotate_left_117() {
    @in_hash { meta.rotate_left_117_out = meta.rotate_left_117_x_out[24:0] ++ meta.rotate_left_117_x_out[31:25]; }
    meta.rotate_left_118_x_out = (meta.op_add_356_out) + (meta.op_xor_357_out);
  }

  action compute_rotate_left_118() {
    @in_hash { meta.rotate_left_118_out = meta.rotate_left_118_x_out[15:0] ++ meta.rotate_left_118_x_out[31:16]; }
  }

  action select_unrolled__54() {
    meta.op_add_285_out = hdr.hdr2.data1;
  }

  action compute_op_add_396() {
    meta.op_add_396_out = hdr.recirc.f32_7 + hdr.recirc.f32_9;
    meta.op_xor_397_out = hdr.recirc.f32_4 ^ hdr.recirc.f32_7;
  }

  action compute_rotate_left_143_x() {
    meta.rotate_left_143_x_out = (hdr.recirc.f32_6) ^ (meta.op_add_396_out);
    meta.op_add_398_out = hdr.recirc.f32_5 + meta.op_xor_397_out;
    meta.op_xor_401_out = hdr.recirc.f32_6 ^ meta.op_add_396_out;
  }

  action compute_rotate_left_143() {
    @in_hash { meta.rotate_left_143_out = meta.rotate_left_143_x_out[26:0] ++ meta.rotate_left_143_x_out[31:27]; }
    meta.op_xor_399_out = hdr.recirc.f32_8 ^ meta.op_add_398_out;
    meta.op_xor_400_out = meta.op_add_398_out ^ hdr.recirc.f32_3;
  }

  action compute_rotate_left_144_x() {
    meta.rotate_left_144_x_out = (meta.op_xor_399_out) ^ (meta.op_add_285_out);
    meta.rotate_left_145_x_out = (meta.op_xor_400_out) + (meta.op_xor_401_out);
    meta.op_add_402_out = meta.op_xor_400_out + meta.op_xor_401_out;
    meta.op_xor_403_out = meta.op_xor_399_out ^ meta.op_add_285_out;
  }

  action compute_rotate_left_144() {
    @in_hash { meta.rotate_left_144_out = meta.rotate_left_144_x_out[23:0] ++ meta.rotate_left_144_x_out[31:24]; }
    meta.rotate_left_146_x_out = (meta.rotate_left_143_out) ^ (meta.op_add_402_out);
    meta.op_add_404_out = hdr.recirc.f32_10 + meta.op_xor_403_out;
    meta.op_xor_405_out = meta.rotate_left_143_out ^ meta.op_add_402_out;
  }

  action compute_rotate_left_144_h1() {
    @in_hash { meta.rotate_left_145_out = meta.rotate_left_145_x_out[15:0] ++ meta.rotate_left_145_x_out[31:16]; }
  }

  action compute_rotate_left_146() {
    @in_hash { meta.rotate_left_146_out = meta.rotate_left_146_x_out[18:0] ++ meta.rotate_left_146_x_out[31:19]; }
    meta.rotate_left_147_x_out = (meta.rotate_left_144_out) ^ (meta.op_add_404_out);
    meta.rotate_left_148_x_out = (meta.op_add_404_out) + (meta.op_xor_405_out);
    meta.op_add_406_out = meta.op_add_404_out + meta.op_xor_405_out;
    meta.op_xor_407_out = meta.rotate_left_144_out ^ meta.op_add_404_out;
  }

  action compute_rotate_left_147() {
    @in_hash { meta.rotate_left_147_out = meta.rotate_left_147_x_out[24:0] ++ meta.rotate_left_147_x_out[31:25]; }
    meta.rotate_left_149_x_out = (meta.rotate_left_146_out) ^ (meta.op_add_406_out);
    meta.op_add_408_out = meta.rotate_left_145_out + meta.op_xor_407_out;
    meta.op_xor_409_out = meta.rotate_left_146_out ^ meta.op_add_406_out;
  }

  action compute_rotate_left_147_h1() {
    @in_hash { meta.rotate_left_148_out = meta.rotate_left_148_x_out[15:0] ++ meta.rotate_left_148_x_out[31:16]; }
  }

  action compute_rotate_left_150_x() {
    meta.rotate_left_150_x_out = (meta.rotate_left_147_out) ^ (meta.op_add_408_out);
    meta.rotate_left_151_x_out = (meta.op_add_408_out) + (meta.op_xor_409_out);
    meta.op_add_410_out = meta.op_add_408_out + meta.op_xor_409_out;
    meta.op_xor_411_out = meta.rotate_left_147_out ^ meta.op_add_408_out;
  }

  action compute_rotate_left_149() {
    @in_hash { meta.rotate_left_149_out = meta.rotate_left_149_x_out[26:0] ++ meta.rotate_left_149_x_out[31:27]; }
    meta.op_add_412_out = meta.rotate_left_148_out + meta.op_xor_411_out;
  }

  action compute_rotate_left_149_h1() {
    @in_hash { meta.rotate_left_150_out = meta.rotate_left_150_x_out[23:0] ++ meta.rotate_left_150_x_out[31:24]; }
  }

  action compute_rotate_left_149_h2() {
    @in_hash { meta.rotate_left_151_out = meta.rotate_left_151_x_out[15:0] ++ meta.rotate_left_151_x_out[31:16]; }
  }

  action compute_rotate_left_152_x() {
    meta.rotate_left_152_x_out = (meta.rotate_left_149_out) ^ (meta.op_add_410_out);
    meta.rotate_left_153_x_out = (meta.rotate_left_150_out) ^ (meta.op_add_412_out);
    meta.op_xor_413_out = meta.rotate_left_149_out ^ meta.op_add_410_out;
    meta.op_xor_415_out = meta.rotate_left_150_out ^ meta.op_add_412_out;
  }

  action compute_rotate_left_152() {
    @in_hash { meta.rotate_left_152_out = meta.rotate_left_152_x_out[18:0] ++ meta.rotate_left_152_x_out[31:19]; }
  }

  action compute_rotate_left_153() {
    @in_hash { meta.rotate_left_153_out = meta.rotate_left_153_x_out[24:0] ++ meta.rotate_left_153_x_out[31:25]; }
    meta.rotate_left_154_x_out = (meta.op_add_412_out) + (meta.op_xor_413_out);
    meta.op_add_414_out = meta.op_add_412_out + meta.op_xor_413_out;
    meta.op_add_416_out = meta.rotate_left_151_out + meta.op_xor_415_out;
  }

  action compute_rotate_left_154() {
    @in_hash { meta.rotate_left_154_out = meta.rotate_left_154_x_out[15:0] ++ meta.rotate_left_154_x_out[31:16]; }
    meta.rotate_left_155_x_out = (meta.rotate_left_152_out) ^ (meta.op_add_414_out);
    meta.rotate_left_156_x_out = (meta.rotate_left_153_out) ^ (meta.op_add_416_out);
    meta.op_xor_417_out = meta.op_add_416_out ^ meta.op_add_285_out;
    meta.op_xor_418_out = meta.rotate_left_152_out ^ meta.op_add_414_out;
    meta.op_xor_420_out = meta.rotate_left_153_out ^ meta.op_add_416_out;
  }

  action compute_rotate_left_155() {
    @in_hash { meta.rotate_left_155_out = meta.rotate_left_155_x_out[26:0] ++ meta.rotate_left_155_x_out[31:27]; }
    meta.rotate_left_157_x_out = (meta.op_xor_417_out) + (meta.op_xor_418_out);
    meta.op_add_419_out = meta.op_xor_417_out + meta.op_xor_418_out;
    meta.op_add_421_out = meta.rotate_left_154_out + meta.op_xor_420_out;
  }

  action compute_rotate_left_155_h1() {
    @in_hash { meta.rotate_left_156_out = meta.rotate_left_156_x_out[23:0] ++ meta.rotate_left_156_x_out[31:24]; }
  }

  action compute_rotate_left_157() {
    @in_hash { meta.rotate_left_157_out = meta.rotate_left_157_x_out[15:0] ++ meta.rotate_left_157_x_out[31:16]; }
    meta.rotate_left_158_x_out = (meta.rotate_left_155_out) ^ (meta.op_add_419_out);
    meta.rotate_left_159_x_out = (meta.rotate_left_156_out) ^ (meta.op_add_421_out);
    meta.op_xor_422_out = meta.rotate_left_155_out ^ meta.op_add_419_out;
    meta.op_xor_424_out = meta.rotate_left_156_out ^ meta.op_add_421_out;
  }

  action compute_rotate_left_158() {
    @in_hash { meta.rotate_left_158_out = meta.rotate_left_158_x_out[18:0] ++ meta.rotate_left_158_x_out[31:19]; }
  }

  action compute_rotate_left_159() {
    @in_hash { meta.rotate_left_159_out = meta.rotate_left_159_x_out[24:0] ++ meta.rotate_left_159_x_out[31:25]; }
    meta.rotate_left_160_x_out = (meta.op_add_421_out) + (meta.op_xor_422_out);
    meta.op_add_423_out = meta.op_add_421_out + meta.op_xor_422_out;
    meta.op_add_425_out = meta.rotate_left_157_out + meta.op_xor_424_out;
  }

  action compute_rotate_left_160() {
    @in_hash { meta.rotate_left_160_out = meta.rotate_left_160_x_out[15:0] ++ meta.rotate_left_160_x_out[31:16]; }
    meta.rotate_left_161_x_out = (meta.rotate_left_158_out) ^ (meta.op_add_423_out);
    meta.rotate_left_162_x_out = (meta.rotate_left_159_out) ^ (meta.op_add_425_out);
    meta.op_xor_426_out = meta.rotate_left_158_out ^ meta.op_add_423_out;
    meta.op_xor_428_out = meta.rotate_left_159_out ^ meta.op_add_425_out;
  }

  action compute_rotate_left_161() {
    @in_hash { meta.rotate_left_161_out = meta.rotate_left_161_x_out[26:0] ++ meta.rotate_left_161_x_out[31:27]; }
    meta.rotate_left_163_x_out = (meta.op_add_425_out) + (meta.op_xor_426_out);
    meta.op_add_427_out = meta.op_add_425_out + meta.op_xor_426_out;
    meta.op_add_429_out = meta.rotate_left_160_out + meta.op_xor_428_out;
  }

  action compute_rotate_left_161_h1() {
    @in_hash { meta.rotate_left_162_out = meta.rotate_left_162_x_out[23:0] ++ meta.rotate_left_162_x_out[31:24]; }
  }

  action compute_rotate_left_163() {
    @in_hash { meta.rotate_left_163_out = meta.rotate_left_163_x_out[15:0] ++ meta.rotate_left_163_x_out[31:16]; }
    meta.rotate_left_164_x_out = (meta.rotate_left_161_out) ^ (meta.op_add_427_out);
    meta.rotate_left_165_x_out = (meta.rotate_left_162_out) ^ (meta.op_add_429_out);
    meta.op_xor_430_out = meta.rotate_left_161_out ^ meta.op_add_427_out;
  }

  action compute_rotate_left_164() {
    @in_hash { meta.rotate_left_164_out = meta.rotate_left_164_x_out[18:0] ++ meta.rotate_left_164_x_out[31:19]; }
  }

  action compute_rotate_left_165() {
    @in_hash { meta.rotate_left_165_out = meta.rotate_left_165_x_out[24:0] ++ meta.rotate_left_165_x_out[31:25]; }
    meta.rotate_left_166_x_out = (meta.op_add_429_out) + (meta.op_xor_430_out);
  }

  action compute_rotate_left_166() {
    @in_hash { meta.rotate_left_166_out = meta.rotate_left_166_x_out[15:0] ++ meta.rotate_left_166_x_out[31:16]; }
  }

  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_0_626828;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_1073926928_hash_1_626828;

  action bf_1073926928_hash_0_626828_calc_626828() {
    bf_1073926928_hash_0_value = bf_1073926928_hash_0_626828.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0xfbc31fc7
    });
  }
  action bf_1073926928_hash_1_626828_calc_626828() {
    bf_1073926928_hash_1_value = bf_1073926928_hash_1_626828.get({
      meta.key_24b_0,
      meta.key_8b_1,
      meta.key_24b_2,
      meta.key_8b_3,
      meta.key_16b_4,
      meta.key_16b_5,
      32w0x2681580b
    });
  }

  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  359203:ArithmeticOp
        // BDD node 281:op_add
        compute_op_add_285();
        compute_op_add_396();
        compute_rotate_left_143_x();
        compute_rotate_left_143();
        compute_rotate_left_144_x();
        compute_rotate_left_144();
        compute_rotate_left_144_h1();
        compute_rotate_left_146();
        compute_rotate_left_147();
        compute_rotate_left_147_h1();
        compute_rotate_left_150_x();
        compute_rotate_left_149();
        compute_rotate_left_149_h1();
        compute_rotate_left_149_h2();
        compute_rotate_left_152_x();
        compute_rotate_left_152();
        compute_rotate_left_153();
        compute_rotate_left_154();
        compute_rotate_left_155();
        compute_rotate_left_155_h1();
        compute_rotate_left_157();
        compute_rotate_left_158();
        compute_rotate_left_159();
        compute_rotate_left_160();
        compute_rotate_left_161();
        compute_rotate_left_161_h1();
        compute_rotate_left_163();
        compute_rotate_left_164();
        compute_rotate_left_165();
        compute_rotate_left_166();
        // EP node  361720:RotateLeft
        // BDD node 52:rotate_left
        // EP node  365501:ArithmeticOp
        // BDD node 282:op_xor
        // EP node  367719:ArithmeticOp
        // BDD node 283:op_add
        // EP node  370260:ArithmeticOp
        // BDD node 284:op_xor
        // EP node  372492:ArithmeticOp
        // BDD node 285:op_add
        // EP node  375049:RotateLeft
        // BDD node 53:rotate_left
        // EP node  377295:ArithmeticOp
        // BDD node 286:op_xor
        // EP node  379549:ArithmeticOp
        // BDD node 287:op_xor
        // EP node  382131:RotateLeft
        // BDD node 54:rotate_left
        // EP node  384399:ArithmeticOp
        // BDD node 288:op_add
        // EP node  386997:RotateLeft
        // BDD node 55:rotate_left
        // EP node  390575:ArithmeticOp
        // BDD node 289:op_xor
        // EP node  392864:ArithmeticOp
        // BDD node 290:op_add
        // EP node  395486:RotateLeft
        // BDD node 56:rotate_left
        // EP node  399097:ArithmeticOp
        // BDD node 291:op_xor
        // EP node  401735:RotateLeft
        // BDD node 57:rotate_left
        // EP node  404052:ArithmeticOp
        // BDD node 292:op_add
        // EP node  406706:RotateLeft
        // BDD node 58:rotate_left
        // EP node  410361:ArithmeticOp
        // BDD node 293:op_xor
        // EP node  412699:ArithmeticOp
        // BDD node 294:op_add
        // EP node  415377:RotateLeft
        // BDD node 59:rotate_left
        // EP node  417729:ArithmeticOp
        // BDD node 295:op_xor
        // EP node  420423:RotateLeft
        // BDD node 60:rotate_left
        // EP node  422789:ArithmeticOp
        // BDD node 296:op_add
        // EP node  425499:RotateLeft
        // BDD node 61:rotate_left
        // EP node  429231:ArithmeticOp
        // BDD node 297:op_xor
        // EP node  431618:ArithmeticOp
        // BDD node 298:op_add
        // EP node  434352:RotateLeft
        // BDD node 62:rotate_left
        // EP node  438117:ArithmeticOp
        // BDD node 299:op_xor
        // EP node  440867:RotateLeft
        // BDD node 63:rotate_left
        // EP node  443282:ArithmeticOp
        // BDD node 300:op_add
        // EP node  446048:RotateLeft
        // BDD node 64:rotate_left
        // EP node  449857:ArithmeticOp
        // BDD node 301:op_xor
        // EP node  452293:ArithmeticOp
        // BDD node 302:op_add
        // EP node  455430:RotateLeft
        // BDD node 65:rotate_left
        // EP node  458228:ArithmeticOp
        // BDD node 303:op_xor
        // EP node  460685:ArithmeticOp
        // BDD node 304:op_xor
        // EP node  463499:RotateLeft
        // BDD node 66:rotate_left
        // EP node  465970:ArithmeticOp
        // BDD node 305:op_add
        // EP node  468800:RotateLeft
        // BDD node 67:rotate_left
        // EP node  472697:ArithmeticOp
        // BDD node 306:op_xor
        // EP node  475189:ArithmeticOp
        // BDD node 307:op_add
        // EP node  478043:RotateLeft
        // BDD node 68:rotate_left
        // EP node  481973:ArithmeticOp
        // BDD node 308:op_xor
        // EP node  484843:RotateLeft
        // BDD node 69:rotate_left
        // EP node  487363:ArithmeticOp
        // BDD node 309:op_add
        // EP node  490249:RotateLeft
        // BDD node 70:rotate_left
        // EP node  494223:ArithmeticOp
        // BDD node 310:op_xor
        // EP node  496764:ArithmeticOp
        // BDD node 311:op_add
        // EP node  499674:RotateLeft
        // BDD node 71:rotate_left
        // EP node  502229:ArithmeticOp
        // BDD node 312:op_xor
        // EP node  505155:RotateLeft
        // BDD node 72:rotate_left
        // EP node  507724:ArithmeticOp
        // BDD node 313:op_add
        // EP node  510666:RotateLeft
        // BDD node 73:rotate_left
        // EP node  514717:ArithmeticOp
        // BDD node 314:op_xor
        // EP node  517307:ArithmeticOp
        // BDD node 315:op_add
        // EP node  520273:RotateLeft
        // BDD node 74:rotate_left
        // EP node  524357:ArithmeticOp
        // BDD node 316:op_xor
        // EP node  527339:RotateLeft
        // BDD node 75:rotate_left
        // EP node  529584:SendToEgress
        // BDD node 317:op_add
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.vector_reg_value0 = hdr.egress_state.vector_reg_value0;
        hdr.egress_state.rotate_left_108_x_out = hdr.egress_state.rotate_left_108_x_out;
        hdr.egress_state.f32_0 = hdr.recirc.f32_0;
        hdr.egress_state.f32_1 = hdr.recirc.f32_1;
        hdr.egress_state.f32_2 = hdr.recirc.f32_2;
        hdr.egress_state.rotate_left_162_out = meta.rotate_left_162_out;
        hdr.egress_state.rotate_left_163_out = meta.rotate_left_163_out;
        hdr.egress_state.rotate_left_164_out = meta.rotate_left_164_out;
        hdr.egress_state.op_add_429_out = meta.op_add_429_out;
        hdr.egress_state.rotate_left_165_out = meta.rotate_left_165_out;
        hdr.egress_state.op_xor_430_out = meta.op_xor_430_out;
        hdr.egress_state.rotate_left_166_out = meta.rotate_left_166_out;
        nf_dev[15:0] = 16w0x0000;
      } else if (hdr.recirc.code_path == 1) {
        // EP node  43277:ArithmeticOp
        // BDD node 396:op_add
        select_unrolled__54();
        compute_op_add_396();
        compute_rotate_left_143_x();
        compute_rotate_left_143();
        compute_rotate_left_144_x();
        compute_rotate_left_144();
        compute_rotate_left_144_h1();
        compute_rotate_left_146();
        compute_rotate_left_147();
        compute_rotate_left_147_h1();
        compute_rotate_left_150_x();
        compute_rotate_left_149();
        compute_rotate_left_149_h1();
        compute_rotate_left_149_h2();
        compute_rotate_left_152_x();
        compute_rotate_left_152();
        compute_rotate_left_153();
        compute_rotate_left_154();
        compute_rotate_left_155();
        compute_rotate_left_155_h1();
        compute_rotate_left_157();
        compute_rotate_left_158();
        compute_rotate_left_159();
        compute_rotate_left_160();
        compute_rotate_left_161();
        compute_rotate_left_161_h1();
        compute_rotate_left_163();
        compute_rotate_left_164();
        compute_rotate_left_165();
        compute_rotate_left_166();
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
        // EP node  88470:ArithmeticOp
        // BDD node 423:op_add
        // EP node  89639:RotateLeft
        // BDD node 161:rotate_left
        // EP node  91147:ArithmeticOp
        // BDD node 424:op_xor
        // EP node  92163:ArithmeticOp
        // BDD node 425:op_add
        // EP node  93353:RotateLeft
        // BDD node 162:rotate_left
        // EP node  94381:ArithmeticOp
        // BDD node 426:op_xor
        // EP node  95585:RotateLeft
        // BDD node 163:rotate_left
        // EP node  96625:ArithmeticOp
        // BDD node 427:op_add
        // EP node  97843:RotateLeft
        // BDD node 164:rotate_left
        // EP node  99414:ArithmeticOp
        // BDD node 428:op_xor
        // EP node  100472:ArithmeticOp
        // BDD node 429:op_add
        // EP node  101711:RotateLeft
        // BDD node 165:rotate_left
        // EP node  103309:ArithmeticOp
        // BDD node 430:op_xor
        // EP node  104562:RotateLeft
        // BDD node 166:rotate_left
        // EP node  105465:SendToEgress
        // BDD node 431:op_add
        meta.to_egress = 1;
        hdr.egress_state.setValid();
        hdr.egress_state.dev = hdr.egress_state.dev;
        hdr.egress_state.vector_reg_value1 = hdr.egress_state.vector_reg_value1;
        hdr.egress_state.rotate_left_108_x_out = hdr.egress_state.rotate_left_108_x_out;
        hdr.egress_state.f32_0 = hdr.recirc.f32_0;
        hdr.egress_state.f32_1 = hdr.recirc.f32_1;
        hdr.egress_state.f32_2 = hdr.recirc.f32_2;
        @in_hash { hdr.egress_state.rotate_left_144_x_out = meta.rotate_left_144_x_out; }
        hdr.egress_state.rotate_left_162_out = meta.rotate_left_162_out;
        hdr.egress_state.rotate_left_163_out = meta.rotate_left_163_out;
        hdr.egress_state.rotate_left_164_out = meta.rotate_left_164_out;
        hdr.egress_state.op_add_429_out = meta.op_add_429_out;
        @in_hash { hdr.egress_state.rotate_left_165_x_out = meta.rotate_left_165_x_out; }
        hdr.egress_state.rotate_left_165_out = meta.rotate_left_165_out;
        hdr.egress_state.op_xor_430_out = meta.op_xor_430_out;
        @in_hash { hdr.egress_state.rotate_left_166_x_out = meta.rotate_left_166_x_out; }
        hdr.egress_state.rotate_left_166_out = meta.rotate_left_166_out;
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
          // EP node  655584:Forward
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
                // EP node  538:BloomFilterQuery
                // BDD node 14:bf_query
                meta.key_24b_0 = hdr.hdr1.data3[31:8];
                meta.key_8b_1 = hdr.hdr1.data3[7:0];
                meta.key_24b_2 = hdr.hdr1.data4[31:8];
                meta.key_8b_3 = hdr.hdr1.data4[7:0];
                meta.key_16b_4 = hdr.hdr2.data0[31:16];
                meta.key_16b_5 = hdr.hdr2.data0[15:0];
                bf_1073926928_hash_0_538_calc_538();
                bf_1073926928_hash_1_538_calc_538();
                meta.bf_1073926928_estimate = 0;
                bf_1073926928_row_0_read_execute();
                bf_1073926928_row_1_read_execute();
                // EP node  833:If
                // BDD node 15:if
                if ((32w0x00000000) == (meta.bf_1073926928_estimate)){
                  // EP node  834:Then
                  // BDD node 15:if
                  // EP node  145704:RotateLeft
                  // BDD node 16:rotate_left
                  compute_rotate_left_107();
                  // EP node  150400:VectorRegisterLookup
                  // BDD node 88:vector_borrow
                  meta.vector_reg_value0 = vector_register_1073939504_0_read_150400.execute(32w0x00000000);
                  // EP node  153100:Ignore
                  // BDD node 89:vector_return
                  // EP node  156038:RotateLeft
                  // BDD node 17:rotate_left
                  compute_rotate_left_107();
                  compute_rotate_left_108();
                  compute_rotate_left_110();
                  compute_rotate_left_111();
                  compute_rotate_left_112();
                  compute_rotate_left_113();
                  compute_rotate_left_113_h1();
                  compute_rotate_left_115();
                  compute_rotate_left_116();
                  compute_rotate_left_117();
                  compute_rotate_left_118();
                  // EP node  158538:RotateLeft
                  // BDD node 18:rotate_left
                  // EP node  160822:RotateLeft
                  // BDD node 19:rotate_left
                  // EP node  164256:ArithmeticOp
                  // BDD node 231:op_xor
                  // EP node  166331:ArithmeticOp
                  // BDD node 232:op_add
                  // EP node  168645:RotateLeft
                  // BDD node 20:rotate_left
                  // EP node  172124:ArithmeticOp
                  // BDD node 233:op_xor
                  // EP node  174226:ArithmeticOp
                  // BDD node 234:op_add
                  // EP node  176570:RotateLeft
                  // BDD node 21:rotate_left
                  // EP node  178690:ArithmeticOp
                  // BDD node 235:op_add
                  // EP node  181054:RotateLeft
                  // BDD node 22:rotate_left
                  // EP node  184608:ArithmeticOp
                  // BDD node 236:op_xor
                  // EP node  186755:ArithmeticOp
                  // BDD node 237:op_add
                  // EP node  189149:RotateLeft
                  // BDD node 23:rotate_left
                  // EP node  191314:ArithmeticOp
                  // BDD node 238:op_xor
                  // EP node  193728:RotateLeft
                  // BDD node 24:rotate_left
                  // EP node  195911:ArithmeticOp
                  // BDD node 239:op_add
                  // EP node  198345:RotateLeft
                  // BDD node 25:rotate_left
                  // EP node  202004:ArithmeticOp
                  // BDD node 240:op_xor
                  // EP node  204214:ArithmeticOp
                  // BDD node 241:op_add
                  // EP node  206678:RotateLeft
                  // BDD node 26:rotate_left
                  // EP node  210382:ArithmeticOp
                  // BDD node 242:op_xor
                  // EP node  212866:RotateLeft
                  // BDD node 27:rotate_left
                  // EP node  214863:SendToEgress
                  // BDD node 243:op_add
                  meta.to_egress = 1;
                  hdr.egress_state.setValid();
                  hdr.egress_state.vector_reg_value0 = meta.vector_reg_value0;
                  hdr.egress_state.rotate_left_108_x_out = meta.rotate_left_108_x_out;
                  hdr.egress_state.rotate_left_114_out = meta.rotate_left_114_out;
                  hdr.egress_state.rotate_left_115_out = meta.rotate_left_115_out;
                  hdr.egress_state.rotate_left_116_out = meta.rotate_left_116_out;
                  hdr.egress_state.op_add_356_out = meta.op_add_356_out;
                  hdr.egress_state.rotate_left_117_out = meta.rotate_left_117_out;
                  hdr.egress_state.op_xor_357_out = meta.op_xor_357_out;
                  hdr.egress_state.rotate_left_118_out = meta.rotate_left_118_out;
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
                  // EP node  1497:Then
                  // BDD node 104:if
                  // EP node  1737:VectorRegisterLookup
                  // BDD node 105:vector_borrow
                  meta.vector_reg_value1 = vector_register_1073939504_0_read_1737.execute(32w0x00000000);
                  // EP node  2229:Ignore
                  // BDD node 106:vector_return
                  // EP node  2552:RotateLeft
                  // BDD node 107:rotate_left
                  compute_rotate_left_107();
                  compute_rotate_left_108();
                  compute_rotate_left_110();
                  compute_rotate_left_111();
                  compute_rotate_left_112();
                  compute_rotate_left_113();
                  compute_rotate_left_113_h1();
                  compute_rotate_left_115();
                  compute_rotate_left_116();
                  compute_rotate_left_117();
                  compute_rotate_left_118();
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
                  hdr.egress_state.dev = meta.dev;
                  hdr.egress_state.vector_reg_value1 = meta.vector_reg_value1;
                  @in_hash { hdr.egress_state.rotate_left_108_x_out = meta.rotate_left_108_x_out; }
                  hdr.egress_state.rotate_left_114_out = meta.rotate_left_114_out;
                  hdr.egress_state.rotate_left_115_out = meta.rotate_left_115_out;
                  hdr.egress_state.rotate_left_116_out = meta.rotate_left_116_out;
                  hdr.egress_state.op_add_356_out = meta.op_add_356_out;
                  @in_hash { hdr.egress_state.rotate_left_117_x_out = meta.rotate_left_117_x_out; }
                  hdr.egress_state.rotate_left_117_out = meta.rotate_left_117_out;
                  hdr.egress_state.op_xor_357_out = meta.op_xor_357_out;
                  @in_hash { hdr.egress_state.rotate_left_118_x_out = meta.rotate_left_118_x_out; }
                  hdr.egress_state.rotate_left_118_out = meta.rotate_left_118_out;
                  fwd_op = fwd_op_t.RECIRCULATE;
                  build_recirc_hdr(1);
                } else {
                  // EP node  1498:Else
                  // BDD node 104:if
                  // EP node  663935:Drop
                  // BDD node 187:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  230:Else
              // BDD node 12:if
              // EP node  621320:If
              // BDD node 188:if
              if ((32w0x00000000) == (((bit<32>)(hdr.hdr2.data3[7:0])) & (32w0x00000040))){
                // EP node  621321:Then
                // BDD node 188:if
                // EP node  626404:Forward
                // BDD node 192:FORWARD
                nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
              } else {
                // EP node  621322:Else
                // BDD node 188:if
                // EP node  626828:BloomFilterSet
                // BDD node 193:bf_set
                meta.key_24b_0 = hdr.hdr1.data3[31:8];
                meta.key_8b_1 = hdr.hdr1.data3[7:0];
                meta.key_24b_2 = hdr.hdr1.data4[31:8];
                meta.key_8b_3 = hdr.hdr1.data4[7:0];
                meta.key_16b_4 = hdr.hdr2.data0[31:16];
                meta.key_16b_5 = hdr.hdr2.data0[15:0];
                bf_1073926928_hash_0_626828_calc_626828();
                bf_1073926928_hash_1_626828_calc_626828();
                bf_1073926928_row_0_set_to_one_execute();
                bf_1073926928_row_1_set_to_one_execute();
                // EP node  633634:Drop
                // BDD node 197:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  105:Else
          // BDD node 10:if
          // EP node  656456:ParserReject
          // BDD node 200:DROP
          // EP node  40:Else
          // BDD node 5:if
          // EP node  636183:ParserCondition
          // BDD node 201:if
          // EP node  636184:Then
          // BDD node 201:if
          // EP node  643906:ParserExtraction
          // BDD node 202:packet_borrow_next_chunk
          if(hdr.hdr3.isValid()) {
            // EP node  652971:If
            // BDD node 203:if
            if ((16w0x0000) == (meta.dev[15:0])){
              // EP node  652972:Then
              // BDD node 203:if
              // EP node  660401:ParserCondition
              // BDD node 204:if
              // EP node  660402:Then
              // BDD node 204:if
              // EP node  666148:ParserCondition
              // BDD node 205:if
              // EP node  666149:Then
              // BDD node 205:if
              // EP node  685477:Forward
              // BDD node 209:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
              // EP node  666150:Else
              // BDD node 205:if
              // EP node  672851:ParserExtraction
              // BDD node 210:packet_borrow_next_chunk
              if(hdr.hdr4.isValid()) {
                // EP node  680028:SendToController
                // BDD node 211:vector_borrow
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(0);
              }
              // EP node  660403:Else
              // BDD node 204:if
              // EP node  684567:Forward
              // BDD node 221:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
            } else {
              // EP node  652973:Else
              // BDD node 203:if
              // EP node  682295:Forward
              // BDD node 225:FORWARD
              nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
            }
          }
          // EP node  636185:Else
          // BDD node 201:if
          // EP node  663051:Forward
          // BDD node 228:FORWARD
          nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);
        }
        // EP node  7:Else
        // BDD node 3:if
        // EP node  643046:ParserReject
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
    pkt.extract(hdr.recirc);
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

  action compute_op_add_336() {
    eg_md.op_add_336_out = 32w0xffffffff + hdr.hdr2.data2;
  }

  action compute_op_sub_334() {
    eg_md.op_sub_448_out = eg_md.op_lshr_447_out[31:0] - hdr.egress_state.vector_reg_value0;
  }

  action compute_op_xor_345() {
    eg_md.op_xor_345_out = eg_md.op_add_336_out ^ eg_md.op_xor_457_out;
  }

  action compute_cond_operand_90_0() {
    eg_md.cond_operand_90_0_out = (eg_md.op_lshr_449_out) - (eg_md.op_xor_345_out);
  }

  action swap_action_92() {
  }
  action hdr_val0_calc() { eg_md.hdr_val0 = (32w0xffffffff) + (hdr.hdr2.data1); }

  action hdr_val1_calc() { eg_md.hdr_val1 = (hdr.hdr2.data3[7:0]) | (8w0x40); }

  action swap_action_93() {
  }
  action compute_op_add_358() {
    eg_md.op_add_358_out = hdr.egress_state.op_add_356_out + hdr.egress_state.op_xor_357_out;
    eg_md.op_xor_359_out = hdr.egress_state.rotate_left_114_out ^ hdr.egress_state.op_add_356_out;
    eg_md.op_shl_380_a_out = (bit<32>)(hdr.hdr2.data0[31:16]);
    eg_md.op_or_381_b_out = (bit<32>)(hdr.hdr2.data0[15:0]);
  }

  action compute_rotate_left_119_x() {
    eg_md.rotate_left_119_x_out = (hdr.egress_state.rotate_left_116_out) ^ (eg_md.op_add_358_out);
    eg_md.op_add_360_out = hdr.egress_state.rotate_left_115_out + eg_md.op_xor_359_out;
    eg_md.op_xor_363_out = hdr.egress_state.rotate_left_116_out ^ eg_md.op_add_358_out;
    eg_md.op_shl_380_out = eg_md.op_shl_380_a_out << 32w0x00000010;
  }

  action compute_rotate_left_119() {
    @in_hash { eg_md.rotate_left_119_out = eg_md.rotate_left_119_x_out[26:0] ++ eg_md.rotate_left_119_x_out[31:27]; }
    eg_md.op_xor_361_out = hdr.egress_state.rotate_left_117_out ^ eg_md.op_add_360_out;
    eg_md.op_xor_362_out = eg_md.op_add_360_out ^ hdr.hdr1.data3;
    eg_md.op_or_381_out = eg_md.op_shl_380_out | eg_md.op_or_381_b_out;
  }

  action compute_rotate_left_120_x() {
    eg_md.rotate_left_120_x_out = (eg_md.op_xor_361_out) ^ (hdr.hdr1.data4);
    eg_md.rotate_left_121_x_out = (eg_md.op_xor_362_out) + (eg_md.op_xor_363_out);
    eg_md.op_add_364_out = eg_md.op_xor_362_out + eg_md.op_xor_363_out;
    eg_md.op_xor_365_out = eg_md.op_xor_361_out ^ hdr.hdr1.data4;
  }

  action compute_rotate_left_120() {
    @in_hash { eg_md.rotate_left_120_out = eg_md.rotate_left_120_x_out[23:0] ++ eg_md.rotate_left_120_x_out[31:24]; }
    eg_md.rotate_left_122_x_out = (eg_md.rotate_left_119_out) ^ (eg_md.op_add_364_out);
    eg_md.op_add_366_out = hdr.egress_state.rotate_left_118_out + eg_md.op_xor_365_out;
    eg_md.op_xor_367_out = eg_md.rotate_left_119_out ^ eg_md.op_add_364_out;
  }

  action compute_rotate_left_120_h1() {
    @in_hash { eg_md.rotate_left_121_out = eg_md.rotate_left_121_x_out[15:0] ++ eg_md.rotate_left_121_x_out[31:16]; }
  }

  action compute_rotate_left_122() {
    @in_hash { eg_md.rotate_left_122_out = eg_md.rotate_left_122_x_out[18:0] ++ eg_md.rotate_left_122_x_out[31:19]; }
    eg_md.rotate_left_123_x_out = (eg_md.rotate_left_120_out) ^ (eg_md.op_add_366_out);
    eg_md.rotate_left_124_x_out = (eg_md.op_add_366_out) + (eg_md.op_xor_367_out);
    eg_md.op_add_368_out = eg_md.op_add_366_out + eg_md.op_xor_367_out;
    eg_md.op_xor_369_out = eg_md.rotate_left_120_out ^ eg_md.op_add_366_out;
  }

  action compute_rotate_left_123() {
    @in_hash { eg_md.rotate_left_123_out = eg_md.rotate_left_123_x_out[24:0] ++ eg_md.rotate_left_123_x_out[31:25]; }
    eg_md.rotate_left_125_x_out = (eg_md.rotate_left_122_out) ^ (eg_md.op_add_368_out);
    eg_md.op_add_370_out = eg_md.rotate_left_121_out + eg_md.op_xor_369_out;
    eg_md.op_xor_371_out = eg_md.rotate_left_122_out ^ eg_md.op_add_368_out;
  }

  action compute_rotate_left_123_h1() {
    @in_hash { eg_md.rotate_left_124_out = eg_md.rotate_left_124_x_out[15:0] ++ eg_md.rotate_left_124_x_out[31:16]; }
  }

  action compute_rotate_left_125() {
    @in_hash { eg_md.rotate_left_125_out = eg_md.rotate_left_125_x_out[26:0] ++ eg_md.rotate_left_125_x_out[31:27]; }
    eg_md.rotate_left_126_x_out = (eg_md.rotate_left_123_out) ^ (eg_md.op_add_370_out);
    eg_md.rotate_left_127_x_out = (eg_md.op_add_370_out) + (eg_md.op_xor_371_out);
    eg_md.op_add_372_out = eg_md.op_add_370_out + eg_md.op_xor_371_out;
    eg_md.op_xor_373_out = eg_md.rotate_left_123_out ^ eg_md.op_add_370_out;
  }

  action compute_rotate_left_126() {
    @in_hash { eg_md.rotate_left_126_out = eg_md.rotate_left_126_x_out[23:0] ++ eg_md.rotate_left_126_x_out[31:24]; }
    eg_md.rotate_left_128_x_out = (eg_md.rotate_left_125_out) ^ (eg_md.op_add_372_out);
    eg_md.op_add_374_out = eg_md.rotate_left_124_out + eg_md.op_xor_373_out;
    eg_md.op_xor_375_out = eg_md.rotate_left_125_out ^ eg_md.op_add_372_out;
  }

  action compute_rotate_left_126_h1() {
    @in_hash { eg_md.rotate_left_127_out = eg_md.rotate_left_127_x_out[15:0] ++ eg_md.rotate_left_127_x_out[31:16]; }
  }

  action compute_rotate_left_128() {
    @in_hash { eg_md.rotate_left_128_out = eg_md.rotate_left_128_x_out[18:0] ++ eg_md.rotate_left_128_x_out[31:19]; }
    eg_md.rotate_left_129_x_out = (eg_md.rotate_left_126_out) ^ (eg_md.op_add_374_out);
    eg_md.rotate_left_130_x_out = (eg_md.op_add_374_out) + (eg_md.op_xor_375_out);
    eg_md.op_add_376_out = eg_md.op_add_374_out + eg_md.op_xor_375_out;
    eg_md.op_xor_377_out = eg_md.rotate_left_126_out ^ eg_md.op_add_374_out;
  }

  action compute_rotate_left_129() {
    @in_hash { eg_md.rotate_left_129_out = eg_md.rotate_left_129_x_out[24:0] ++ eg_md.rotate_left_129_x_out[31:25]; }
    eg_md.rotate_left_131_x_out = (eg_md.rotate_left_128_out) ^ (eg_md.op_add_376_out);
    eg_md.op_add_378_out = eg_md.rotate_left_127_out + eg_md.op_xor_377_out;
    eg_md.op_xor_383_out = eg_md.rotate_left_128_out ^ eg_md.op_add_376_out;
  }

  action compute_rotate_left_129_h1() {
    @in_hash { eg_md.rotate_left_130_out = eg_md.rotate_left_130_x_out[15:0] ++ eg_md.rotate_left_130_x_out[31:16]; }
  }

  action compute_rotate_left_131() {
    @in_hash { eg_md.rotate_left_131_out = eg_md.rotate_left_131_x_out[26:0] ++ eg_md.rotate_left_131_x_out[31:27]; }
    eg_md.op_xor_379_out = eg_md.rotate_left_129_out ^ eg_md.op_add_378_out;
    eg_md.op_xor_382_out = eg_md.op_add_378_out ^ hdr.hdr1.data4;
  }

  action compute_rotate_left_132_x() {
    eg_md.rotate_left_132_x_out = (eg_md.op_xor_379_out) ^ (eg_md.op_or_381_out);
    eg_md.rotate_left_133_x_out = (eg_md.op_xor_382_out) + (eg_md.op_xor_383_out);
    eg_md.op_add_384_out = eg_md.op_xor_382_out + eg_md.op_xor_383_out;
    eg_md.op_xor_385_out = eg_md.op_xor_379_out ^ eg_md.op_or_381_out;
  }

  action compute_rotate_left_132() {
    @in_hash { eg_md.rotate_left_132_out = eg_md.rotate_left_132_x_out[23:0] ++ eg_md.rotate_left_132_x_out[31:24]; }
    eg_md.rotate_left_134_x_out = (eg_md.rotate_left_131_out) ^ (eg_md.op_add_384_out);
    eg_md.op_add_386_out = eg_md.rotate_left_130_out + eg_md.op_xor_385_out;
    eg_md.op_xor_387_out = eg_md.rotate_left_131_out ^ eg_md.op_add_384_out;
  }

  action compute_rotate_left_132_h1() {
    @in_hash { eg_md.rotate_left_133_out = eg_md.rotate_left_133_x_out[15:0] ++ eg_md.rotate_left_133_x_out[31:16]; }
  }

  action compute_rotate_left_134() {
    @in_hash { eg_md.rotate_left_134_out = eg_md.rotate_left_134_x_out[18:0] ++ eg_md.rotate_left_134_x_out[31:19]; }
    eg_md.rotate_left_135_x_out = (eg_md.rotate_left_132_out) ^ (eg_md.op_add_386_out);
    eg_md.rotate_left_136_x_out = (eg_md.op_add_386_out) + (eg_md.op_xor_387_out);
    eg_md.op_add_388_out = eg_md.op_add_386_out + eg_md.op_xor_387_out;
    eg_md.op_xor_389_out = eg_md.rotate_left_132_out ^ eg_md.op_add_386_out;
  }

  action compute_rotate_left_135() {
    @in_hash { eg_md.rotate_left_135_out = eg_md.rotate_left_135_x_out[24:0] ++ eg_md.rotate_left_135_x_out[31:25]; }
    eg_md.rotate_left_137_x_out = (eg_md.rotate_left_134_out) ^ (eg_md.op_add_388_out);
    eg_md.op_add_390_out = eg_md.rotate_left_133_out + eg_md.op_xor_389_out;
    eg_md.op_xor_391_out = eg_md.rotate_left_134_out ^ eg_md.op_add_388_out;
  }

  action compute_rotate_left_135_h1() {
    @in_hash { eg_md.rotate_left_136_out = eg_md.rotate_left_136_x_out[15:0] ++ eg_md.rotate_left_136_x_out[31:16]; }
  }

  action compute_rotate_left_137() {
    @in_hash { eg_md.rotate_left_137_out = eg_md.rotate_left_137_x_out[26:0] ++ eg_md.rotate_left_137_x_out[31:27]; }
    eg_md.rotate_left_138_x_out = (eg_md.rotate_left_135_out) ^ (eg_md.op_add_390_out);
    eg_md.rotate_left_139_x_out = (eg_md.op_add_390_out) + (eg_md.op_xor_391_out);
    eg_md.op_add_392_out = eg_md.op_add_390_out + eg_md.op_xor_391_out;
    eg_md.op_xor_393_out = eg_md.rotate_left_135_out ^ eg_md.op_add_390_out;
  }

  action compute_rotate_left_138() {
    @in_hash { eg_md.rotate_left_138_out = eg_md.rotate_left_138_x_out[23:0] ++ eg_md.rotate_left_138_x_out[31:24]; }
    eg_md.rotate_left_140_x_out = (eg_md.rotate_left_137_out) ^ (eg_md.op_add_392_out);
    eg_md.op_add_394_out = eg_md.rotate_left_136_out + eg_md.op_xor_393_out;
    eg_md.op_xor_395_out = eg_md.rotate_left_137_out ^ eg_md.op_add_392_out;
  }

  action compute_rotate_left_138_h1() {
    @in_hash { eg_md.rotate_left_139_out = eg_md.rotate_left_139_x_out[15:0] ++ eg_md.rotate_left_139_x_out[31:16]; }
  }

  action compute_rotate_left_140() {
    @in_hash { eg_md.rotate_left_140_out = eg_md.rotate_left_140_x_out[18:0] ++ eg_md.rotate_left_140_x_out[31:19]; }
    eg_md.rotate_left_141_x_out = (eg_md.rotate_left_138_out) ^ (eg_md.op_add_394_out);
    eg_md.rotate_left_142_x_out = (eg_md.op_add_394_out) + (eg_md.op_xor_395_out);
  }

  action compute_rotate_left_141() {
    @in_hash { eg_md.rotate_left_141_out = eg_md.rotate_left_141_x_out[24:0] ++ eg_md.rotate_left_141_x_out[31:25]; }
  }

  action compute_rotate_left_141_h1() {
    @in_hash { eg_md.rotate_left_142_out = eg_md.rotate_left_142_x_out[15:0] ++ eg_md.rotate_left_142_x_out[31:16]; }
  }

  action compute_op_add_431() {
    eg_md.op_add_431_out = hdr.egress_state.op_add_429_out + hdr.egress_state.op_xor_430_out;
    eg_md.op_xor_432_out = hdr.egress_state.rotate_left_162_out ^ hdr.egress_state.op_add_429_out;
    eg_md.op_lshr_447_out = eg_md.time;
  }

  action compute_rotate_left_167_x() {
    eg_md.rotate_left_167_x_out = (hdr.egress_state.rotate_left_164_out) ^ (eg_md.op_add_431_out);
    eg_md.op_add_433_out = hdr.egress_state.rotate_left_163_out + eg_md.op_xor_432_out;
    eg_md.op_xor_434_out = hdr.egress_state.rotate_left_164_out ^ eg_md.op_add_431_out;
    eg_md.op_sub_448_out = eg_md.op_lshr_447_out[31:0] - hdr.egress_state.vector_reg_value1;
  }

  action compute_rotate_left_168_x() {
    eg_md.rotate_left_168_x_out = (hdr.egress_state.rotate_left_165_out) ^ (eg_md.op_add_433_out);
    eg_md.rotate_left_169_x_out = (eg_md.op_add_433_out) + (eg_md.op_xor_434_out);
    eg_md.op_add_435_out = eg_md.op_add_433_out + eg_md.op_xor_434_out;
    eg_md.op_xor_436_out = hdr.egress_state.rotate_left_165_out ^ eg_md.op_add_433_out;
    eg_md.op_lshr_449_out = eg_md.op_sub_448_out >> 32w0x0000000c;
  }

  action compute_rotate_left_167() {
    @in_hash { eg_md.rotate_left_167_out = eg_md.rotate_left_167_x_out[26:0] ++ eg_md.rotate_left_167_x_out[31:27]; }
    eg_md.op_add_437_out = hdr.egress_state.rotate_left_166_out + eg_md.op_xor_436_out;
  }

  action compute_rotate_left_167_h1() {
    @in_hash { eg_md.rotate_left_168_out = eg_md.rotate_left_168_x_out[23:0] ++ eg_md.rotate_left_168_x_out[31:24]; }
  }

  action compute_rotate_left_167_h2() {
    @in_hash { eg_md.rotate_left_169_out = eg_md.rotate_left_169_x_out[15:0] ++ eg_md.rotate_left_169_x_out[31:16]; }
  }

  action compute_rotate_left_170_x() {
    eg_md.rotate_left_170_x_out = (eg_md.rotate_left_167_out) ^ (eg_md.op_add_435_out);
    eg_md.rotate_left_171_x_out = (eg_md.rotate_left_168_out) ^ (eg_md.op_add_437_out);
    eg_md.op_xor_438_out = eg_md.rotate_left_167_out ^ eg_md.op_add_435_out;
    eg_md.op_xor_440_out = eg_md.rotate_left_168_out ^ eg_md.op_add_437_out;
  }

  action compute_rotate_left_172_x() {
    eg_md.rotate_left_172_x_out = (eg_md.op_add_437_out) + (eg_md.op_xor_438_out);
    eg_md.op_add_439_out = eg_md.op_add_437_out + eg_md.op_xor_438_out;
    eg_md.op_add_441_out = eg_md.rotate_left_169_out + eg_md.op_xor_440_out;
  }

  action compute_rotate_left_170() {
    @in_hash { eg_md.rotate_left_170_out = eg_md.rotate_left_170_x_out[18:0] ++ eg_md.rotate_left_170_x_out[31:19]; }
  }

  action compute_rotate_left_170_h1() {
    @in_hash { eg_md.rotate_left_172_out = eg_md.rotate_left_172_x_out[15:0] ++ eg_md.rotate_left_172_x_out[31:16]; }
  }

  action compute_rotate_left_173_x() {
    eg_md.rotate_left_173_x_out = (eg_md.rotate_left_170_out) ^ (eg_md.op_add_439_out);
    eg_md.op_xor_442_out = eg_md.rotate_left_170_out ^ eg_md.op_add_439_out;
  }

  action compute_rotate_left_171() {
    @in_hash { eg_md.rotate_left_171_out = eg_md.rotate_left_171_x_out[24:0] ++ eg_md.rotate_left_171_x_out[31:25]; }
  }

  action compute_rotate_left_173() {
    @in_hash { eg_md.rotate_left_173_out = eg_md.rotate_left_173_x_out[26:0] ++ eg_md.rotate_left_173_x_out[31:27]; }
    eg_md.rotate_left_175_x_out = (eg_md.op_add_441_out) + (eg_md.op_xor_442_out);
    eg_md.op_add_443_out = eg_md.op_add_441_out + eg_md.op_xor_442_out;
  }

  action compute_rotate_left_174_x() {
    eg_md.rotate_left_174_x_out = (eg_md.rotate_left_171_out) ^ (eg_md.op_add_441_out);
    @in_hash { eg_md.rotate_left_175_out = eg_md.rotate_left_175_x_out[15:0] ++ eg_md.rotate_left_175_x_out[31:16]; }
    eg_md.rotate_left_176_x_out = (eg_md.rotate_left_173_out) ^ (eg_md.op_add_443_out);
    eg_md.op_xor_444_out = eg_md.rotate_left_171_out ^ eg_md.op_add_441_out;
    eg_md.op_xor_446_out = eg_md.rotate_left_173_out ^ eg_md.op_add_443_out;
  }

  action compute_rotate_left_174() {
    @in_hash { eg_md.rotate_left_174_out = eg_md.rotate_left_174_x_out[23:0] ++ eg_md.rotate_left_174_x_out[31:24]; }
    eg_md.op_add_445_out = eg_md.rotate_left_172_out + eg_md.op_xor_444_out;
  }

  action compute_rotate_left_176() {
    @in_hash { eg_md.rotate_left_176_out = eg_md.rotate_left_176_x_out[18:0] ++ eg_md.rotate_left_176_x_out[31:19]; }
    eg_md.rotate_left_177_x_out = (eg_md.rotate_left_174_out) ^ (eg_md.op_add_445_out);
    eg_md.rotate_left_178_x_out = (eg_md.op_add_445_out) + (eg_md.op_xor_446_out);
    eg_md.op_xor_450_out = eg_md.rotate_left_174_out ^ eg_md.op_add_445_out;
    eg_md.op_add_452_out = eg_md.op_add_445_out + eg_md.op_xor_446_out;
  }

  action compute_rotate_left_177() {
    @in_hash { eg_md.rotate_left_177_out = eg_md.rotate_left_177_x_out[24:0] ++ eg_md.rotate_left_177_x_out[31:25]; }
    eg_md.op_add_451_out = eg_md.rotate_left_175_out + eg_md.op_xor_450_out;
    eg_md.op_xor_453_out = eg_md.rotate_left_176_out ^ eg_md.op_add_452_out;
  }

  action compute_rotate_left_177_h1() {
    @in_hash { eg_md.rotate_left_178_out = eg_md.rotate_left_178_x_out[15:0] ++ eg_md.rotate_left_178_x_out[31:16]; }
  }

  action compute_op_xor_454() {
    eg_md.op_xor_454_out = eg_md.op_add_451_out ^ eg_md.op_xor_453_out;
    eg_md.op_xor_456_out = eg_md.rotate_left_177_out ^ eg_md.op_add_451_out;
  }

  action compute_op_xor_455() {
    eg_md.op_xor_455_out = eg_md.op_xor_454_out ^ eg_md.rotate_left_178_out;
  }

  action compute_op_xor_457() {
    eg_md.op_xor_457_out = eg_md.op_xor_455_out ^ eg_md.op_xor_456_out;
  }

  action swap_action_180() {
    hdr.hdr2.data0 = hdr.hdr2.data0[15:0] ++ hdr.hdr2.data0[31:16];
  }
  action hdr_val2_calc() { eg_md.hdr_val2 = (eg_md.op_lshr_449_out) ^ (eg_md.op_xor_457_out); }

  action hdr_val3_calc() { eg_md.hdr_val3 = (32w0x00000001) + (hdr.hdr2.data1); }

  action hdr_val4_calc() { eg_md.hdr_val4 = (hdr.hdr2.data3[7:0]) | (8w0x12); }

  action swap_action_181() {
    swap32(hdr.hdr1.data3, hdr.hdr1.data4);
  }

  apply {
    eg_md.time = eg_intr_md_from_prsr.global_tstamp[47:16];
    // EP node  217366:ArithmeticOp
    // BDD node 243:op_add
    compute_op_add_358();
    compute_rotate_left_119_x();
    compute_rotate_left_119();
    compute_rotate_left_120_x();
    compute_rotate_left_120();
    compute_rotate_left_120_h1();
    compute_rotate_left_122();
    compute_rotate_left_123();
    compute_rotate_left_123_h1();
    compute_rotate_left_125();
    compute_rotate_left_126();
    compute_rotate_left_126_h1();
    compute_rotate_left_128();
    compute_rotate_left_129();
    compute_rotate_left_129_h1();
    compute_rotate_left_131();
    compute_rotate_left_132_x();
    compute_rotate_left_132();
    compute_rotate_left_132_h1();
    compute_rotate_left_134();
    compute_rotate_left_135();
    compute_rotate_left_135_h1();
    compute_rotate_left_137();
    compute_rotate_left_138();
    compute_rotate_left_138_h1();
    compute_rotate_left_140();
    compute_rotate_left_141();
    compute_rotate_left_141_h1();
    // EP node  219368:RotateLeft
    // BDD node 28:rotate_left
    // EP node  222633:ArithmeticOp
    // BDD node 244:op_xor
    // EP node  224399:ArithmeticOp
    // BDD node 245:op_add
    // EP node  226425:ArithmeticOp
    // BDD node 246:op_xor
    // EP node  228713:RotateLeft
    // BDD node 29:rotate_left
    // EP node  230755:ArithmeticOp
    // BDD node 247:op_xor
    // EP node  232549:ArithmeticOp
    // BDD node 248:op_xor
    // EP node  234607:RotateLeft
    // BDD node 30:rotate_left
    // EP node  236415:ArithmeticOp
    // BDD node 249:op_add
    // EP node  238489:RotateLeft
    // BDD node 31:rotate_left
    // EP node  241871:ArithmeticOp
    // BDD node 250:op_xor
    // EP node  243700:ArithmeticOp
    // BDD node 251:op_add
    // EP node  245798:RotateLeft
    // BDD node 32:rotate_left
    // EP node  249219:ArithmeticOp
    // BDD node 252:op_xor
    // EP node  251333:RotateLeft
    // BDD node 33:rotate_left
    // EP node  253190:ArithmeticOp
    // BDD node 253:op_add
    // EP node  255320:RotateLeft
    // BDD node 34:rotate_left
    // EP node  258793:ArithmeticOp
    // BDD node 254:op_xor
    // EP node  260671:ArithmeticOp
    // BDD node 255:op_add
    // EP node  262825:RotateLeft
    // BDD node 35:rotate_left
    // EP node  264717:ArithmeticOp
    // BDD node 256:op_xor
    // EP node  266887:RotateLeft
    // BDD node 36:rotate_left
    // EP node  268793:ArithmeticOp
    // BDD node 257:op_add
    // EP node  270979:RotateLeft
    // BDD node 37:rotate_left
    // EP node  274543:ArithmeticOp
    // BDD node 258:op_xor
    // EP node  276470:ArithmeticOp
    // BDD node 259:op_add
    // EP node  278680:RotateLeft
    // BDD node 38:rotate_left
    // EP node  282283:ArithmeticOp
    // BDD node 260:op_xor
    // EP node  284509:RotateLeft
    // BDD node 39:rotate_left
    // EP node  286464:ArithmeticOp
    // BDD node 261:op_add
    // EP node  288706:RotateLeft
    // BDD node 40:rotate_left
    // EP node  292361:ArithmeticOp
    // BDD node 262:op_xor
    // EP node  294337:ArithmeticOp
    // BDD node 263:op_add
    // EP node  296603:ArithmeticOp
    // BDD node 264:op_xor
    // EP node  298593:ArithmeticOp
    // BDD node 265:op_shl
    // EP node  300590:ArithmeticOp
    // BDD node 266:op_or
    // EP node  302880:RotateLeft
    // BDD node 41:rotate_left
    // EP node  304891:ArithmeticOp
    // BDD node 267:op_xor
    // EP node  306621:ArithmeticOp
    // BDD node 268:op_xor
    // EP node  308646:RotateLeft
    // BDD node 42:rotate_left
    // EP node  310388:ArithmeticOp
    // BDD node 269:op_add
    // EP node  312427:RotateLeft
    // BDD node 43:rotate_left
    // EP node  315641:ArithmeticOp
    // BDD node 270:op_xor
    // EP node  317401:ArithmeticOp
    // BDD node 271:op_add
    // EP node  319461:RotateLeft
    // BDD node 44:rotate_left
    // EP node  322708:ArithmeticOp
    // BDD node 272:op_xor
    // EP node  324782:RotateLeft
    // BDD node 45:rotate_left
    // EP node  326566:ArithmeticOp
    // BDD node 273:op_add
    // EP node  328654:RotateLeft
    // BDD node 46:rotate_left
    // EP node  331945:ArithmeticOp
    // BDD node 274:op_xor
    // EP node  333747:ArithmeticOp
    // BDD node 275:op_add
    // EP node  335856:RotateLeft
    // BDD node 47:rotate_left
    // EP node  337670:ArithmeticOp
    // BDD node 276:op_xor
    // EP node  339793:RotateLeft
    // BDD node 48:rotate_left
    // EP node  341619:ArithmeticOp
    // BDD node 277:op_add
    // EP node  343756:RotateLeft
    // BDD node 49:rotate_left
    // EP node  347124:ArithmeticOp
    // BDD node 278:op_xor
    // EP node  348968:ArithmeticOp
    // BDD node 279:op_add
    // EP node  351126:RotateLeft
    // BDD node 50:rotate_left
    // EP node  353290:ArithmeticOp
    // BDD node 280:op_xor
    // EP node  355462:RotateLeft
    // BDD node 51:rotate_left
    // EP node  357018:Recirculate
    // BDD node 281:op_add
    hdr.recirc.f32_0 = eg_md.rotate_left_120_x_out;
    hdr.recirc.f32_1 = eg_md.op_shl_380_a_out;
    hdr.recirc.f32_2 = eg_md.op_or_381_b_out;
    hdr.recirc.f32_3 = eg_md.op_or_381_out;
    hdr.recirc.f32_4 = eg_md.rotate_left_138_out;
    hdr.recirc.f32_5 = eg_md.rotate_left_139_out;
    hdr.recirc.f32_6 = eg_md.rotate_left_140_out;
    hdr.recirc.f32_7 = eg_md.op_add_394_out;
    hdr.recirc.f32_8 = eg_md.rotate_left_141_out;
    hdr.recirc.f32_9 = eg_md.op_xor_395_out;
    hdr.recirc.f32_10 = eg_md.rotate_left_142_out;
    // EP node  532581:ArithmeticOp
    // BDD node 317:op_add
    compute_op_add_336();
    compute_op_add_431();
    compute_op_sub_334();
    compute_rotate_left_167_x();
    compute_rotate_left_168_x();
    compute_rotate_left_167();
    compute_rotate_left_167_h1();
    compute_rotate_left_167_h2();
    compute_rotate_left_170_x();
    compute_rotate_left_172_x();
    compute_rotate_left_170();
    compute_rotate_left_170_h1();
    compute_rotate_left_173_x();
    compute_rotate_left_171();
    compute_rotate_left_173();
    compute_rotate_left_174_x();
    compute_rotate_left_174();
    compute_rotate_left_176();
    compute_rotate_left_177();
    compute_rotate_left_177_h1();
    compute_op_xor_454();
    compute_op_xor_455();
    compute_op_xor_457();
    compute_op_xor_345();
    compute_cond_operand_90_0();
    // EP node  534827:RotateLeft
    // BDD node 76:rotate_left
    // EP node  538204:ArithmeticOp
    // BDD node 318:op_xor
    // EP node  540086:ArithmeticOp
    // BDD node 319:op_add
    // EP node  542350:RotateLeft
    // BDD node 77:rotate_left
    // EP node  544242:ArithmeticOp
    // BDD node 320:op_xor
    // EP node  546518:RotateLeft
    // BDD node 78:rotate_left
    // EP node  548420:ArithmeticOp
    // BDD node 321:op_add
    // EP node  550708:RotateLeft
    // BDD node 79:rotate_left
    // EP node  554148:ArithmeticOp
    // BDD node 322:op_xor
    // EP node  556065:ArithmeticOp
    // BDD node 323:op_add
    // EP node  558371:RotateLeft
    // BDD node 80:rotate_left
    // EP node  561838:ArithmeticOp
    // BDD node 324:op_xor
    // EP node  564156:RotateLeft
    // BDD node 81:rotate_left
    // EP node  566093:ArithmeticOp
    // BDD node 325:op_add
    // EP node  568423:RotateLeft
    // BDD node 82:rotate_left
    // EP node  571926:ArithmeticOp
    // BDD node 326:op_xor
    // EP node  573878:ArithmeticOp
    // BDD node 327:op_add
    // EP node  576226:RotateLeft
    // BDD node 83:rotate_left
    // EP node  578188:ArithmeticOp
    // BDD node 328:op_xor
    // EP node  580548:RotateLeft
    // BDD node 84:rotate_left
    // EP node  582520:ArithmeticOp
    // BDD node 329:op_add
    // EP node  584892:RotateLeft
    // BDD node 85:rotate_left
    // EP node  588458:ArithmeticOp
    // BDD node 330:op_xor
    // EP node  590445:ArithmeticOp
    // BDD node 331:op_add
    // EP node  592835:RotateLeft
    // BDD node 86:rotate_left
    // EP node  596428:ArithmeticOp
    // BDD node 332:op_xor
    // EP node  598830:RotateLeft
    // BDD node 87:rotate_left
    // EP node  600837:ArithmeticOp
    // BDD node 333:op_lshr
    // EP node  602849:ArithmeticOp
    // BDD node 334:op_sub
    // EP node  604866:ArithmeticOp
    // BDD node 335:op_lshr
    // EP node  606484:ArithmeticOp
    // BDD node 336:op_add
    // EP node  607701:ArithmeticOp
    // BDD node 337:op_xor
    // EP node  608921:ArithmeticOp
    // BDD node 338:op_add
    // EP node  610144:ArithmeticOp
    // BDD node 339:op_add
    // EP node  611370:ArithmeticOp
    // BDD node 340:op_xor
    // EP node  612599:ArithmeticOp
    // BDD node 341:op_xor
    // EP node  613831:ArithmeticOp
    // BDD node 342:op_xor
    // EP node  614655:ArithmeticOp
    // BDD node 343:op_xor
    // EP node  615481:ArithmeticOp
    // BDD node 344:op_xor
    // EP node  616309:ArithmeticOp
    // BDD node 345:op_xor
    // EP node  617139:If
    // BDD node 90:if
    bool cond0 = false;
    if ((eg_md.cond_operand_90_0_out[31:8]) == (24w0x000000)){
      if ((eg_md.cond_operand_90_0_out[7:0]) <= (8w0x02)){
        cond0 = true;
      }
    }
    if (cond0) {
      // EP node  617140:Then
      // BDD node 90:if
      // EP node  641761:Ignore
      // BDD node 91:nf_set_rte_ipv4_udptcp_checksum
      // EP node  651676:ModifyHeader
      // BDD node 92:packet_return_chunk
      swap_action_92();
      hdr_val0_calc();
      hdr_val1_calc();
      @in_hash { hdr.hdr2.data1 = eg_md.hdr_val0; }
      hdr.hdr2.data3 = 8w0x50 ++ eg_md.hdr_val1;
      // EP node  659088:ModifyHeader
      // BDD node 93:packet_return_chunk
      swap_action_93();
      hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;
      // EP node  670168:Forward
      // BDD node 95:FORWARD
    } else {
      // EP node  617141:Else
      // BDD node 90:if
      // EP node  620482:Drop
      // BDD node 99:DROP
      ig_intr_dprs_md.drop_ctl = 1;
    }
    // EP node  12263:ArithmeticOp
    // BDD node 358:op_add
    compute_op_add_358();
    compute_rotate_left_119_x();
    compute_rotate_left_119();
    compute_rotate_left_120_x();
    compute_rotate_left_120();
    compute_rotate_left_120_h1();
    compute_rotate_left_122();
    compute_rotate_left_123();
    compute_rotate_left_123_h1();
    compute_rotate_left_125();
    compute_rotate_left_126();
    compute_rotate_left_126_h1();
    compute_rotate_left_128();
    compute_rotate_left_129();
    compute_rotate_left_129_h1();
    compute_rotate_left_131();
    compute_rotate_left_132_x();
    compute_rotate_left_132();
    compute_rotate_left_132_h1();
    compute_rotate_left_134();
    compute_rotate_left_135();
    compute_rotate_left_135_h1();
    compute_rotate_left_137();
    compute_rotate_left_138();
    compute_rotate_left_138_h1();
    compute_rotate_left_140();
    compute_rotate_left_141();
    compute_rotate_left_141_h1();
    // EP node  12607:RotateLeft
    // BDD node 119:rotate_left
    // EP node  13131:ArithmeticOp
    // BDD node 359:op_xor
    // EP node  13428:ArithmeticOp
    // BDD node 360:op_add
    // EP node  13790:ArithmeticOp
    // BDD node 361:op_xor
    // EP node  14219:RotateLeft
    // BDD node 120:rotate_left
    // EP node  14593:ArithmeticOp
    // BDD node 362:op_xor
    // EP node  14910:ArithmeticOp
    // BDD node 363:op_xor
    // EP node  15296:RotateLeft
    // BDD node 121:rotate_left
    // EP node  15623:ArithmeticOp
    // BDD node 364:op_add
    // EP node  16021:RotateLeft
    // BDD node 122:rotate_left
    // EP node  16626:ArithmeticOp
    // BDD node 365:op_xor
    // EP node  16968:ArithmeticOp
    // BDD node 366:op_add
    // EP node  17384:RotateLeft
    // BDD node 123:rotate_left
    // EP node  18016:ArithmeticOp
    // BDD node 367:op_xor
    // EP node  18444:RotateLeft
    // BDD node 124:rotate_left
    // EP node  18806:ArithmeticOp
    // BDD node 368:op_add
    // EP node  19246:RotateLeft
    // BDD node 125:rotate_left
    // EP node  19914:ArithmeticOp
    // BDD node 369:op_xor
    // EP node  20291:ArithmeticOp
    // BDD node 370:op_add
    // EP node  20749:RotateLeft
    // BDD node 126:rotate_left
    // EP node  21136:ArithmeticOp
    // BDD node 371:op_xor
    // EP node  21606:RotateLeft
    // BDD node 127:rotate_left
    // EP node  22003:ArithmeticOp
    // BDD node 372:op_add
    // EP node  22485:RotateLeft
    // BDD node 128:rotate_left
    // EP node  23216:ArithmeticOp
    // BDD node 373:op_xor
    // EP node  23628:ArithmeticOp
    // BDD node 374:op_add
    // EP node  24128:RotateLeft
    // BDD node 129:rotate_left
    // EP node  24886:ArithmeticOp
    // BDD node 375:op_xor
    // EP node  25398:RotateLeft
    // BDD node 130:rotate_left
    // EP node  25830:ArithmeticOp
    // BDD node 376:op_add
    // EP node  26354:RotateLeft
    // BDD node 131:rotate_left
    // EP node  27148:ArithmeticOp
    // BDD node 377:op_xor
    // EP node  27595:ArithmeticOp
    // BDD node 378:op_add
    // EP node  28137:ArithmeticOp
    // BDD node 379:op_xor
    // EP node  28594:ArithmeticOp
    // BDD node 380:op_shl
    // EP node  29056:ArithmeticOp
    // BDD node 381:op_or
    // EP node  29616:RotateLeft
    // BDD node 132:rotate_left
    // EP node  30088:ArithmeticOp
    // BDD node 382:op_xor
    // EP node  30470:ArithmeticOp
    // BDD node 383:op_xor
    // EP node  30952:RotateLeft
    // BDD node 133:rotate_left
    // EP node  31342:ArithmeticOp
    // BDD node 384:op_add
    // EP node  31834:RotateLeft
    // BDD node 134:rotate_left
    // EP node  32529:ArithmeticOp
    // BDD node 385:op_xor
    // EP node  32931:ArithmeticOp
    // BDD node 386:op_add
    // EP node  33438:RotateLeft
    // BDD node 135:rotate_left
    // EP node  34154:ArithmeticOp
    // BDD node 387:op_xor
    // EP node  34671:RotateLeft
    // BDD node 136:rotate_left
    // EP node  35089:ArithmeticOp
    // BDD node 388:op_add
    // EP node  35616:RotateLeft
    // BDD node 137:rotate_left
    // EP node  36360:ArithmeticOp
    // BDD node 389:op_xor
    // EP node  36790:ArithmeticOp
    // BDD node 390:op_add
    // EP node  37332:RotateLeft
    // BDD node 138:rotate_left
    // EP node  37770:ArithmeticOp
    // BDD node 391:op_xor
    // EP node  38322:RotateLeft
    // BDD node 139:rotate_left
    // EP node  38768:ArithmeticOp
    // BDD node 392:op_add
    // EP node  39330:RotateLeft
    // BDD node 140:rotate_left
    // EP node  40123:ArithmeticOp
    // BDD node 393:op_xor
    // EP node  40581:ArithmeticOp
    // BDD node 394:op_add
    // EP node  41158:RotateLeft
    // BDD node 141:rotate_left
    // EP node  41739:ArithmeticOp
    // BDD node 395:op_xor
    // EP node  42326:RotateLeft
    // BDD node 142:rotate_left
    // EP node  42681:Recirculate
    // BDD node 396:op_add
    @in_hash { hdr.recirc.f32_0 = eg_md.rotate_left_120_x_out; }
    hdr.recirc.f32_1 = eg_md.op_shl_380_a_out;
    hdr.recirc.f32_2 = eg_md.op_or_381_b_out;
    hdr.recirc.f32_3 = eg_md.op_or_381_out;
    @in_hash { hdr.recirc.f32_11 = eg_md.rotate_left_132_x_out; }
    hdr.recirc.f32_4 = eg_md.rotate_left_138_out;
    hdr.recirc.f32_5 = eg_md.rotate_left_139_out;
    hdr.recirc.f32_6 = eg_md.rotate_left_140_out;
    hdr.recirc.f32_7 = eg_md.op_add_394_out;
    @in_hash { hdr.recirc.f32_12 = eg_md.rotate_left_141_x_out; }
    hdr.recirc.f32_8 = eg_md.rotate_left_141_out;
    hdr.recirc.f32_9 = eg_md.op_xor_395_out;
    @in_hash { hdr.recirc.f32_13 = eg_md.rotate_left_142_x_out; }
    hdr.recirc.f32_10 = eg_md.rotate_left_142_out;
    // EP node  106731:ArithmeticOp
    // BDD node 431:op_add
    compute_op_add_431();
    compute_rotate_left_167_x();
    compute_rotate_left_168_x();
    compute_rotate_left_167();
    compute_rotate_left_167_h1();
    compute_rotate_left_167_h2();
    compute_rotate_left_170_x();
    compute_rotate_left_172_x();
    compute_rotate_left_170();
    compute_rotate_left_170_h1();
    compute_rotate_left_173_x();
    compute_rotate_left_171();
    compute_rotate_left_173();
    compute_rotate_left_174_x();
    compute_rotate_left_174();
    compute_rotate_left_176();
    compute_rotate_left_177();
    compute_rotate_left_177_h1();
    compute_op_xor_454();
    compute_op_xor_455();
    compute_op_xor_457();
    // EP node  107633:RotateLeft
    // BDD node 167:rotate_left
    // EP node  108902:ArithmeticOp
    // BDD node 432:op_xor
    // EP node  109632:ArithmeticOp
    // BDD node 433:op_add
    // EP node  110549:RotateLeft
    // BDD node 168:rotate_left
    // EP node  111287:ArithmeticOp
    // BDD node 434:op_xor
    // EP node  112214:RotateLeft
    // BDD node 169:rotate_left
    // EP node  112960:ArithmeticOp
    // BDD node 435:op_add
    // EP node  113897:RotateLeft
    // BDD node 170:rotate_left
    // EP node  115215:ArithmeticOp
    // BDD node 436:op_xor
    // EP node  115973:ArithmeticOp
    // BDD node 437:op_add
    // EP node  116925:RotateLeft
    // BDD node 171:rotate_left
    // EP node  118264:ArithmeticOp
    // BDD node 438:op_xor
    // EP node  119226:RotateLeft
    // BDD node 172:rotate_left
    // EP node  120000:ArithmeticOp
    // BDD node 439:op_add
    // EP node  120972:RotateLeft
    // BDD node 173:rotate_left
    // EP node  122339:ArithmeticOp
    // BDD node 440:op_xor
    // EP node  123125:ArithmeticOp
    // BDD node 441:op_add
    // EP node  124112:RotateLeft
    // BDD node 174:rotate_left
    // EP node  124906:ArithmeticOp
    // BDD node 442:op_xor
    // EP node  125903:RotateLeft
    // BDD node 175:rotate_left
    // EP node  126705:ArithmeticOp
    // BDD node 443:op_add
    // EP node  127712:RotateLeft
    // BDD node 176:rotate_left
    // EP node  129128:ArithmeticOp
    // BDD node 444:op_xor
    // EP node  129942:ArithmeticOp
    // BDD node 445:op_add
    // EP node  130964:RotateLeft
    // BDD node 177:rotate_left
    // EP node  132401:ArithmeticOp
    // BDD node 446:op_xor
    // EP node  133433:RotateLeft
    // BDD node 178:rotate_left
    // EP node  134263:Ignore
    // BDD node 179:nf_set_rte_ipv4_udptcp_checksum
    // EP node  135304:ArithmeticOp
    // BDD node 447:op_lshr
    // EP node  136142:ArithmeticOp
    // BDD node 448:op_sub
    // EP node  136984:ArithmeticOp
    // BDD node 449:op_lshr
    // EP node  137619:ArithmeticOp
    // BDD node 450:op_xor
    // EP node  138257:ArithmeticOp
    // BDD node 451:op_add
    // EP node  138898:ArithmeticOp
    // BDD node 452:op_add
    // EP node  139542:ArithmeticOp
    // BDD node 453:op_xor
    // EP node  140189:ArithmeticOp
    // BDD node 454:op_xor
    // EP node  140839:ArithmeticOp
    // BDD node 455:op_xor
    // EP node  141275:ArithmeticOp
    // BDD node 456:op_xor
    // EP node  141713:ArithmeticOp
    // BDD node 457:op_xor
    // EP node  142153:ModifyHeader
    // BDD node 180:packet_return_chunk
    swap_action_180();
    hdr_val2_calc();
    hdr_val3_calc();
    hdr_val4_calc();
    @in_hash { hdr.hdr2.data1 = eg_md.hdr_val2; }
    @in_hash { hdr.hdr2.data2 = eg_md.hdr_val3; }
    hdr.hdr2.data3 = 8w0x50 ++ eg_md.hdr_val4;
    // EP node  142595:ModifyHeader
    // BDD node 181:packet_return_chunk
    swap_action_181();
    hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;
    // EP node  143482:Forward
    // BDD node 183:FORWARD

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
