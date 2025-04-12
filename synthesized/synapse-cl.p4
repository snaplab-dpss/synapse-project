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

header cpu_h {
  bit<16> code_path;                  // Written by the data plane
  bit<16> egress_dev;                 // Written by the control plane
  bit<8> trigger_dataplane_execution; // Written by the control plane
  bit<32> vector_table_1074092880_141_get_value_param0;
  @padding bit<7> pad_hit0;
  bool hit0;
  bit<32> cms_1074080304_min0;
  bit<16> vector_reg_value0;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;

};

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
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

}

struct synapse_ingress_metadata_t {
  bit<32> dev;
  bit<32> time;
  bool recirculate;

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
      1 : parse_resubmit;
      0 : parse_port_metadata;
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

    meta.dev = 0;
    meta.time = ig_intr_md.ingress_mac_tstamp[47:16];
    meta.recirculate = false;
    
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
  action drop() { ig_dprsr_md.drop_ctl = 1; }
  action fwd(bit<16> port) { ig_tm_md.ucast_egress_port = port[8:0]; }

  action send_to_controller(bit<16> code_path) {
    hdr.cpu.setValid();
    hdr.cpu.code_path = code_path;
    fwd(CPU_PCIE_PORT);
  }

  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

  #define bswap32(x) (x[7:0] ++ x[15:8] ++ x[23:16] ++ x[31:24])
  #define bswap16(x) (x[7:0] ++ x[15:8])

  action set_ingress_dev(bit<32> nf_dev) { meta.dev = nf_dev; }
  table ingress_port_to_nf_dev {
    key = { ig_intr_md.ingress_port: exact; }
    actions = { set_ingress_dev; drop; }
    size = 32;
    const default_action = drop();
  }

  bool trigger_forward = false;
  bit<32> nf_dev = 0;
  table forward_nf_dev {
    key = { nf_dev: exact; }
    actions = { fwd; }
    size = 64;
  }

  Register<bit<16>,_>(32, 0) vector_register_1074110096_0;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074110096_0) vector_register_1074110096_0_read_17 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  bit<32> vector_table_1074092880_141_get_value_param0 = 32w0;
  action vector_table_1074092880_141_get_value(bit<32> _vector_table_1074092880_141_get_value_param0) {
    vector_table_1074092880_141_get_value_param0 = _vector_table_1074092880_141_get_value_param0;
  }

  bit<32> vector_table_1074092880_141_key0 = 32w0;
  table vector_table_1074092880_141 {
    key = {
      vector_table_1074092880_141_key0: exact;
    }
    actions = {
      vector_table_1074092880_141_get_value;
    }
    size = 32;
  }

  bit<32> guarded_map_table_1074047904_144_get_value_param0 = 32w0;
  action guarded_map_table_1074047904_144_get_value(bit<32> _guarded_map_table_1074047904_144_get_value_param0) {
    guarded_map_table_1074047904_144_get_value_param0 = _guarded_map_table_1074047904_144_get_value_param0;
  }

  bit<32> guarded_map_table_1074047904_144_key0 = 32w0;
  bit<64> guarded_map_table_1074047904_144_key1 = 64w0;
  bit<8> guarded_map_table_1074047904_144_key2 = 8w0;
  table guarded_map_table_1074047904_144 {
    key = {
      guarded_map_table_1074047904_144_key0: exact;
      guarded_map_table_1074047904_144_key1: exact;
      guarded_map_table_1074047904_144_key2: exact;
    }
    actions = {
      guarded_map_table_1074047904_144_get_value;
    }
    size = 65536;
    idle_timeout = true;
  }

  Register<bit<8>,_>(1, 0) guarded_map_table_1074047904_guard;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080304_hash_0;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080304_hash_1;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080304_hash_2;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) cms_1074080304_hash_3;

  bit<10> cms_1074080304_hash_0_value;
  bit<10> cms_1074080304_hash_1_value;
  bit<10> cms_1074080304_hash_2_value;
  bit<10> cms_1074080304_hash_3_value;

  Register<bit<32>,_>(1024, 0) cms_1074080304_row_0;
  Register<bit<32>,_>(1024, 0) cms_1074080304_row_1;
  Register<bit<32>,_>(1024, 0) cms_1074080304_row_2;
  Register<bit<32>,_>(1024, 0) cms_1074080304_row_3;
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_0) cms_1074080304_row_0_inc_and_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_0_inc_and_read_value;
  action cms_1074080304_row_0_inc_and_read_execute() {
    cms_1074080304_row_0_inc_and_read_value = cms_1074080304_row_0_inc_and_read.execute(cms_1074080304_hash_0_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080304_row_0) cms_1074080304_row_0_inc_execute = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  bit<32> cms_1074080304_row_0_inc_value;
  action cms_1074080304_row_0_inc_execute() {
    cms_1074080304_row_0_inc_value = cms_1074080304_row_0_inc.execute(cms_1074080304_hash_0_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_0) cms_1074080304_row_0_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_0_read_value;
  action cms_1074080304_row_0_read_execute() {
    cms_1074080304_row_0_read_value = cms_1074080304_row_0_read.execute(cms_1074080304_hash_0_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_1) cms_1074080304_row_1_inc_and_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_1_inc_and_read_value;
  action cms_1074080304_row_1_inc_and_read_execute() {
    cms_1074080304_row_1_inc_and_read_value = cms_1074080304_row_1_inc_and_read.execute(cms_1074080304_hash_1_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080304_row_1) cms_1074080304_row_1_inc_execute = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  bit<32> cms_1074080304_row_1_inc_value;
  action cms_1074080304_row_1_inc_execute() {
    cms_1074080304_row_1_inc_value = cms_1074080304_row_1_inc.execute(cms_1074080304_hash_1_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_1) cms_1074080304_row_1_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_1_read_value;
  action cms_1074080304_row_1_read_execute() {
    cms_1074080304_row_1_read_value = cms_1074080304_row_1_read.execute(cms_1074080304_hash_1_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_2) cms_1074080304_row_2_inc_and_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_2_inc_and_read_value;
  action cms_1074080304_row_2_inc_and_read_execute() {
    cms_1074080304_row_2_inc_and_read_value = cms_1074080304_row_2_inc_and_read.execute(cms_1074080304_hash_2_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080304_row_2) cms_1074080304_row_2_inc_execute = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  bit<32> cms_1074080304_row_2_inc_value;
  action cms_1074080304_row_2_inc_execute() {
    cms_1074080304_row_2_inc_value = cms_1074080304_row_2_inc.execute(cms_1074080304_hash_2_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_2) cms_1074080304_row_2_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_2_read_value;
  action cms_1074080304_row_2_read_execute() {
    cms_1074080304_row_2_read_value = cms_1074080304_row_2_read.execute(cms_1074080304_hash_2_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_3) cms_1074080304_row_3_inc_and_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_3_inc_and_read_value;
  action cms_1074080304_row_3_inc_and_read_execute() {
    cms_1074080304_row_3_inc_and_read_value = cms_1074080304_row_3_inc_and_read.execute(cms_1074080304_hash_3_value);
  }
  RegisterAction<bit<32>, bit<10>, void>(cms_1074080304_row_3) cms_1074080304_row_3_inc_execute = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  bit<32> cms_1074080304_row_3_inc_value;
  action cms_1074080304_row_3_inc_execute() {
    cms_1074080304_row_3_inc_value = cms_1074080304_row_3_inc.execute(cms_1074080304_hash_3_value);
  }
  RegisterAction<bit<32>, bit<10>, bit<32>>(cms_1074080304_row_3) cms_1074080304_row_3_read_execute = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> cms_1074080304_row_3_read_value;
  action cms_1074080304_row_3_read_execute() {
    cms_1074080304_row_3_read_value = cms_1074080304_row_3_read.execute(cms_1074080304_hash_3_value);
  }
  bit<64> cms_1074080304_key_39550 = 64w0;
  action cms_1074080304_hash_0_calc_3955() {
    cms_1074080304_hash_0_value = cms_1074080304_hash_0.get({
      cms_1074080304_key_39550,
      32w0xfbc31fc7
    });
  }
  action cms_1074080304_hash_1_calc_3955() {
    cms_1074080304_hash_1_value = cms_1074080304_hash_1.get({
      cms_1074080304_key_39550,
      32w0x2681580b
    });
  }
  action cms_1074080304_hash_2_calc_3955() {
    cms_1074080304_hash_2_value = cms_1074080304_hash_2.get({
      cms_1074080304_key_39550,
      32w0x486d7e2f
    });
  }
  action cms_1074080304_hash_3_calc_3955() {
    cms_1074080304_hash_3_value = cms_1074080304_hash_3.get({
      cms_1074080304_key_39550,
      32w0x1f3a2b4d
    });
  }
  RegisterAction<bit<8>, bit<1>, bit<8>>(guarded_map_table_1074047904_guard) guarded_map_table_1074047904_guard_read_4592 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
    }
  };

  bit<8> guarded_map_table_1074047904_guard_value_1490 = 0;
  action guarded_map_table_1074047904_guard_check_149() {
    guarded_map_table_1074047904_guard_value_1490 = guarded_map_table_1074047904_guard_read_4592.execute(0);
  }
  bit<32> dchain_table_1074079888_174_key0 = 32w0;
  table dchain_table_1074079888_174 {
    key = {
      dchain_table_1074079888_174_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 65536;
    idle_timeout = true;
  }


  apply {
    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:Ignore
      // BDD node 134:expire_items_single_map(chain:(w64 1074079888), vector:(w64 1074061728), time:(Add w64 (w64 18446744073609551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074047904))
      // EP node  17:VectorRegisterLookup
      // BDD node 183:vector_borrow(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074232488)[ -> (w64 1074123992)])
      bit<16> vector_reg_value0 = vector_register_1074110096_0_read_17.execute(meta.dev);
      // EP node  51:Ignore
      // BDD node 135:cms_periodic_cleanup(cms:(w64 1074080304), time:(ReadLSB w64 (w32 0) next_time))
      // EP node  97:ParserExtraction
      // BDD node 136:packet_borrow_next_chunk(p:(w64 1074212736), length:(w32 14), chunk:(w64 1074223496)[ -> (w64 1073763888)])
      if(hdr.hdr0.isValid()) {
        // EP node  161:ParserCondition
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  162:Then
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  244:ParserExtraction
        // BDD node 138:packet_borrow_next_chunk(p:(w64 1074212736), length:(w32 20), chunk:(w64 1074224232)[ -> (w64 1073764144)])
        if(hdr.hdr1.isValid()) {
          // EP node  395:ParserCondition
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  396:Then
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  578:ParserExtraction
          // BDD node 140:packet_borrow_next_chunk(p:(w64 1074212736), length:(w32 4), chunk:(w64 1074224888)[ -> (w64 1073764400)])
          if(hdr.hdr2.isValid()) {
            // EP node  658:VectorTableLookup
            // BDD node 141:vector_borrow(vector:(w64 1074092880), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074225168)[ -> (w64 1074106776)])
            vector_table_1074092880_141_key0 = meta.dev;
            vector_table_1074092880_141.apply();
            // EP node  840:Ignore
            // BDD node 142:vector_return(vector:(w64 1074092880), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074106776)[(ReadLSB w32 (w32 0) vector_data_512)])
            // EP node  933:If
            // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
            if ((32w0x00000000) == (vector_table_1074092880_141_get_value_param0)) {
              // EP node  934:Then
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  2037:GuardedMapTableLookup
              // BDD node 144:map_get(map:(w64 1074047904), key:(w64 1074222714)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074225776)[(w32 4294967295) -> (ReadLSB w32 (w32 0) allocated_index)])
              guarded_map_table_1074047904_144_key0 = hdr.hdr2.data0;
              guarded_map_table_1074047904_144_key1 = hdr.hdr1.data3;
              guarded_map_table_1074047904_144_key2 = hdr.hdr1.data1;
              bool hit0 = guarded_map_table_1074047904_144.apply().hit;
              // EP node  2212:If
              // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0) {
                // EP node  2213:Then
                // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  3955:CMSIncAndQuery
                // BDD node 146:cms_increment(cms:(w64 1074080304), key:(w64 1074225810)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
                cms_1074080304_hash_0_calc_3955();
                cms_1074080304_hash_1_calc_3955();
                cms_1074080304_hash_2_calc_3955();
                cms_1074080304_hash_3_calc_3955();
                cms_1074080304_row_0_inc_and_read_execute();
                cms_1074080304_row_1_inc_and_read_execute();
                cms_1074080304_row_2_inc_and_read_execute();
                cms_1074080304_row_3_inc_and_read_execute();
                bit<32> cms_1074080304_min0 = cms_1074080304_row_0_read_value;
                cms_1074080304_min0 = min(cms_1074080304_min0, cms_1074080304_row_1_read_value);
                cms_1074080304_min0 = min(cms_1074080304_min0, cms_1074080304_row_2_read_value);
                cms_1074080304_min0 = min(cms_1074080304_min0, cms_1074080304_row_3_read_value);
                // EP node  4354:If
                // BDD node 148:if ((Sle (ReadLSB w32 (w32 0) min_estimate) (w32 64))
                if ((cms_1074080304_min0) <= (32w0x00000040)) {
                  // EP node  4355:Then
                  // BDD node 148:if ((Sle (ReadLSB w32 (w32 0) min_estimate) (w32 64))
                  // EP node  4592:GuardedMapTableGuardCheck
                  // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  guarded_map_table_1074047904_guard_check_149();
                  bool guarded_map_table_1074047904_guard_allow0 = false;
                  if (guarded_map_table_1074047904_guard_value_1490 != 0) {
                    guarded_map_table_1074047904_guard_allow0 = true;
                  }
                  // EP node  4593:If
                  // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  if (guarded_map_table_1074047904_guard_allow0) {
                    // EP node  4594:Then
                    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    // EP node  4694:SendToController
                    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    send_to_controller(4694);
                    hdr.cpu.vector_table_1074092880_141_get_value_param0 = vector_table_1074092880_141_get_value_param0;
                    hdr.cpu.hit0 = hit0;
                    hdr.cpu.cms_1074080304_min0 = cms_1074080304_min0;
                    hdr.cpu.vector_reg_value0 = vector_reg_value0;
                    hdr.cpu.dev = meta.dev;
                  } else {
                    // EP node  4595:Else
                    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    // EP node  7433:Ignore
                    // BDD node 260:vector_return(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074123992)[(ReadLSB w16 (w32 0) vector_data_r2)])
                    // EP node  13824:If
                    // BDD node 264:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                    if ((16w0xffff) != (vector_reg_value0)) {
                      // EP node  13825:Then
                      // BDD node 264:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                      // EP node  15568:Forward
                      // BDD node 265:FORWARD
                      nf_dev[15:0] = vector_reg_value0;
                      trigger_forward = true;
                    } else {
                      // EP node  13826:Else
                      // BDD node 264:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                      // EP node  15943:Drop
                      // BDD node 266:DROP
                      drop();
                    }
                  }
                } else {
                  // EP node  4356:Else
                  // BDD node 148:if ((Sle (ReadLSB w32 (w32 0) min_estimate) (w32 64))
                  // EP node  13382:Drop
                  // BDD node 173:DROP
                  drop();
                }
              } else {
                // EP node  2214:Else
                // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  2520:DchainTableLookup
                // BDD node 174:dchain_rejuvenate_index(chain:(w64 1074079888), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074079888_174_key0 = guarded_map_table_1074047904_144_get_value_param0;
                dchain_table_1074079888_174.apply();
                // EP node  2727:If
                // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                if ((16w0xffff) != (vector_reg_value0)) {
                  // EP node  2728:Then
                  // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                  // EP node  2929:Ignore
                  // BDD node 176:vector_return(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074123992)[(ReadLSB w16 (w32 0) vector_data_r2)])
                  // EP node  3743:Forward
                  // BDD node 181:FORWARD
                  nf_dev[15:0] = vector_reg_value0;
                  trigger_forward = true;
                } else {
                  // EP node  2729:Else
                  // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                  // EP node  6616:Ignore
                  // BDD node 252:vector_return(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074123992)[(ReadLSB w16 (w32 0) vector_data_r2)])
                  // EP node  14260:Drop
                  // BDD node 182:DROP
                  drop();
                }
              }
            } else {
              // EP node  935:Else
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  1048:Ignore
              // BDD node 184:vector_return(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074123992)[(ReadLSB w16 (w32 0) vector_data_r2)])
              // EP node  1617:If
              // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
              if ((16w0xffff) != (vector_reg_value0)) {
                // EP node  1618:Then
                // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                // EP node  1890:Forward
                // BDD node 189:FORWARD
                nf_dev[15:0] = vector_reg_value0;
                trigger_forward = true;
              } else {
                // EP node  1619:Else
                // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                // EP node  8565:Drop
                // BDD node 190:DROP
                drop();
              }
            }
          }
          // EP node  397:Else
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  10606:ParserReject
          // BDD node 193:DROP
        }
        // EP node  163:Else
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  7827:ParserReject
        // BDD node 195:DROP
      }

    }

    if (trigger_forward) {
      forward_nf_dev.apply();
    }

    if (!meta.recirculate) {
      hdr.recirc.setInvalid();
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
