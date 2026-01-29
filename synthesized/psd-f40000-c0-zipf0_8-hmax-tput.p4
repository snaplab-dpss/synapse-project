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
  DONE    = 0x04,
}

enum bit<2> fwd_op_t {
  FORWARD_NF_DEV  = 0,
  FORWARD_TO_CPU  = 1,
  RECIRCULATE     = 2,
  DROP            = 3,
}

header cpu_h {
  bit<16> code_path;                  // Written by the data plane
  bit<16> egress_dev;                 // Written by the control plane
  bit<8> trigger_dataplane_execution; // Written by the control plane
  @padding bit<31> pad_hit0;
  bool hit0;
  bit<32> vector_table_1074109560_142_get_value_param0;
  bit<32> dev;
  bit<32> bf_1074096984_estimate0;
  bit<16> vector_table_1074126776_174_get_value_param0;
  bit<32> vector_reg_value0;
  bit<32> map_table_1074048392_145_get_value_param0;

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
  bit<96> data0;
  bit<16> data1;
}
header hdr1_h {
  bit<72> data0;
  bit<8> data1;
  bit<16> data2;
  bit<64> data3;
}
header hdr2_h {
  bit<16> data0;
  bit<16> data1;
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
  bit<32> key_32b_0;
  bit<16> key_16b_1;

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
    transition parser_138;
  }
  state parser_138 {
    transition parser_138_0;
  }
  state parser_138_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_139;
      default: parser_202;
    }
  }
  state parser_139 {
    pkt.extract(hdr.hdr1);
    transition parser_140;
  }
  state parser_202 {
    transition reject;
  }
  state parser_140 {
    transition parser_140_0;
  }
  state parser_140_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_141;
      8w0x11: parser_141;
      default: parser_200;
    }
  }
  state parser_141 {
    pkt.extract(hdr.hdr2);
    transition parser_197;
  }
  state parser_200 {
    transition reject;
  }
  state parser_197 {
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

  bit<32> map_table_1074048392_145_get_value_param0 = 32w0;
  action map_table_1074048392_145_get_value(bit<32> _map_table_1074048392_145_get_value_param0) {
    map_table_1074048392_145_get_value_param0 = _map_table_1074048392_145_get_value_param0;
  }

  table map_table_1074048392_145 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      map_table_1074048392_145_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<32> vector_table_1074109560_142_get_value_param0 = 32w0;
  action vector_table_1074109560_142_get_value(bit<32> _vector_table_1074109560_142_get_value_param0) {
    vector_table_1074109560_142_get_value_param0 = _vector_table_1074109560_142_get_value_param0;
  }

  table vector_table_1074109560_142 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074109560_142_get_value;
    }
    size = 36;
  }

  table dchain_table_1074096568_167 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<32>,_>(65536, 0) vector_register_1074079432_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074079432_0) vector_register_1074079432_0_read_768 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  Register<bit<32>,_>(1024, 0) bf_1074096984_row_0;
  Register<bit<32>,_>(1024, 0) bf_1074096984_row_1;
  Register<bit<32>,_>(1024, 0) bf_1074096984_row_2;
  Register<bit<32>,_>(1024, 0) bf_1074096984_row_3;

  bit<10> bf_1074096984_hash_0_value;
  bit<10> bf_1074096984_hash_1_value;
  bit<10> bf_1074096984_hash_2_value;
  bit<10> bf_1074096984_hash_3_value;

  RegisterAction<bit<32>, bit<10>, void>(bf_1074096984_row_0) bf_1074096984_row_0_set_to_one = {
    void apply(inout bit<32> value) {
      value = 1;
    }
  };

  action bf_1074096984_row_0_set_to_one_execute() {
    bf_1074096984_row_0_set_to_one.execute(bf_1074096984_hash_0_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_0) bf_1074096984_row_0_read_and_set = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
      value = 1;
    }
  };

  bit<32> bf_1074096984_row_0_read_and_set_value;
  action bf_1074096984_row_0_read_and_set_execute() {
    bf_1074096984_row_0_read_and_set_value = bf_1074096984_row_0_read_and_set.execute(bf_1074096984_hash_0_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_0) bf_1074096984_row_0_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> bf_1074096984_row_0_read_value;
  action bf_1074096984_row_0_read_execute() {
    bf_1074096984_row_0_read_value = bf_1074096984_row_0_read.execute(bf_1074096984_hash_0_value);
  }

  RegisterAction<bit<32>, bit<10>, void>(bf_1074096984_row_1) bf_1074096984_row_1_set_to_one = {
    void apply(inout bit<32> value) {
      value = 1;
    }
  };

  action bf_1074096984_row_1_set_to_one_execute() {
    bf_1074096984_row_1_set_to_one.execute(bf_1074096984_hash_1_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_1) bf_1074096984_row_1_read_and_set = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
      value = 1;
    }
  };

  bit<32> bf_1074096984_row_1_read_and_set_value;
  action bf_1074096984_row_1_read_and_set_execute() {
    bf_1074096984_row_1_read_and_set_value = bf_1074096984_row_1_read_and_set.execute(bf_1074096984_hash_1_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_1) bf_1074096984_row_1_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> bf_1074096984_row_1_read_value;
  action bf_1074096984_row_1_read_execute() {
    bf_1074096984_row_1_read_value = bf_1074096984_row_1_read.execute(bf_1074096984_hash_1_value);
  }

  RegisterAction<bit<32>, bit<10>, void>(bf_1074096984_row_2) bf_1074096984_row_2_set_to_one = {
    void apply(inout bit<32> value) {
      value = 1;
    }
  };

  action bf_1074096984_row_2_set_to_one_execute() {
    bf_1074096984_row_2_set_to_one.execute(bf_1074096984_hash_2_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_2) bf_1074096984_row_2_read_and_set = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
      value = 1;
    }
  };

  bit<32> bf_1074096984_row_2_read_and_set_value;
  action bf_1074096984_row_2_read_and_set_execute() {
    bf_1074096984_row_2_read_and_set_value = bf_1074096984_row_2_read_and_set.execute(bf_1074096984_hash_2_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_2) bf_1074096984_row_2_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> bf_1074096984_row_2_read_value;
  action bf_1074096984_row_2_read_execute() {
    bf_1074096984_row_2_read_value = bf_1074096984_row_2_read.execute(bf_1074096984_hash_2_value);
  }

  RegisterAction<bit<32>, bit<10>, void>(bf_1074096984_row_3) bf_1074096984_row_3_set_to_one = {
    void apply(inout bit<32> value) {
      value = 1;
    }
  };

  action bf_1074096984_row_3_set_to_one_execute() {
    bf_1074096984_row_3_set_to_one.execute(bf_1074096984_hash_3_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_3) bf_1074096984_row_3_read_and_set = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
      value = 1;
    }
  };

  bit<32> bf_1074096984_row_3_read_and_set_value;
  action bf_1074096984_row_3_read_and_set_execute() {
    bf_1074096984_row_3_read_and_set_value = bf_1074096984_row_3_read_and_set.execute(bf_1074096984_hash_3_value);
  }

  RegisterAction<bit<32>, bit<10>, bit<32>>(bf_1074096984_row_3) bf_1074096984_row_3_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> bf_1074096984_row_3_read_value;
  action bf_1074096984_row_3_read_execute() {
    bf_1074096984_row_3_read_value = bf_1074096984_row_3_read.execute(bf_1074096984_hash_3_value);
  }

  Hash<bit<10>>(HashAlgorithm_t.CRC32) bf_1074096984_hash_0_961;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) bf_1074096984_hash_1_961;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) bf_1074096984_hash_2_961;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) bf_1074096984_hash_3_961;

  action bf_1074096984_hash_0_961_calc_961() {
    bf_1074096984_hash_0_value = bf_1074096984_hash_0_961.get({
      meta.key_32b_0,
      meta.key_16b_1,
      32w0xfbc31fc7
    });
  }
  action bf_1074096984_hash_1_961_calc_961() {
    bf_1074096984_hash_1_value = bf_1074096984_hash_1_961.get({
      meta.key_32b_0,
      meta.key_16b_1,
      32w0x2681580b
    });
  }
  action bf_1074096984_hash_2_961_calc_961() {
    bf_1074096984_hash_2_value = bf_1074096984_hash_2_961.get({
      meta.key_32b_0,
      meta.key_16b_1,
      32w0x486d7e2f
    });
  }
  action bf_1074096984_hash_3_961_calc_961() {
    bf_1074096984_hash_3_value = bf_1074096984_hash_3_961.get({
      meta.key_32b_0,
      meta.key_16b_1,
      32w0x1f3a2b4d
    });
  }
  bit<16> vector_table_1074126776_174_get_value_param0 = 16w0;
  action vector_table_1074126776_174_get_value(bit<16> _vector_table_1074126776_174_get_value_param0) {
    vector_table_1074126776_174_get_value_param0 = _vector_table_1074126776_174_get_value_param0;
  }

  table vector_table_1074126776_174 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074126776_174_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074126776_186_get_value_param0 = 16w0;
  action vector_table_1074126776_186_get_value(bit<16> _vector_table_1074126776_186_get_value_param0) {
    vector_table_1074126776_186_get_value_param0 = _vector_table_1074126776_186_get_value_param0;
  }

  table vector_table_1074126776_186 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074126776_186_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074126776_192_get_value_param0 = 16w0;
  action vector_table_1074126776_192_get_value(bit<16> _vector_table_1074126776_192_get_value_param0) {
    vector_table_1074126776_192_get_value_param0 = _vector_table_1074126776_192_get_value_param0;
  }

  table vector_table_1074126776_192 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074126776_192_get_value;
    }
    size = 36;
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:Ignore
      // BDD node 135:expire_items_single_map
      // EP node  4:Ignore
      // BDD node 136:bf_periodic_cleanup
      // EP node  11:ParserExtraction
      // BDD node 137:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  24:ParserCondition
        // BDD node 138:if
        // EP node  25:Then
        // BDD node 138:if
        // EP node  39:ParserExtraction
        // BDD node 139:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  69:MapTableLookup
          // BDD node 145:map_get
          meta.key_32b_0 = hdr.hdr1.data3[63:32];
          bool hit0 = map_table_1074048392_145.apply().hit;
          // EP node  112:ParserCondition
          // BDD node 140:if
          // EP node  113:Then
          // BDD node 140:if
          // EP node  149:ParserExtraction
          // BDD node 141:packet_borrow_next_chunk
          if(hdr.hdr2.isValid()) {
            // EP node  201:VectorTableLookup
            // BDD node 142:vector_borrow
            meta.key_32b_0 = meta.dev;
            vector_table_1074109560_142.apply();
            // EP node  244:Ignore
            // BDD node 143:vector_return
            // EP node  290:If
            // BDD node 144:if
            if ((32w0x00000000) == (vector_table_1074109560_142_get_value_param0)){
              // EP node  291:Then
              // BDD node 144:if
              // EP node  343:If
              // BDD node 146:if
              if (!hit0){
                // EP node  344:Then
                // BDD node 146:if
                // EP node  2407:SendToController
                // BDD node 147:dchain_allocate_new_index
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(2407);
                hdr.cpu.hit0 = hit0;
                hdr.cpu.vector_table_1074109560_142_get_value_param0 = vector_table_1074109560_142_get_value_param0;
                hdr.cpu.dev = meta.dev;
              } else {
                // EP node  345:Else
                // BDD node 146:if
                // EP node  587:DchainTableLookup
                // BDD node 167:dchain_rejuvenate_index
                meta.key_32b_0 = map_table_1074048392_145_get_value_param0;
                dchain_table_1074096568_167.apply();
                // EP node  768:VectorRegisterLookup
                // BDD node 168:vector_borrow
                bit<32> vector_reg_value0 = vector_register_1074079432_0_read_768.execute(map_table_1074048392_145_get_value_param0);
                // EP node  961:BloomFilterQueryAndSet
                // BDD node 169:bf_query
                meta.key_32b_0 = hdr.hdr1.data3[63:32];
                meta.key_16b_1 = hdr.hdr2.data1;
                bf_1074096984_hash_0_961_calc_961();
                bf_1074096984_hash_1_961_calc_961();
                bf_1074096984_hash_2_961_calc_961();
                bf_1074096984_hash_3_961_calc_961();
                bf_1074096984_row_0_read_and_set_execute();
                bf_1074096984_row_1_read_and_set_execute();
                bf_1074096984_row_2_read_and_set_execute();
                bf_1074096984_row_3_read_and_set_execute();
                bit<32> bf_1074096984_estimate0 = 0;
                bf_1074096984_estimate0[0:0] = bf_1074096984_row_0_read_and_set_value[0:0];
                bf_1074096984_estimate0[1:1] = bf_1074096984_row_1_read_and_set_value[0:0];
                bf_1074096984_estimate0[2:2] = bf_1074096984_row_2_read_and_set_value[0:0];
                bf_1074096984_estimate0[3:3] = bf_1074096984_row_3_read_and_set_value[0:0];
                // EP node  1135:If
                // BDD node 171:if
                if ((32w0x00000000) == (bf_1074096984_estimate0)){
                  // EP node  1136:Then
                  // BDD node 171:if
                  // EP node  2456:If
                  // BDD node 172:if
                  if ((vector_reg_value0) <= (32w0x0000000f)){
                    // EP node  2457:Then
                    // BDD node 172:if
                    // EP node  3058:VectorTableLookup
                    // BDD node 174:vector_borrow
                    meta.key_32b_0 = meta.dev;
                    vector_table_1074126776_174.apply();
                    // EP node  3655:SendToController
                    // BDD node 173:vector_return
                    fwd_op = fwd_op_t.FORWARD_TO_CPU;
                    build_cpu_hdr(3655);
                    hdr.cpu.bf_1074096984_estimate0 = bf_1074096984_estimate0;
                    hdr.cpu.vector_table_1074126776_174_get_value_param0 = vector_table_1074126776_174_get_value_param0;
                    hdr.cpu.vector_reg_value0 = vector_reg_value0;
                    hdr.cpu.vector_table_1074109560_142_get_value_param0 = vector_table_1074109560_142_get_value_param0;
                    hdr.cpu.dev = meta.dev;
                    hdr.cpu.hit0 = hit0;
                    hdr.cpu.map_table_1074048392_145_get_value_param0 = map_table_1074048392_145_get_value_param0;
                  } else {
                    // EP node  2458:Else
                    // BDD node 172:if
                    // EP node  3236:Ignore
                    // BDD node 180:vector_return
                    // EP node  4562:Drop
                    // BDD node 184:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  1137:Else
                  // BDD node 171:if
                  // EP node  1353:Ignore
                  // BDD node 185:vector_return
                  // EP node  1519:VectorTableLookup
                  // BDD node 186:vector_borrow
                  meta.key_32b_0 = meta.dev;
                  vector_table_1074126776_186.apply();
                  // EP node  1630:Ignore
                  // BDD node 187:vector_return
                  // EP node  2121:Forward
                  // BDD node 191:FORWARD
                  nf_dev[15:0] = vector_table_1074126776_186_get_value_param0;
                }
              }
            } else {
              // EP node  292:Else
              // BDD node 144:if
              // EP node  473:VectorTableLookup
              // BDD node 192:vector_borrow
              meta.key_32b_0 = meta.dev;
              vector_table_1074126776_192.apply();
              // EP node  667:Ignore
              // BDD node 193:vector_return
              // EP node  1487:Forward
              // BDD node 197:FORWARD
              nf_dev[15:0] = vector_table_1074126776_192_get_value_param0;
            }
          }
          // EP node  114:Else
          // BDD node 140:if
          // EP node  3470:ParserReject
          // BDD node 200:DROP
        }
        // EP node  26:Else
        // BDD node 138:if
        // EP node  2672:ParserReject
        // BDD node 202:DROP
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
