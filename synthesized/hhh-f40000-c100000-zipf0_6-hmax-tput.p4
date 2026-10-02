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
  bit<16> data2;
  bit<16> data3;
  bit<32> data4;
  bit<32> data5;
}
header hdr2_h {
  bit<32> data0;
}
struct vector_register_1074072832_0_pair_t {
  bit<32> lo;
  bit<32> hi;
}

struct vector_register_1074091064_0_pair_t {
  bit<32> lo;
  bit<32> hi;
}

struct vector_register_1074109296_0_pair_t {
  bit<32> lo;
  bit<32> hi;
}

struct vector_register_1074127528_0_pair_t {
  bit<32> lo;
  bit<32> hi;
}

struct vector_register_1074145760_0_pair_t {
  bit<32> lo;
  bit<32> hi;
}

struct vector_register_1074163992_0_pair_t {
  bit<32> lo;
  bit<32> hi;
}



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

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
  bit<32> key_32b_0;
  bit<8> reg_incr_index0;
  bit<32> reg_new0;
  bit<16> reg_incr_index1;
  bit<32> reg_new1;
  bit<32> hash_164_in0;
  bit<10> hash_164_value;
  bit<32> carried_count0;
  bit<32> old_key0;
  bit<32> old_count0;
  bit<16> inc_or_swap_predicate0;
  bit<32> carried_key0;
  bit<32> carried_count1;
  bit<10> hash_166_value;
  bit<32> old_key1;
  bit<32> old_count1;
  bit<16> inc_or_swap_predicate1;
  bit<32> carried_key1;
  bit<32> carried_count2;
  bit<10> hash_168_value;
  bit<32> old_key2;
  bit<32> old_count2;
  bit<16> inc_or_swap_predicate2;
  bit<32> carried_key2;
  bit<32> carried_count3;
  bit<10> hash_170_value;
  bit<32> old_key3;
  bit<32> old_count3;
  bit<16> inc_or_swap_predicate3;
  bit<32> carried_key3;
  bit<32> carried_count4;
  bit<10> hash_172_value;
  bit<32> old_key4;
  bit<32> old_count4;
  bit<16> inc_or_swap_predicate4;
  bit<32> carried_key4;
  bit<32> carried_count5;
  bit<10> hash_174_value;
  bit<32> old_key5;
  bit<32> old_count5;
  bit<16> inc_or_swap_predicate5;
  bit<16> pkt_len;
  bit<1> pc_145;
  bit<1> pc_147;

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
    meta.pc_145 = 0;
    meta.pc_147 = 0;
    pkt.extract(hdr.hdr0);
    transition parser_145;
  }
  state parser_145 {
    transition parser_145_0;
  }
  state parser_145_0 {
    transition select (hdr.hdr0.data3) {
      16w0x0800: parser_146;
      default: parser_196;
    }
  }
  state parser_146 {
    meta.pc_145 = 1;
    pkt.extract(hdr.hdr1);
    transition parser_147;
  }
  state parser_196 {
    transition reject;
  }
  state parser_147 {
    transition parser_147_0;
  }
  state parser_147_0 {
    transition select (hdr.hdr1.data2[7:0]) {
      8w0x06: parser_148;
      8w0x11: parser_148;
      default: parser_194;
    }
  }
  state parser_148 {
    meta.pc_147 = 1;
    pkt.extract(hdr.hdr2);
    transition parser_190;
  }
  state parser_194 {
    transition reject;
  }
  state parser_190 {
    transition accept;
  }

}


@pa_solitary("ingress", "meta.pc_145")
@pa_solitary("ingress", "meta.pc_147")


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

  bit<32> vector_table_1074190072_157_get_value_param0 = 32w0;
  action vector_table_1074190072_157_get_value(bit<32> _vector_table_1074190072_157_get_value_param0) {
    vector_table_1074190072_157_get_value_param0 = _vector_table_1074190072_157_get_value_param0;
  }

  table vector_table_1074190072_157 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074190072_157_get_value;
    }
    size = 36;
  }

  Register<bit<32>,_>(256, 0) vector_register_1074038336_0;

  RegisterAction<bit<32>, bit<8>, bit<32>>(vector_register_1074038336_0) vector_register_1074038336_0_add_value_1730 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 32w0x00000001;
      out_value = value;
    }
  };

  action regexec_vector_register_1074038336_0_add_value_1730() {
    meta.reg_new0 = vector_register_1074038336_0_add_value_1730.execute(meta.reg_incr_index0);
  }
  Register<bit<32>,_>(65536, 0) vector_register_1074055552_0;

  RegisterAction<bit<32>, bit<16>, bit<32>>(vector_register_1074055552_0) vector_register_1074055552_0_add_value_2269 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 32w0x00000001;
      out_value = value;
    }
  };

  action regexec_vector_register_1074055552_0_add_value_2269() {
    meta.reg_new1 = vector_register_1074055552_0_add_value_2269.execute(meta.reg_incr_index1);
  }
  Hash<bit<10>>(HashAlgorithm_t.CRC32) hash_164;
  action hash_164_calc() {
    meta.hash_164_value = hash_164.get({
      meta.hash_164_in0
      });
  }
  Register<vector_register_1074072832_0_pair_t,_>(1024) vector_register_1074072832_0;

  RegisterAction3<vector_register_1074072832_0_pair_t, bit<10>, bit<32>, bit<32>, bit<16>>(vector_register_1074072832_0) vector_register_1074072832_0_inc_or_swap_2765 = {
    void apply(inout vector_register_1074072832_0_pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<16> predicate) {
      vector_register_1074072832_0_pair_t in_value = value;
      old_key = in_value.lo;
      old_count = in_value.hi;
      bool hit = in_value.lo == meta.hash_164_in0;
      predicate = this.predicate<bit<16>>(hit);
      if (hit) {
        value.hi = in_value.hi + meta.carried_count0;
      } else {
        value.lo = meta.hash_164_in0;
        value.hi = meta.carried_count0;
      }
    }
  };
  action regexec_vector_register_1074072832_0_inc_or_swap_2765() {
    meta.old_key0 = vector_register_1074072832_0_inc_or_swap_2765.execute(meta.hash_164_value, meta.old_count0, meta.inc_or_swap_predicate0);
  }
  CRCPolynomial<bit<32>>(32w0x7b17a39f, true, false, false, 32w0xffffffff, 32w0xffffffff) hash_166_poly; // p1
  Hash<bit<10>>(HashAlgorithm_t.CUSTOM, hash_166_poly) hash_166;
  action hash_166_calc() {
    meta.hash_166_value = hash_166.get({
      meta.carried_key0[7:0],
      meta.carried_key0[15:8],
      meta.carried_key0[23:16],
      meta.carried_key0[31:24]
      });
  }
  Register<vector_register_1074091064_0_pair_t,_>(1024) vector_register_1074091064_0;

  RegisterAction3<vector_register_1074091064_0_pair_t, bit<10>, bit<32>, bit<32>, bit<16>>(vector_register_1074091064_0) vector_register_1074091064_0_inc_or_swap_3251 = {
    void apply(inout vector_register_1074091064_0_pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<16> predicate) {
      vector_register_1074091064_0_pair_t in_value = value;
      old_key = in_value.lo;
      old_count = in_value.hi;
      bool hit = in_value.lo == meta.carried_key0;
      bool lighter = in_value.hi < meta.carried_count1;
      predicate = this.predicate<bit<16>>(hit, lighter);
      if (hit) {
        value.hi = in_value.hi + meta.carried_count1;
      } else if (lighter) {
        value.lo = meta.carried_key0;
        value.hi = meta.carried_count1;
      }
    }
  };
  action regexec_vector_register_1074091064_0_inc_or_swap_3251() {
    meta.old_key1 = vector_register_1074091064_0_inc_or_swap_3251.execute(meta.hash_166_value, meta.old_count1, meta.inc_or_swap_predicate1);
  }
  CRCPolynomial<bit<32>>(32w0x99f29aad, true, false, false, 32w0xffffffff, 32w0xffffffff) hash_168_poly; // p2
  Hash<bit<10>>(HashAlgorithm_t.CUSTOM, hash_168_poly) hash_168;
  action hash_168_calc() {
    meta.hash_168_value = hash_168.get({
      meta.carried_key1[7:0],
      meta.carried_key1[15:8],
      meta.carried_key1[23:16],
      meta.carried_key1[31:24]
      });
  }
  Register<vector_register_1074109296_0_pair_t,_>(1024) vector_register_1074109296_0;

  RegisterAction3<vector_register_1074109296_0_pair_t, bit<10>, bit<32>, bit<32>, bit<16>>(vector_register_1074109296_0) vector_register_1074109296_0_inc_or_swap_3561 = {
    void apply(inout vector_register_1074109296_0_pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<16> predicate) {
      vector_register_1074109296_0_pair_t in_value = value;
      old_key = in_value.lo;
      old_count = in_value.hi;
      bool hit = in_value.lo == meta.carried_key1;
      bool lighter = in_value.hi < meta.carried_count2;
      predicate = this.predicate<bit<16>>(hit, lighter);
      if (hit) {
        value.hi = in_value.hi + meta.carried_count2;
      } else if (lighter) {
        value.lo = meta.carried_key1;
        value.hi = meta.carried_count2;
      }
    }
  };
  action regexec_vector_register_1074109296_0_inc_or_swap_3561() {
    meta.old_key2 = vector_register_1074109296_0_inc_or_swap_3561.execute(meta.hash_168_value, meta.old_count2, meta.inc_or_swap_predicate2);
  }
  CRCPolynomial<bit<32>>(32w0x21bca2c3, true, false, false, 32w0xffffffff, 32w0xffffffff) hash_170_poly; // p3
  Hash<bit<10>>(HashAlgorithm_t.CUSTOM, hash_170_poly) hash_170;
  action hash_170_calc() {
    meta.hash_170_value = hash_170.get({
      meta.carried_key2[7:0],
      meta.carried_key2[15:8],
      meta.carried_key2[23:16],
      meta.carried_key2[31:24]
      });
  }
  Register<vector_register_1074127528_0_pair_t,_>(1024) vector_register_1074127528_0;

  RegisterAction3<vector_register_1074127528_0_pair_t, bit<10>, bit<32>, bit<32>, bit<16>>(vector_register_1074127528_0) vector_register_1074127528_0_inc_or_swap_3887 = {
    void apply(inout vector_register_1074127528_0_pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<16> predicate) {
      vector_register_1074127528_0_pair_t in_value = value;
      old_key = in_value.lo;
      old_count = in_value.hi;
      bool hit = in_value.lo == meta.carried_key2;
      bool lighter = in_value.hi < meta.carried_count3;
      predicate = this.predicate<bit<16>>(hit, lighter);
      if (hit) {
        value.hi = in_value.hi + meta.carried_count3;
      } else if (lighter) {
        value.lo = meta.carried_key2;
        value.hi = meta.carried_count3;
      }
    }
  };
  action regexec_vector_register_1074127528_0_inc_or_swap_3887() {
    meta.old_key3 = vector_register_1074127528_0_inc_or_swap_3887.execute(meta.hash_170_value, meta.old_count3, meta.inc_or_swap_predicate3);
  }
  CRCPolynomial<bit<32>>(32w0x0c596849, true, false, false, 32w0xffffffff, 32w0xffffffff) hash_172_poly; // p4
  Hash<bit<10>>(HashAlgorithm_t.CUSTOM, hash_172_poly) hash_172;
  action hash_172_calc() {
    meta.hash_172_value = hash_172.get({
      meta.carried_key3[7:0],
      meta.carried_key3[15:8],
      meta.carried_key3[23:16],
      meta.carried_key3[31:24]
      });
  }
  Register<vector_register_1074145760_0_pair_t,_>(1024) vector_register_1074145760_0;

  RegisterAction3<vector_register_1074145760_0_pair_t, bit<10>, bit<32>, bit<32>, bit<16>>(vector_register_1074145760_0) vector_register_1074145760_0_inc_or_swap_4229 = {
    void apply(inout vector_register_1074145760_0_pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<16> predicate) {
      vector_register_1074145760_0_pair_t in_value = value;
      old_key = in_value.lo;
      old_count = in_value.hi;
      bool hit = in_value.lo == meta.carried_key3;
      bool lighter = in_value.hi < meta.carried_count4;
      predicate = this.predicate<bit<16>>(hit, lighter);
      if (hit) {
        value.hi = in_value.hi + meta.carried_count4;
      } else if (lighter) {
        value.lo = meta.carried_key3;
        value.hi = meta.carried_count4;
      }
    }
  };
  action regexec_vector_register_1074145760_0_inc_or_swap_4229() {
    meta.old_key4 = vector_register_1074145760_0_inc_or_swap_4229.execute(meta.hash_172_value, meta.old_count4, meta.inc_or_swap_predicate4);
  }
  CRCPolynomial<bit<32>>(32w0x3553d717, true, false, false, 32w0xffffffff, 32w0xffffffff) hash_174_poly; // p5
  Hash<bit<10>>(HashAlgorithm_t.CUSTOM, hash_174_poly) hash_174;
  action hash_174_calc() {
    meta.hash_174_value = hash_174.get({
      meta.carried_key4[7:0],
      meta.carried_key4[15:8],
      meta.carried_key4[23:16],
      meta.carried_key4[31:24]
      });
  }
  Register<vector_register_1074163992_0_pair_t,_>(1024) vector_register_1074163992_0;

  RegisterAction3<vector_register_1074163992_0_pair_t, bit<10>, bit<32>, bit<32>, bit<16>>(vector_register_1074163992_0) vector_register_1074163992_0_inc_or_swap_4587 = {
    void apply(inout vector_register_1074163992_0_pair_t value, out bit<32> old_key, out bit<32> old_count, out bit<16> predicate) {
      vector_register_1074163992_0_pair_t in_value = value;
      old_key = in_value.lo;
      old_count = in_value.hi;
      bool hit = in_value.lo == meta.carried_key4;
      bool lighter = in_value.hi < meta.carried_count5;
      predicate = this.predicate<bit<16>>(hit, lighter);
      if (hit) {
        value.hi = in_value.hi + meta.carried_count5;
      } else if (lighter) {
        value.lo = meta.carried_key4;
        value.hi = meta.carried_count5;
      }
    }
  };
  action regexec_vector_register_1074163992_0_inc_or_swap_4587() {
    meta.old_key5 = vector_register_1074163992_0_inc_or_swap_4587.execute(meta.hash_174_value, meta.old_count5, meta.inc_or_swap_predicate5);
  }
  bit<16> vector_table_1074207288_176_get_value_param0 = 16w0;
  action vector_table_1074207288_176_get_value(bit<16> _vector_table_1074207288_176_get_value_param0) {
    vector_table_1074207288_176_get_value_param0 = _vector_table_1074207288_176_get_value_param0;
  }

  table vector_table_1074207288_176 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074207288_176_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074207288_184_get_value_param0 = 16w0;
  action vector_table_1074207288_184_get_value(bit<16> _vector_table_1074207288_184_get_value_param0) {
    vector_table_1074207288_184_get_value_param0 = _vector_table_1074207288_184_get_value_param0;
  }

  table vector_table_1074207288_184 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074207288_184_get_value;
    }
    size = 36;
  }


  apply {
    meta.pkt_len = 0;
    if (hdr.hdr1.isValid()) {
      meta.pkt_len = hdr.hdr1.data0[15:0] + 14;
    }

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:ParserExtraction
      // BDD node 144:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  5:ParserCondition
        // BDD node 145:if
        if (meta.pc_145 == 1) {
          // EP node  6:Then
          // BDD node 145:if
          // EP node  16:ParserExtraction
          // BDD node 146:packet_borrow_next_chunk
          if(hdr.hdr1.isValid()) {
            // EP node  43:ParserCondition
            // BDD node 147:if
            if (meta.pc_147 == 1) {
              // EP node  44:Then
              // BDD node 147:if
              // EP node  143:ParserExtraction
              // BDD node 148:packet_borrow_next_chunk
              if(hdr.hdr2.isValid()) {
                // EP node  254:Ignore
                // BDD node 149:vector_periodic_clear
                // EP node  368:Ignore
                // BDD node 150:vector_periodic_clear
                // EP node  482:Ignore
                // BDD node 151:vector_periodic_clear
                // EP node  594:Ignore
                // BDD node 152:vector_periodic_clear
                // EP node  702:Ignore
                // BDD node 153:vector_periodic_clear
                // EP node  804:Ignore
                // BDD node 154:vector_periodic_clear
                // EP node  898:Ignore
                // BDD node 155:vector_periodic_clear
                // EP node  982:Ignore
                // BDD node 156:vector_periodic_clear
                // EP node  1090:VectorTableLookup
                // BDD node 157:vector_borrow
                meta.key_32b_0 = meta.dev;
                vector_table_1074190072_157.apply();
                // EP node  1166:Ignore
                // BDD node 158:vector_return
                // EP node  1265:If
                // BDD node 159:if
                if ((32w0x00000000) == (vector_table_1074190072_157_get_value_param0)){
                  // EP node  1266:Then
                  // BDD node 159:if
                  // EP node  1380:Ignore
                  // BDD node 160:vector_borrow
                  // EP node  1730:VectorRegisterUpdate
                  // BDD node 161:vector_return
                  meta.reg_incr_index0 = (bit<8>)((bit<32>)(hdr.hdr1.data4[31:24]));
                  regexec_vector_register_1074038336_0_add_value_1730();
                  // EP node  1979:Ignore
                  // BDD node 162:vector_borrow
                  // EP node  2269:VectorRegisterUpdate
                  // BDD node 163:vector_return
                  meta.reg_incr_index1 = (bit<16>)((bit<32>)(hdr.hdr1.data4[23:16] ++ hdr.hdr1.data4[31:24]));
                  regexec_vector_register_1074055552_0_add_value_2269();
                  // EP node  2513:Crc32HasherHash
                  // BDD node 164:crc32_hasher_hash
                  meta.hash_164_in0 = (hdr.hdr1.data4) & (32w0xffffff00);
                  hash_164_calc();
                  // EP node  2765:VectorIncOrSwap
                  // BDD node 165:vector_inc_or_swap
                  meta.carried_count0 = 32w0x00000001;
                  regexec_vector_register_1074072832_0_inc_or_swap_2765();
                  meta.carried_key0 = meta.hash_164_in0;
                  meta.carried_count1 = meta.carried_count0;
                  if (meta.inc_or_swap_predicate0 == 2 || meta.inc_or_swap_predicate0 == 8) {
                    meta.carried_key0 = 0;
                    meta.carried_count1 = 0;
                  } else if (meta.inc_or_swap_predicate0 == 1) {
                    meta.carried_key0 = meta.old_key0;
                    meta.carried_count1 = meta.old_count0;
                  }
                  // EP node  3034:Crc32HasherHash
                  // BDD node 166:crc32_hasher_hash
                  hash_166_calc();
                  // EP node  3251:VectorIncOrSwap
                  // BDD node 167:vector_inc_or_swap
                  regexec_vector_register_1074091064_0_inc_or_swap_3251();
                  meta.carried_key1 = meta.carried_key0;
                  meta.carried_count2 = meta.carried_count1;
                  if (meta.inc_or_swap_predicate1 == 2 || meta.inc_or_swap_predicate1 == 8) {
                    meta.carried_key1 = 0;
                    meta.carried_count2 = 0;
                  } else if (meta.inc_or_swap_predicate1 == 4) {
                    meta.carried_key1 = meta.old_key1;
                    meta.carried_count2 = meta.old_count1;
                  }
                  // EP node  3404:Crc32HasherHash
                  // BDD node 168:crc32_hasher_hash
                  hash_168_calc();
                  // EP node  3561:VectorIncOrSwap
                  // BDD node 169:vector_inc_or_swap
                  regexec_vector_register_1074109296_0_inc_or_swap_3561();
                  meta.carried_key2 = meta.carried_key1;
                  meta.carried_count3 = meta.carried_count2;
                  if (meta.inc_or_swap_predicate2 == 2 || meta.inc_or_swap_predicate2 == 8) {
                    meta.carried_key2 = 0;
                    meta.carried_count3 = 0;
                  } else if (meta.inc_or_swap_predicate2 == 4) {
                    meta.carried_key2 = meta.old_key2;
                    meta.carried_count3 = meta.old_count2;
                  }
                  // EP node  3722:Crc32HasherHash
                  // BDD node 170:crc32_hasher_hash
                  hash_170_calc();
                  // EP node  3887:VectorIncOrSwap
                  // BDD node 171:vector_inc_or_swap
                  regexec_vector_register_1074127528_0_inc_or_swap_3887();
                  meta.carried_key3 = meta.carried_key2;
                  meta.carried_count4 = meta.carried_count3;
                  if (meta.inc_or_swap_predicate3 == 2 || meta.inc_or_swap_predicate3 == 8) {
                    meta.carried_key3 = 0;
                    meta.carried_count4 = 0;
                  } else if (meta.inc_or_swap_predicate3 == 4) {
                    meta.carried_key3 = meta.old_key3;
                    meta.carried_count4 = meta.old_count3;
                  }
                  // EP node  4056:Crc32HasherHash
                  // BDD node 172:crc32_hasher_hash
                  hash_172_calc();
                  // EP node  4229:VectorIncOrSwap
                  // BDD node 173:vector_inc_or_swap
                  regexec_vector_register_1074145760_0_inc_or_swap_4229();
                  meta.carried_key4 = meta.carried_key3;
                  meta.carried_count5 = meta.carried_count4;
                  if (meta.inc_or_swap_predicate4 == 2 || meta.inc_or_swap_predicate4 == 8) {
                    meta.carried_key4 = 0;
                    meta.carried_count5 = 0;
                  } else if (meta.inc_or_swap_predicate4 == 4) {
                    meta.carried_key4 = meta.old_key4;
                    meta.carried_count5 = meta.old_count4;
                  }
                  // EP node  4406:Crc32HasherHash
                  // BDD node 174:crc32_hasher_hash
                  hash_174_calc();
                  // EP node  4587:VectorIncOrSwap
                  // BDD node 175:vector_inc_or_swap
                  regexec_vector_register_1074163992_0_inc_or_swap_4587();
                  // EP node  4728:VectorTableLookup
                  // BDD node 176:vector_borrow
                  meta.key_32b_0 = meta.dev;
                  vector_table_1074207288_176.apply();
                  // EP node  4918:Ignore
                  // BDD node 177:vector_return
                  // EP node  5743:If
                  // BDD node 181:if
                  if ((meta.dev[15:0]) != (vector_table_1074207288_176_get_value_param0)){
                    // EP node  5744:Then
                    // BDD node 181:if
                    // EP node  5900:Forward
                    // BDD node 182:FORWARD
                    nf_dev[15:0] = vector_table_1074207288_176_get_value_param0;
                  } else {
                    // EP node  5745:Else
                    // BDD node 181:if
                    // EP node  6413:Drop
                    // BDD node 183:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  1267:Else
                  // BDD node 159:if
                  // EP node  1577:VectorTableLookup
                  // BDD node 184:vector_borrow
                  meta.key_32b_0 = meta.dev;
                  vector_table_1074207288_184.apply();
                  // EP node  1840:Ignore
                  // BDD node 185:vector_return
                  // EP node  2925:If
                  // BDD node 189:if
                  if ((meta.dev[15:0]) != (vector_table_1074207288_184_get_value_param0)){
                    // EP node  2926:Then
                    // BDD node 189:if
                    // EP node  3179:Forward
                    // BDD node 190:FORWARD
                    nf_dev[15:0] = vector_table_1074207288_184_get_value_param0;
                  } else {
                    // EP node  2927:Else
                    // BDD node 189:if
                    // EP node  6309:Drop
                    // BDD node 191:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                }
              }
            } else {
              // EP node  45:Else
              // BDD node 147:if
              // EP node  6790:ParserReject
              // BDD node 194:DROP
            }
          }
        } else {
          // EP node  7:Else
          // BDD node 145:if
          // EP node  6519:ParserReject
          // BDD node 196:DROP
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
