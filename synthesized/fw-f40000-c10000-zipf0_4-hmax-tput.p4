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
  bit<32> cached_insert_success0;
  bit<32> dev;

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
  bit<32> data3;
  bit<32> data4;
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
  bit<32> fcfs_cs_1074044080_key_32b_0;
  bit<32> fcfs_cs_1074044080_key_32b_1;
  bit<16> fcfs_cs_1074044080_key_16b_2;
  bit<16> fcfs_cs_1074044080_key_16b_3;

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
    transition parser_135;
  }
  state parser_135 {
    transition parser_135_0;
  }
  state parser_135_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_136;
      default: parser_185;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_185 {
    transition reject;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_138;
      8w0x11: parser_138;
      default: parser_183;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_147;
  }
  state parser_183 {
    transition reject;
  }
  state parser_147 {
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

  bit<32> vector_table_1074076488_139_get_value_param0 = 32w0;
  action vector_table_1074076488_139_get_value(bit<32> _vector_table_1074076488_139_get_value_param0) {
    vector_table_1074076488_139_get_value_param0 = _vector_table_1074076488_139_get_value_param0;
  }

  table vector_table_1074076488_139 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074076488_139_get_value;
    }
    size = 36;
  }

  Hash<bit<13>>(HashAlgorithm_t.CRC32) fcfs_cs_1074044080_hash_142;
  Hash<bit<13>>(HashAlgorithm_t.CRC32) fcfs_cs_1074044080_hash_155;
  Register<bit<32>,_>(8192, 0) fcfs_cs_1074044080_reg_liveness;
  RegisterAction<bit<32>, bit<13>, bool>(fcfs_cs_1074044080_reg_liveness) fcfs_cs_1074044080_reg_liveness_query_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
      } else {
        was_alive = true;
      }
    }
  };

  RegisterAction<bit<32>, bit<13>, bool>(fcfs_cs_1074044080_reg_liveness) fcfs_cs_1074044080_reg_liveness_query_and_refresh_timestamp = {
    void apply(inout bit<32> alarm, out bool was_alive) {
      if (meta.time > alarm) {
        was_alive = false;
      } else {
        was_alive = true;
      }
      alarm = meta.time + 16384;
    }
  };

  Register<bit<32>,_>(8192, 0) fcfs_cs_1074044080_reg_key_0;
  RegisterAction<bit<32>, bit<13>, void>(fcfs_cs_1074044080_reg_key_0) fcfs_cs_1074044080_reg_key_0_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_cs_1074044080_key_32b_0;
    }
  };

  RegisterAction<bit<32>, bit<13>, bit<8>>(fcfs_cs_1074044080_reg_key_0) fcfs_cs_1074044080_reg_key_0_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_cs_1074044080_key_32b_0) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<32>,_>(8192, 0) fcfs_cs_1074044080_reg_key_1;
  RegisterAction<bit<32>, bit<13>, void>(fcfs_cs_1074044080_reg_key_1) fcfs_cs_1074044080_reg_key_1_write = {
    void apply(inout bit<32> value) {
      value = meta.fcfs_cs_1074044080_key_32b_1;
    }
  };

  RegisterAction<bit<32>, bit<13>, bit<8>>(fcfs_cs_1074044080_reg_key_1) fcfs_cs_1074044080_reg_key_1_check_value = {
    void apply(inout bit<32> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_cs_1074044080_key_32b_1) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<16>,_>(8192, 0) fcfs_cs_1074044080_reg_key_2;
  RegisterAction<bit<16>, bit<13>, void>(fcfs_cs_1074044080_reg_key_2) fcfs_cs_1074044080_reg_key_2_write = {
    void apply(inout bit<16> value) {
      value = meta.fcfs_cs_1074044080_key_16b_2;
    }
  };

  RegisterAction<bit<16>, bit<13>, bit<8>>(fcfs_cs_1074044080_reg_key_2) fcfs_cs_1074044080_reg_key_2_check_value = {
    void apply(inout bit<16> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_cs_1074044080_key_16b_2) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  Register<bit<16>,_>(8192, 0) fcfs_cs_1074044080_reg_key_3;
  RegisterAction<bit<16>, bit<13>, void>(fcfs_cs_1074044080_reg_key_3) fcfs_cs_1074044080_reg_key_3_write = {
    void apply(inout bit<16> value) {
      value = meta.fcfs_cs_1074044080_key_16b_3;
    }
  };

  RegisterAction<bit<16>, bit<13>, bit<8>>(fcfs_cs_1074044080_reg_key_3) fcfs_cs_1074044080_reg_key_3_check_value = {
    void apply(inout bit<16> curr_value, out bit<8> match) {
      if (curr_value == meta.fcfs_cs_1074044080_key_16b_3) {
        match = 1;
      } else {
        match = 0;
      }
    }
  };

  table fcfs_cs_1074044080_table_142 {
    key = {
      meta.fcfs_cs_1074044080_key_32b_0: exact;
      meta.fcfs_cs_1074044080_key_32b_1: exact;
      meta.fcfs_cs_1074044080_key_16b_2: exact;
      meta.fcfs_cs_1074044080_key_16b_3: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<13> fcfs_cs_1074044080_hash_142_value;
  action fcfs_cs_1074044080_hash_142_calc() {
    fcfs_cs_1074044080_hash_142_value = fcfs_cs_1074044080_hash_142.get({
      meta.fcfs_cs_1074044080_key_32b_0,
      meta.fcfs_cs_1074044080_key_32b_1,
      meta.fcfs_cs_1074044080_key_16b_2,
      meta.fcfs_cs_1074044080_key_16b_3
      });
  }
  bit<8> match_counter0 = 0;
  action fcfs_cs_1074044080_check_key_0_142() {
    match_counter0 = match_counter0 + fcfs_cs_1074044080_reg_key_0_check_value.execute(fcfs_cs_1074044080_hash_142_value);
  }
  action fcfs_cs_1074044080_check_key_1_142() {
    match_counter0 = match_counter0 + fcfs_cs_1074044080_reg_key_1_check_value.execute(fcfs_cs_1074044080_hash_142_value);
  }
  action fcfs_cs_1074044080_check_key_2_142() {
    match_counter0 = match_counter0 + fcfs_cs_1074044080_reg_key_2_check_value.execute(fcfs_cs_1074044080_hash_142_value);
  }
  action fcfs_cs_1074044080_check_key_3_142() {
    match_counter0 = match_counter0 + fcfs_cs_1074044080_reg_key_3_check_value.execute(fcfs_cs_1074044080_hash_142_value);
  }
  bit<16> vector_table_1074093704_149_get_value_param0 = 16w0;
  action vector_table_1074093704_149_get_value(bit<16> _vector_table_1074093704_149_get_value_param0) {
    vector_table_1074093704_149_get_value_param0 = _vector_table_1074093704_149_get_value_param0;
  }

  table vector_table_1074093704_149 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074093704_149_get_value;
    }
    size = 36;
  }

  table fcfs_cs_1074044080_table_155 {
    key = {
      meta.fcfs_cs_1074044080_key_32b_0: exact;
      meta.fcfs_cs_1074044080_key_32b_1: exact;
      meta.fcfs_cs_1074044080_key_16b_2: exact;
      meta.fcfs_cs_1074044080_key_16b_3: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<13> fcfs_cs_1074044080_hash_155_value;
  action fcfs_cs_1074044080_hash_155_calc() {
    fcfs_cs_1074044080_hash_155_value = fcfs_cs_1074044080_hash_155.get({
      meta.fcfs_cs_1074044080_key_32b_0,
      meta.fcfs_cs_1074044080_key_32b_1,
      meta.fcfs_cs_1074044080_key_16b_2,
      meta.fcfs_cs_1074044080_key_16b_3
      });
  }
  bit<8> match_counter1 = 0;
  action fcfs_cs_1074044080_check_key_0_155() {
    match_counter1 = match_counter1 + fcfs_cs_1074044080_reg_key_0_check_value.execute(fcfs_cs_1074044080_hash_155_value);
  }
  action fcfs_cs_1074044080_check_key_1_155() {
    match_counter1 = match_counter1 + fcfs_cs_1074044080_reg_key_1_check_value.execute(fcfs_cs_1074044080_hash_155_value);
  }
  action fcfs_cs_1074044080_check_key_2_155() {
    match_counter1 = match_counter1 + fcfs_cs_1074044080_reg_key_2_check_value.execute(fcfs_cs_1074044080_hash_155_value);
  }
  action fcfs_cs_1074044080_check_key_3_155() {
    match_counter1 = match_counter1 + fcfs_cs_1074044080_reg_key_3_check_value.execute(fcfs_cs_1074044080_hash_155_value);
  }
  bit<16> vector_table_1074093704_175_get_value_param0 = 16w0;
  action vector_table_1074093704_175_get_value(bit<16> _vector_table_1074093704_175_get_value_param0) {
    vector_table_1074093704_175_get_value_param0 = _vector_table_1074093704_175_get_value_param0;
  }

  table vector_table_1074093704_175 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074093704_175_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074093704_168_get_value_param0 = 16w0;
  action vector_table_1074093704_168_get_value(bit<16> _vector_table_1074093704_168_get_value_param0) {
    vector_table_1074093704_168_get_value_param0 = _vector_table_1074093704_168_get_value_param0;
  }

  table vector_table_1074093704_168 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074093704_168_get_value;
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
      // BDD node 133:expire_items_single_map
      // EP node  4:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 135:if
        // EP node  14:Then
        // BDD node 135:if
        // EP node  26:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  52:ParserCondition
          // BDD node 137:if
          // EP node  53:Then
          // BDD node 137:if
          // EP node  83:ParserExtraction
          // BDD node 138:packet_borrow_next_chunk
          if(hdr.hdr2.isValid()) {
            // EP node  169:VectorTableLookup
            // BDD node 139:vector_borrow
            meta.key_32b_0 = meta.dev;
            vector_table_1074076488_139.apply();
            // EP node  228:Ignore
            // BDD node 140:vector_return
            // EP node  292:If
            // BDD node 141:if
            if ((32w0x00000000) == (vector_table_1074076488_139_get_value_param0)){
              // EP node  293:Then
              // BDD node 141:if
              // EP node  562:FCFSCachedSetRead
              // BDD node 142:map_get
              meta.fcfs_cs_1074044080_key_32b_0 = hdr.hdr1.data4;
              meta.fcfs_cs_1074044080_key_32b_1 = hdr.hdr1.data3;
              meta.fcfs_cs_1074044080_key_16b_2 = hdr.hdr2.data1;
              meta.fcfs_cs_1074044080_key_16b_3 = hdr.hdr2.data0;
              bool hit0 = fcfs_cs_1074044080_table_142.apply().hit;
              fcfs_cs_1074044080_hash_142_calc();
              bool fcfs_cs_is_alive0 = fcfs_cs_1074044080_reg_liveness_query_timestamp.execute(fcfs_cs_1074044080_hash_142_value);
              if (!hit0 && fcfs_cs_is_alive0) {
                fcfs_cs_1074044080_check_key_0_142();
                fcfs_cs_1074044080_check_key_1_142();
                fcfs_cs_1074044080_check_key_2_142();
                fcfs_cs_1074044080_check_key_3_142();
                if (match_counter0 == 4) {
                  hit0 = true;
                }
              }
              // EP node  1157:If
              // BDD node 143:if
              if (!hit0){
                // EP node  1158:Then
                // BDD node 143:if
                // EP node  5187:Drop
                // BDD node 147:DROP
                fwd_op = fwd_op_t.DROP;
              } else {
                // EP node  1159:Else
                // BDD node 143:if
                // EP node  1268:Ignore
                // BDD node 148:dchain_rejuvenate_index
                // EP node  1391:VectorTableLookup
                // BDD node 149:vector_borrow
                meta.key_32b_0 = meta.dev;
                vector_table_1074093704_149.apply();
                // EP node  1489:Ignore
                // BDD node 150:vector_return
                // EP node  1925:Forward
                // BDD node 154:FORWARD
                nf_dev[15:0] = vector_table_1074093704_149_get_value_param0;
              }
            } else {
              // EP node  294:Else
              // BDD node 141:if
              // EP node  1053:FCFSCachedSetReadInsert
              // BDD node 155:map_get
              meta.fcfs_cs_1074044080_key_32b_0 = hdr.hdr1.data3;
              meta.fcfs_cs_1074044080_key_32b_1 = hdr.hdr1.data4;
              meta.fcfs_cs_1074044080_key_16b_2 = hdr.hdr2.data0;
              meta.fcfs_cs_1074044080_key_16b_3 = hdr.hdr2.data1;
              bool hit1 = fcfs_cs_1074044080_table_155.apply().hit;
              bit<32> cached_insert_success0 = 0;
              if (!hit1) {
                fcfs_cs_1074044080_hash_155_calc();
                bool fcfs_cs_is_alive1 = fcfs_cs_1074044080_reg_liveness_query_and_refresh_timestamp.execute(fcfs_cs_1074044080_hash_155_value);
                if (fcfs_cs_is_alive1) {
                  fcfs_cs_1074044080_check_key_0_155();
                  fcfs_cs_1074044080_check_key_1_155();
                  fcfs_cs_1074044080_check_key_2_155();
                  fcfs_cs_1074044080_check_key_3_155();
                  if (match_counter1 == 4) {
                    hit1 = true;
                  }
                } else {
                  fcfs_cs_1074044080_reg_key_0_write.execute(fcfs_cs_1074044080_hash_155_value);
                  fcfs_cs_1074044080_reg_key_1_write.execute(fcfs_cs_1074044080_hash_155_value);
                  fcfs_cs_1074044080_reg_key_2_write.execute(fcfs_cs_1074044080_hash_155_value);
                  fcfs_cs_1074044080_reg_key_3_write.execute(fcfs_cs_1074044080_hash_155_value);
                  cached_insert_success0 = 1;
                }
              }
              // EP node  1054:If
              // BDD node 155:map_get
              if (hit1){
                // EP node  1055:Then
                // BDD node 155:map_get
                // EP node  1987:Ignore
                // BDD node 174:dchain_rejuvenate_index
                // EP node  2094:VectorTableLookup
                // BDD node 175:vector_borrow
                meta.key_32b_0 = meta.dev;
                vector_table_1074093704_175.apply();
                // EP node  2204:Ignore
                // BDD node 176:vector_return
                // EP node  2692:Forward
                // BDD node 180:FORWARD
                nf_dev[15:0] = vector_table_1074093704_175_get_value_param0;
              } else {
                // EP node  1056:Else
                // BDD node 155:map_get
                // EP node  1057:If
                // BDD node 155:map_get
                if ((cached_insert_success0) != (32w0x00000000)){
                  // EP node  1058:Then
                  // BDD node 155:map_get
                  // EP node  2833:VectorTableLookup
                  // BDD node 168:vector_borrow
                  meta.key_32b_0 = meta.dev;
                  vector_table_1074093704_168.apply();
                  // EP node  3010:Ignore
                  // BDD node 169:vector_return
                  // EP node  3787:Forward
                  // BDD node 173:FORWARD
                  nf_dev[15:0] = vector_table_1074093704_168_get_value_param0;
                } else {
                  // EP node  1059:Else
                  // BDD node 155:map_get
                  // EP node  2779:SendToController
                  // BDD node 263:tofino_force_send_to_controller
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(2779);
                  hdr.cpu.cached_insert_success0 = cached_insert_success0;
                  hdr.cpu.dev = meta.dev;
                }
              }
            }
          }
          // EP node  54:Else
          // BDD node 137:if
          // EP node  4885:ParserReject
          // BDD node 183:DROP
        }
        // EP node  15:Else
        // BDD node 135:if
        // EP node  4416:ParserReject
        // BDD node 185:DROP
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
