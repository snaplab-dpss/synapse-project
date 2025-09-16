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
  bit<32> vector_table_1074093552_141_get_value_param0;
  @padding bit<31> pad_hit0;
  bool hit0;
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
  bit<64> data3;
}
header hdr2_h {
  bit<32> data0;
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
    transition parser_137;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_138;
      default: parser_195;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr1);
    transition parser_139;
  }
  state parser_195 {
    transition reject;
  }
  state parser_139 {
    transition parser_139_0;
  }
  state parser_139_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_140;
      8w0x11: parser_140;
      default: parser_193;
    }
  }
  state parser_140 {
    pkt.extract(hdr.hdr2);
    transition parser_189;
  }
  state parser_193 {
    transition reject;
  }
  state parser_189 {
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

  bit<32> vector_table_1074093552_141_get_value_param0 = 32w0;
  action vector_table_1074093552_141_get_value(bit<32> _vector_table_1074093552_141_get_value_param0) {
    vector_table_1074093552_141_get_value_param0 = _vector_table_1074093552_141_get_value_param0;
  }

  bit<32> vector_table_1074093552_141_key0 = 32w0;
  table vector_table_1074093552_141 {
    key = {
      vector_table_1074093552_141_key0: exact;
    }
    actions = {
      vector_table_1074093552_141_get_value;
    }
    size = 36;
  }

  bit<32> map_table_1074048576_144_get_value_param0 = 32w0;
  action map_table_1074048576_144_get_value(bit<32> _map_table_1074048576_144_get_value_param0) {
    map_table_1074048576_144_get_value_param0 = _map_table_1074048576_144_get_value_param0;
  }

  bit<32> map_table_1074048576_144_key0 = 32w0;
  bit<64> map_table_1074048576_144_key1 = 64w0;
  bit<8> map_table_1074048576_144_key2 = 8w0;
  table map_table_1074048576_144 {
    key = {
      map_table_1074048576_144_key0: exact;
      map_table_1074048576_144_key1: exact;
      map_table_1074048576_144_key2: exact;
    }
    actions = {
      map_table_1074048576_144_get_value;
    }
    size = 72818;
  }

  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080976_hash_0;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080976_hash_1;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080976_hash_2;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080976_hash_3;

  bit<10> cms_1074080976_hash_0_value;
  bit<10> cms_1074080976_hash_1_value;
  bit<10> cms_1074080976_hash_2_value;
  bit<10> cms_1074080976_hash_3_value;

  Register<bit<32>,_>(1024, 0) cms_1074080976_row_0;
  Register<bit<32>,_>(1024, 0) cms_1074080976_row_1;
  Register<bit<32>,_>(1024, 0) cms_1074080976_row_2;
  Register<bit<32>,_>(1024, 0) cms_1074080976_row_3;
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_0) cms_1074080976_row_0_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_0_inc_and_read_value;
  action cms_1074080976_row_0_inc_and_read_execute() {
    cms_1074080976_row_0_inc_and_read_value = cms_1074080976_row_0_inc_and_read.execute(cms_1074080976_hash_0_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080976_row_0) cms_1074080976_row_0_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080976_row_0_inc_execute() {
    cms_1074080976_row_0_inc.execute(cms_1074080976_hash_0_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_0) cms_1074080976_row_0_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_0_read_value;
  action cms_1074080976_row_0_read_execute() {
    cms_1074080976_row_0_read_value = cms_1074080976_row_0_read.execute(cms_1074080976_hash_0_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_1) cms_1074080976_row_1_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_1_inc_and_read_value;
  action cms_1074080976_row_1_inc_and_read_execute() {
    cms_1074080976_row_1_inc_and_read_value = cms_1074080976_row_1_inc_and_read.execute(cms_1074080976_hash_1_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080976_row_1) cms_1074080976_row_1_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080976_row_1_inc_execute() {
    cms_1074080976_row_1_inc.execute(cms_1074080976_hash_1_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_1) cms_1074080976_row_1_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_1_read_value;
  action cms_1074080976_row_1_read_execute() {
    cms_1074080976_row_1_read_value = cms_1074080976_row_1_read.execute(cms_1074080976_hash_1_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_2) cms_1074080976_row_2_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_2_inc_and_read_value;
  action cms_1074080976_row_2_inc_and_read_execute() {
    cms_1074080976_row_2_inc_and_read_value = cms_1074080976_row_2_inc_and_read.execute(cms_1074080976_hash_2_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080976_row_2) cms_1074080976_row_2_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080976_row_2_inc_execute() {
    cms_1074080976_row_2_inc.execute(cms_1074080976_hash_2_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_2) cms_1074080976_row_2_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_2_read_value;
  action cms_1074080976_row_2_read_execute() {
    cms_1074080976_row_2_read_value = cms_1074080976_row_2_read.execute(cms_1074080976_hash_2_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_3) cms_1074080976_row_3_inc_and_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_3_inc_and_read_value;
  action cms_1074080976_row_3_inc_and_read_execute() {
    cms_1074080976_row_3_inc_and_read_value = cms_1074080976_row_3_inc_and_read.execute(cms_1074080976_hash_3_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080976_row_3) cms_1074080976_row_3_inc = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  action cms_1074080976_row_3_inc_execute() {
    cms_1074080976_row_3_inc.execute(cms_1074080976_hash_3_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080976_row_3) cms_1074080976_row_3_read = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080976_row_3_read_value;
  action cms_1074080976_row_3_read_execute() {
    cms_1074080976_row_3_read_value = cms_1074080976_row_3_read.execute(cms_1074080976_hash_3_value);
  }
  bit<64> cms_1074080976_key_27320 = 64w0;
  action cms_1074080976_hash_0_calc_2732() {
    cms_1074080976_hash_0_value = cms_1074080976_hash_0.get({
      cms_1074080976_key_27320,
      32w0xfbc31fc7
    });
  }
  action cms_1074080976_hash_1_calc_2732() {
    cms_1074080976_hash_1_value = cms_1074080976_hash_1.get({
      cms_1074080976_key_27320,
      32w0x2681580b
    });
  }
  action cms_1074080976_hash_2_calc_2732() {
    cms_1074080976_hash_2_value = cms_1074080976_hash_2.get({
      cms_1074080976_key_27320,
      32w0x486d7e2f
    });
  }
  action cms_1074080976_hash_3_calc_2732() {
    cms_1074080976_hash_3_value = cms_1074080976_hash_3.get({
      cms_1074080976_key_27320,
      32w0x1f3a2b4d
    });
  }
  bit<16> vector_table_1074110768_175_get_value_param0 = 16w0;
  action vector_table_1074110768_175_get_value(bit<16> _vector_table_1074110768_175_get_value_param0) {
    vector_table_1074110768_175_get_value_param0 = _vector_table_1074110768_175_get_value_param0;
  }

  bit<32> vector_table_1074110768_175_key0 = 32w0;
  table vector_table_1074110768_175 {
    key = {
      vector_table_1074110768_175_key0: exact;
    }
    actions = {
      vector_table_1074110768_175_get_value;
    }
    size = 36;
  }

  bit<32> dchain_table_1074080560_174_key0 = 32w0;
  table dchain_table_1074080560_174 {
    key = {
      dchain_table_1074080560_174_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<16> vector_table_1074110768_183_get_value_param0 = 16w0;
  action vector_table_1074110768_183_get_value(bit<16> _vector_table_1074110768_183_get_value_param0) {
    vector_table_1074110768_183_get_value_param0 = _vector_table_1074110768_183_get_value_param0;
  }

  bit<32> vector_table_1074110768_183_key0 = 32w0;
  table vector_table_1074110768_183 {
    key = {
      vector_table_1074110768_183_key0: exact;
    }
    actions = {
      vector_table_1074110768_183_get_value;
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
      // BDD node 134:expire_items_single_map(chain:(w64 1074080560), vector:(w64 1074062400), time:(Add w64 (w64 18446744073609551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074048576))
      // EP node  4:Ignore
      // BDD node 135:cms_periodic_cleanup(cms:(w64 1074080976), time:(ReadLSB w64 (w32 0) next_time))
      // EP node  11:ParserExtraction
      // BDD node 136:packet_borrow_next_chunk(p:(w64 1074213408), length:(w32 14), chunk:(w64 1074224168)[ -> (w64 1073764560)])
      if(hdr.hdr0.isValid()) {
        // EP node  24:ParserCondition
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  25:Then
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  39:ParserExtraction
        // BDD node 138:packet_borrow_next_chunk(p:(w64 1074213408), length:(w32 20), chunk:(w64 1074224904)[ -> (w64 1073764816)])
        if(hdr.hdr1.isValid()) {
          // EP node  69:ParserCondition
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  70:Then
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  114:ParserExtraction
          // BDD node 140:packet_borrow_next_chunk(p:(w64 1074213408), length:(w32 4), chunk:(w64 1074225560)[ -> (w64 1073765072)])
          if(hdr.hdr2.isValid()) {
            // EP node  173:VectorTableLookup
            // BDD node 141:vector_borrow(vector:(w64 1074093552), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074225840)[ -> (w64 1074107448)])
            vector_table_1074093552_141_key0 = meta.dev;
            vector_table_1074093552_141.apply();
            // EP node  299:Ignore
            // BDD node 142:vector_return(vector:(w64 1074093552), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107448)[(ReadLSB w32 (w32 0) vector_data_512)])
            // EP node  370:MapTableLookup
            // BDD node 144:map_get(map:(w64 1074048576), key:(w64 1074223386)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074226448)[(w32 4294967295) -> (ReadLSB w32 (w32 0) allocated_index_r6)])
            map_table_1074048576_144_key0 = hdr.hdr2.data0;
            map_table_1074048576_144_key1 = hdr.hdr1.data3;
            map_table_1074048576_144_key2 = hdr.hdr1.data1;
            bool hit0 = map_table_1074048576_144.apply().hit;
            // EP node  431:If
            // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
            if ((32w0x00000000) == (vector_table_1074093552_141_get_value_param0)){
              // EP node  432:Then
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  486:If
              // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r6))
              if (!hit0){
                // EP node  487:Then
                // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r6))
                // EP node  2732:CMSIncrement
                // BDD node 146:cms_increment(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
                cms_1074080976_hash_0_calc_2732();
                cms_1074080976_hash_1_calc_2732();
                cms_1074080976_hash_2_calc_2732();
                cms_1074080976_hash_3_calc_2732();
                cms_1074080976_row_0_inc_execute();
                cms_1074080976_row_1_inc_execute();
                cms_1074080976_row_2_inc_execute();
                cms_1074080976_row_3_inc_execute();
                // EP node  3361:SendToController
                // BDD node 147:cms_count_min(cms:(w64 1074080976), key:(w64 1074226482)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(3361);
                hdr.cpu.vector_table_1074093552_141_get_value_param0 = vector_table_1074093552_141_get_value_param0;
                hdr.cpu.hit0 = hit0;
                hdr.cpu.dev = meta.dev;
              } else {
                // EP node  488:Else
                // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r6))
                // EP node  763:VectorTableLookup
                // BDD node 175:vector_borrow(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074231752)[ -> (w64 1074124664)])
                vector_table_1074110768_175_key0 = meta.dev;
                vector_table_1074110768_175.apply();
                // EP node  1146:Ignore
                // BDD node 176:vector_return(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124664)[(ReadLSB w16 (w32 0) vector_data_r4)])
                // EP node  1430:DchainTableLookup
                // BDD node 174:dchain_rejuvenate_index(chain:(w64 1074080560), index:(ReadLSB w32 (w32 0) allocated_index_r6), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074080560_174_key0 = map_table_1074048576_144_get_value_param0;
                dchain_table_1074080560_174.apply();
                // EP node  1729:If
                // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r4)))
                if ((16w0xffff) != (vector_table_1074110768_175_get_value_param0)){
                  // EP node  1730:Then
                  // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r4)))
                  // EP node  2492:Forward
                  // BDD node 181:FORWARD
                  nf_dev[15:0] = vector_table_1074110768_175_get_value_param0;
                } else {
                  // EP node  1731:Else
                  // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r4)))
                  // EP node  4414:Drop
                  // BDD node 182:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  433:Else
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  575:VectorTableLookup
              // BDD node 183:vector_borrow(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074233160)[ -> (w64 1074124664)])
              vector_table_1074110768_183_key0 = meta.dev;
              vector_table_1074110768_183.apply();
              // EP node  946:If
              // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              if ((16w0xffff) != (vector_table_1074110768_183_get_value_param0)){
                // EP node  947:Then
                // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                // EP node  1266:Ignore
                // BDD node 184:vector_return(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124664)[(ReadLSB w16 (w32 0) vector_data_640)])
                // EP node  2310:Forward
                // BDD node 189:FORWARD
                nf_dev[15:0] = vector_table_1074110768_183_get_value_param0;
              } else {
                // EP node  948:Else
                // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                // EP node  2882:Ignore
                // BDD node 251:vector_return(vector:(w64 1074110768), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124664)[(ReadLSB w16 (w32 0) vector_data_640)])
                // EP node  4520:Drop
                // BDD node 190:DROP
                fwd_op = fwd_op_t.DROP;
              }
            }
          }
          // EP node  71:Else
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  3799:ParserReject
          // BDD node 193:DROP
        }
        // EP node  26:Else
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  3162:ParserReject
        // BDD node 195:DROP
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
