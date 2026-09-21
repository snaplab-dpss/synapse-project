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
  bit<16> data1;
  bit<16> data2;
  bit<32> data3;
  bit<16> data4;
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
}
header hdr3_h {
  bit<8> data0;
  bit<32> data1;
  bit<32> data2;
  bit<8> data3;
  bit<16> data4;
}
header hh_table_1073923096_digest_hdr {
  bit<32> data0;
  bit<32> data1;
}



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;

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
  bit<32> hh_table_1073923096_cms_min;
  bit<32> regexec_vector_register_1073954136_0_write_897_index0;
  bit<32> vector_reg_value0;
  bit<32> regexec_vector_register_1073954136_0_read_896_index0;

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
    pkt.extract(hdr.hdr0);
    transition parser_6;
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr0.data4) {
      16w0x0800: parser_7;
      default: parser_66;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr1);
    transition parser_8;
  }
  state parser_66 {
    transition reject;
  }
  state parser_8 {
    transition parser_8_0;
  }
  state parser_8_0 {
    transition select (hdr.hdr1.data2[23:0][23:16]) {
      8w0x11: parser_9;
      default: parser_64;
    }
  }
  state parser_9 {
    pkt.extract(hdr.hdr2);
    transition parser_10;
  }
  state parser_64 {
    transition reject;
  }
  state parser_10 {
    transition parser_10_0;
  }
  state parser_10_0 {
    transition select (hdr.hdr2.data0[31:16]) {
      16w0x029e: parser_11;
      default: parser_10_1;
    }
  }
  state parser_10_1 {
    transition select (hdr.hdr2.data0[15:0]) {
      16w0x029e: parser_11;
      default: parser_61;
    }
  }
  state parser_11 {
    pkt.extract(hdr.hdr3);
    transition parser_57;
  }
  state parser_61 {
    transition reject;
  }
  state parser_57 {
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

  bit<32> hh_table_1073923096_table_13_get_value_param0 = 32w0;
  action hh_table_1073923096_table_13_get_value(bit<32> _hh_table_1073923096_table_13_get_value_param0) {
    hh_table_1073923096_table_13_get_value_param0 = _hh_table_1073923096_table_13_get_value_param0;
  }

  table hh_table_1073923096_table_13 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      hh_table_1073923096_table_13_get_value;
    }
    size = 9103;
    idle_timeout = true;
  }

  Register<bit<32>,_>(8192, 0) hh_table_1073923096_cached_counters;
  RegisterAction<bit<32>, bit<32>, void>(hh_table_1073923096_cached_counters) hh_table_1073923096_cached_counters_inc_639 = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  Register<bit<8>,_>(1, 0) hh_table_1073923096_packet_sampler;
  RegisterAction<bit<8>, bit<1>, bit<8>>(hh_table_1073923096_packet_sampler) hh_table_1073923096_packet_sampler_sample_every_fourth_639 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = 0;
      if (value < 3) {
        value = value + 1;
      } else {
        value = 0;
        out_value = 1;
      }
    }
  };

  Hash<bit<16>>(HashAlgorithm_t.CRC32) hh_table_1073923096_hash_calc_0;
  CRCPolynomial<bit<32>>(32w0x7b17a39f, true, false, false, 32w0xffffffff, 32w0xffffffff) hh_table_1073923096_hash_calc_1_poly; // p1
  Hash<bit<16>>(HashAlgorithm_t.CUSTOM, hh_table_1073923096_hash_calc_1_poly) hh_table_1073923096_hash_calc_1;
  CRCPolynomial<bit<32>>(32w0x99f29aad, true, false, false, 32w0xffffffff, 32w0xffffffff) hh_table_1073923096_hash_calc_2_poly; // p2
  Hash<bit<16>>(HashAlgorithm_t.CUSTOM, hh_table_1073923096_hash_calc_2_poly) hh_table_1073923096_hash_calc_2;
  CRCPolynomial<bit<32>>(32w0x21bca2c3, true, false, false, 32w0xffffffff, 32w0xffffffff) hh_table_1073923096_hash_calc_3_poly; // p3
  Hash<bit<16>>(HashAlgorithm_t.CUSTOM, hh_table_1073923096_hash_calc_3_poly) hh_table_1073923096_hash_calc_3;

  Register<bit<32>,_>(65536, 0) hh_table_1073923096_cms_row_0;
  RegisterAction<bit<32>, bit<16>, bit<32>>(hh_table_1073923096_cms_row_0) hh_table_1073923096_cms_row_0_inc_and_read_639 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(65536, 0) hh_table_1073923096_cms_row_1;
  RegisterAction<bit<32>, bit<16>, bit<32>>(hh_table_1073923096_cms_row_1) hh_table_1073923096_cms_row_1_inc_and_read_639 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(65536, 0) hh_table_1073923096_cms_row_2;
  RegisterAction<bit<32>, bit<16>, bit<32>>(hh_table_1073923096_cms_row_2) hh_table_1073923096_cms_row_2_inc_and_read_639 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(65536, 0) hh_table_1073923096_cms_row_3;
  RegisterAction<bit<32>, bit<16>, bit<32>>(hh_table_1073923096_cms_row_3) hh_table_1073923096_cms_row_3_inc_and_read_639 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1, 0) hh_table_1073923096_threshold;
  bit<32> hh_table_1073923096_threshold_diff_639_cmp;
  RegisterAction<bit<32>, bit<1>, bit<32>>(hh_table_1073923096_threshold) hh_table_1073923096_threshold_diff_639 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = hh_table_1073923096_threshold_diff_639_cmp - value;
    }
  };

  bit<16> hh_table_1073923096_hash_calc_0_value;
  action hh_table_1073923096_hash_calc_0_calc() {
    hh_table_1073923096_hash_calc_0_value = hh_table_1073923096_hash_calc_0.get({
      meta.key_32b_0
      });
  }
  bit<16> hh_table_1073923096_hash_calc_1_value;
  action hh_table_1073923096_hash_calc_1_calc() {
    hh_table_1073923096_hash_calc_1_value = hh_table_1073923096_hash_calc_1.get({
      meta.key_32b_0
      });
  }
  bit<16> hh_table_1073923096_hash_calc_2_value;
  action hh_table_1073923096_hash_calc_2_calc() {
    hh_table_1073923096_hash_calc_2_value = hh_table_1073923096_hash_calc_2.get({
      meta.key_32b_0
      });
  }
  bit<16> hh_table_1073923096_hash_calc_3_value;
  action hh_table_1073923096_hash_calc_3_calc() {
    hh_table_1073923096_hash_calc_3_value = hh_table_1073923096_hash_calc_3.get({
      meta.key_32b_0
      });
  }
  bit<32> hh_table_1073923096_cms_row_0_value;
  action hh_table_1073923096_cms_row_0_inc_and_read_639_execute() {
    hh_table_1073923096_cms_row_0_value = hh_table_1073923096_cms_row_0_inc_and_read_639.execute(hh_table_1073923096_hash_calc_0_value);
  }
  bit<32> hh_table_1073923096_cms_row_1_value;
  action hh_table_1073923096_cms_row_1_inc_and_read_639_execute() {
    hh_table_1073923096_cms_row_1_value = hh_table_1073923096_cms_row_1_inc_and_read_639.execute(hh_table_1073923096_hash_calc_1_value);
  }
  bit<32> hh_table_1073923096_cms_row_2_value;
  action hh_table_1073923096_cms_row_2_inc_and_read_639_execute() {
    hh_table_1073923096_cms_row_2_value = hh_table_1073923096_cms_row_2_inc_and_read_639.execute(hh_table_1073923096_hash_calc_2_value);
  }
  bit<32> hh_table_1073923096_cms_row_3_value;
  action hh_table_1073923096_cms_row_3_inc_and_read_639_execute() {
    hh_table_1073923096_cms_row_3_value = hh_table_1073923096_cms_row_3_inc_and_read_639.execute(hh_table_1073923096_hash_calc_3_value);
  }
  action rewrite_18() {
  }
  action rewrite_33() {
  }
  Register<bit<32>,_>(8192, 0) vector_register_1073954136_0;

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073954136_0) vector_register_1073954136_0_write_897 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2;
    }
  };

  action regexec_vector_register_1073954136_0_write_897() {
    vector_register_1073954136_0_write_897.execute(meta.regexec_vector_register_1073954136_0_write_897_index0);
  }
  action rewrite_42() {
    hdr.hdr3.data3 = 8w0x01;
  }
  action rewrite_43() {
    hdr.hdr2.data0 = hdr.hdr2.data0[15:0] ++ hdr.hdr2.data0[31:16];
  }
  action rewrite_44() {
    swap32(hdr.hdr1.data3, hdr.hdr1.data4);
  }
  action rewrite_45() {
    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data2[15:8]);
    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data2[7:0]);
    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data3[31:24]);
    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data3[23:16]);
    swap(hdr.hdr0.data1[15:8], hdr.hdr0.data3[15:8]);
    swap(hdr.hdr0.data1[7:0], hdr.hdr0.data3[7:0]);
  }

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073954136_0) vector_register_1073954136_0_read_896 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action regexec_vector_register_1073954136_0_read_896() {
    meta.vector_reg_value0 = vector_register_1073954136_0_read_896.execute(meta.regexec_vector_register_1073954136_0_read_896_index0);
  }
  action rewrite_48() {
    hdr.hdr3.data2 = meta.vector_reg_value0;
    hdr.hdr3.data3 = 8w0x01;
  }
  action rewrite_49() {
    hdr.hdr2.data0 = hdr.hdr2.data0[15:0] ++ hdr.hdr2.data0[31:16];
  }
  action rewrite_50() {
    swap32(hdr.hdr1.data3, hdr.hdr1.data4);
  }
  action rewrite_51() {
    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data2[15:8]);
    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data2[7:0]);
    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data3[31:24]);
    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data3[23:16]);
    swap(hdr.hdr0.data1[15:8], hdr.hdr0.data3[15:8]);
    swap(hdr.hdr0.data1[7:0], hdr.hdr0.data3[7:0]);
  }

  apply {

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:Ignore
      // BDD node 4:expire_items_single_map
      // EP node  4:ParserExtraction
      // BDD node 5:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 6:if
        // EP node  14:Then
        // BDD node 6:if
        // EP node  26:ParserExtraction
        // BDD node 7:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  52:ParserCondition
          // BDD node 8:if
          // EP node  53:Then
          // BDD node 8:if
          // EP node  74:ParserExtraction
          // BDD node 9:packet_borrow_next_chunk
          if(hdr.hdr2.isValid()) {
            // EP node  117:ParserCondition
            // BDD node 10:if
            // EP node  118:Then
            // BDD node 10:if
            // EP node  148:ParserExtraction
            // BDD node 11:packet_borrow_next_chunk
            if(hdr.hdr3.isValid()) {
              // EP node  222:If
              // BDD node 12:if
              if ((16w0x0000) != (meta.dev[15:0])){
                // EP node  223:Then
                // BDD node 12:if
                // EP node  639:HHTableRead
                // BDD node 13:map_get
                meta.key_32b_0 = hdr.hdr3.data1;
                bool hit0 = hh_table_1073923096_table_13.apply().hit;
                bit<8> hh_table_1073923096_packet_sampler_sample_every_fourth_639_out_value = hh_table_1073923096_packet_sampler_sample_every_fourth_639.execute(0);
                if (hh_table_1073923096_packet_sampler_sample_every_fourth_639_out_value == 1) {
                  if (hit0) {
                    hh_table_1073923096_cached_counters_inc_639.execute(hh_table_1073923096_table_13_get_value_param0);
                  } else {
                    hh_table_1073923096_hash_calc_0_calc();
                    hh_table_1073923096_hash_calc_1_calc();
                    hh_table_1073923096_hash_calc_2_calc();
                    hh_table_1073923096_hash_calc_3_calc();
                    hh_table_1073923096_cms_row_0_inc_and_read_639_execute();
                    hh_table_1073923096_cms_row_1_inc_and_read_639_execute();
                    hh_table_1073923096_cms_row_2_inc_and_read_639_execute();
                    hh_table_1073923096_cms_row_3_inc_and_read_639_execute();
                    meta.hh_table_1073923096_cms_min = hh_table_1073923096_cms_row_0_value;
                    meta.hh_table_1073923096_cms_min = min(meta.hh_table_1073923096_cms_min, hh_table_1073923096_cms_row_1_value);
                    meta.hh_table_1073923096_cms_min = min(meta.hh_table_1073923096_cms_min, hh_table_1073923096_cms_row_2_value);
                    meta.hh_table_1073923096_cms_min = min(meta.hh_table_1073923096_cms_min, hh_table_1073923096_cms_row_3_value);
                    hh_table_1073923096_threshold_diff_639_cmp = meta.hh_table_1073923096_cms_min;
                    bit<32> hh_table_1073923096_threshold_diff = hh_table_1073923096_threshold_diff_639.execute(0);
                    if (hh_table_1073923096_threshold_diff_639_cmp[31:31] == 0) {
                      ig_dprsr_md.digest_type = 1;
                    }
                  }
                }
                // EP node  726:If
                // BDD node 14:if
                if (!hit0){
                  // EP node  727:Then
                  // BDD node 14:if
                  // EP node  1583:If
                  // BDD node 15:if
                  if ((8w0x01) == (hdr.hdr3.data0)){
                    // EP node  1584:Then
                    // BDD node 15:if
                    // EP node  3039:HHTableOutOfBandUpdate
                    // BDD node 16:dchain_allocate_new_index
                    // EP node  3269:ModifyHeader
                    // BDD node 18:packet_return_chunk
                    rewrite_18();
                    @in_hash { hdr.hdr3.data4 = meta.dev[7:0] ++ meta.dev[15:8]; }
                    // EP node  3857:Forward
                    // BDD node 22:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                  } else {
                    // EP node  1585:Else
                    // BDD node 15:if
                    // EP node  1735:ModifyHeader
                    // BDD node 33:packet_return_chunk
                    rewrite_33();
                    @in_hash { hdr.hdr3.data4 = meta.dev[7:0] ++ meta.dev[15:8]; }
                    // EP node  2227:Forward
                    // BDD node 37:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                  }
                } else {
                  // EP node  728:Else
                  // BDD node 14:if
                  // EP node  795:Ignore
                  // BDD node 38:dchain_rejuvenate_index
                  // EP node  893:If
                  // BDD node 40:if
                  if ((8w0x01) == (hdr.hdr3.data0)){
                    // EP node  894:Then
                    // BDD node 40:if
                    // EP node  897:VectorRegisterUpdate
                    // BDD node 39:vector_borrow
                    meta.regexec_vector_register_1073954136_0_write_897_index0 = hh_table_1073923096_table_13_get_value_param0;
                    regexec_vector_register_1073954136_0_write_897();
                    // EP node  2341:ModifyHeader
                    // BDD node 42:packet_return_chunk
                    rewrite_42();
                    // EP node  2509:ModifyHeader
                    // BDD node 43:packet_return_chunk
                    rewrite_43();
                    // EP node  2681:ModifyHeader
                    // BDD node 44:packet_return_chunk
                    rewrite_44();
                    // EP node  2857:ModifyHeader
                    // BDD node 45:packet_return_chunk
                    rewrite_45();
                    // EP node  2995:Forward
                    // BDD node 46:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  } else {
                    // EP node  895:Else
                    // BDD node 40:if
                    // EP node  896:VectorRegisterLookup
                    // BDD node 39:vector_borrow
                    meta.regexec_vector_register_1073954136_0_read_896_index0 = hh_table_1073923096_table_13_get_value_param0;
                    regexec_vector_register_1073954136_0_read_896();
                    // EP node  1013:ModifyHeader
                    // BDD node 48:packet_return_chunk
                    rewrite_48();
                    // EP node  1141:ModifyHeader
                    // BDD node 49:packet_return_chunk
                    rewrite_49();
                    // EP node  1273:ModifyHeader
                    // BDD node 50:packet_return_chunk
                    rewrite_50();
                    // EP node  1409:ModifyHeader
                    // BDD node 51:packet_return_chunk
                    rewrite_51();
                    // EP node  1517:Forward
                    // BDD node 52:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  }
                }
              } else {
                // EP node  224:Else
                // BDD node 12:if
                // EP node  5667:Forward
                // BDD node 57:FORWARD
                nf_dev[15:0] = bswap16(hdr.hdr3.data4);
              }
            }
            // EP node  119:Else
            // BDD node 10:if
            // EP node  5419:ParserReject
            // BDD node 61:DROP
          }
          // EP node  54:Else
          // BDD node 8:if
          // EP node  5030:ParserReject
          // BDD node 64:DROP
        }
        // EP node  15:Else
        // BDD node 6:if
        // EP node  4507:ParserReject
        // BDD node 66:DROP
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
  Digest<hh_table_1073923096_digest_hdr>() hh_table_1073923096_digest;

  apply {
    if (ig_dprsr_md.digest_type == 1) {
      hh_table_1073923096_digest.pack({
        meta.key_32b_0,
        meta.hh_table_1073923096_cms_min,
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
