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
  bit<32> vector_table_1074119656_142_get_value_param0;
  @padding bit<31> pad_hit0;
  bool hit0;
  bit<32> dev;
  @padding bit<31> pad_hit1;
  bool hit1;
  bit<16> vector_table_1074136872_183_get_value_param0;
  bit<32> vector_reg_value0;

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
  bit<16> data0;
  bit<16> data1;
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
    transition parser_138;
  }
  state parser_138 {
    transition parser_138_0;
  }
  state parser_138_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_139;
      default: parser_217;
    }
  }
  state parser_139 {
    pkt.extract(hdr.hdr1);
    transition parser_140;
  }
  state parser_217 {
    transition reject;
  }
  state parser_140 {
    transition parser_140_0;
  }
  state parser_140_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_141;
      8w0x11: parser_141;
      default: parser_215;
    }
  }
  state parser_141 {
    pkt.extract(hdr.hdr2);
    transition parser_211;
  }
  state parser_215 {
    transition reject;
  }
  state parser_211 {
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

  action fwd_nf_dev(bit<16> port) {
    hdr.recirc.setInvalid();
    ig_tm_md.ucast_egress_port = port[8:0];
  }

  bool trigger_forward = false;
  bit<32> nf_dev = 0;
  table forward_nf_dev {
    key = { nf_dev: exact; }
    actions = { fwd_nf_dev; }
    size = 64;
  }

  bit<32> guarded_map_table_1074040056_145_get_value_param0 = 32w0;
  action guarded_map_table_1074040056_145_get_value(bit<32> _guarded_map_table_1074040056_145_get_value_param0) {
    guarded_map_table_1074040056_145_get_value_param0 = _guarded_map_table_1074040056_145_get_value_param0;
  }

  bit<32> guarded_map_table_1074040056_145_key0 = 32w0;
  table guarded_map_table_1074040056_145 {
    key = {
      guarded_map_table_1074040056_145_key0: exact;
    }
    actions = {
      guarded_map_table_1074040056_145_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<8>,_>(1, 0) guarded_map_table_1074040056_guard;
  bit<32> vector_table_1074119656_142_get_value_param0 = 32w0;
  action vector_table_1074119656_142_get_value(bit<32> _vector_table_1074119656_142_get_value_param0) {
    vector_table_1074119656_142_get_value_param0 = _vector_table_1074119656_142_get_value_param0;
  }

  bit<32> vector_table_1074119656_142_key0 = 32w0;
  table vector_table_1074119656_142 {
    key = {
      vector_table_1074119656_142_key0: exact;
    }
    actions = {
      vector_table_1074119656_142_get_value;
    }
    size = 36;
  }

  RegisterAction<bit<8>, bit<1>, bit<8>>(guarded_map_table_1074040056_guard) guarded_map_table_1074040056_guard_read_19613 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
    }
  };

  bit<8> guarded_map_table_1074040056_guard_value_1470 = 0;
  action guarded_map_table_1074040056_guard_check_147() {
    guarded_map_table_1074040056_guard_value_1470 = guarded_map_table_1074040056_guard_read_19613.execute(0);
  }
  bit<16> vector_table_1074136872_315_get_value_param0 = 16w0;
  action vector_table_1074136872_315_get_value(bit<16> _vector_table_1074136872_315_get_value_param0) {
    vector_table_1074136872_315_get_value_param0 = _vector_table_1074136872_315_get_value_param0;
  }

  bit<32> vector_table_1074136872_315_key0 = 32w0;
  table vector_table_1074136872_315 {
    key = {
      vector_table_1074136872_315_key0: exact;
    }
    actions = {
      vector_table_1074136872_315_get_value;
    }
    size = 36;
  }

  bit<32> dchain_table_1074088232_174_key0 = 32w0;
  table dchain_table_1074088232_174 {
    key = {
      dchain_table_1074088232_174_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<32>,_>(65536, 0) vector_register_1074071096_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074071096_0) vector_register_1074071096_0_read_2654 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  bit<32> guarded_map_table_1074088616_176_get_value_param0 = 32w0;
  action guarded_map_table_1074088616_176_get_value(bit<32> _guarded_map_table_1074088616_176_get_value_param0) {
    guarded_map_table_1074088616_176_get_value_param0 = _guarded_map_table_1074088616_176_get_value_param0;
  }

  bit<32> guarded_map_table_1074088616_176_key0 = 32w0;
  bit<16> guarded_map_table_1074088616_176_key1 = 16w0;
  table guarded_map_table_1074088616_176 {
    key = {
      guarded_map_table_1074088616_176_key0: exact;
      guarded_map_table_1074088616_176_key1: exact;
    }
    actions = {
      guarded_map_table_1074088616_176_get_value;
    }
    size = 1165085;
  }

  Register<bit<8>,_>(1, 0) guarded_map_table_1074088616_guard;
  bit<16> vector_table_1074136872_183_get_value_param0 = 16w0;
  action vector_table_1074136872_183_get_value(bit<16> _vector_table_1074136872_183_get_value_param0) {
    vector_table_1074136872_183_get_value_param0 = _vector_table_1074136872_183_get_value_param0;
  }

  bit<32> vector_table_1074136872_183_key0 = 32w0;
  table vector_table_1074136872_183 {
    key = {
      vector_table_1074136872_183_key0: exact;
    }
    actions = {
      vector_table_1074136872_183_get_value;
    }
    size = 36;
  }


  RegisterAction<bit<32>, bit<32>, void>(vector_register_1074071096_0) vector_register_1074071096_0_write_9575 = {
    void apply(inout bit<32> value) {
      value = reg_write0;
    }
  };

  bit<16> vector_table_1074136872_197_get_value_param0 = 16w0;
  action vector_table_1074136872_197_get_value(bit<16> _vector_table_1074136872_197_get_value_param0) {
    vector_table_1074136872_197_get_value_param0 = _vector_table_1074136872_197_get_value_param0;
  }

  bit<32> vector_table_1074136872_197_key0 = 32w0;
  table vector_table_1074136872_197 {
    key = {
      vector_table_1074136872_197_key0: exact;
    }
    actions = {
      vector_table_1074136872_197_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074136872_205_get_value_param0 = 16w0;
  action vector_table_1074136872_205_get_value(bit<16> _vector_table_1074136872_205_get_value_param0) {
    vector_table_1074136872_205_get_value_param0 = _vector_table_1074136872_205_get_value_param0;
  }

  bit<32> vector_table_1074136872_205_key0 = 32w0;
  table vector_table_1074136872_205 {
    key = {
      vector_table_1074136872_205_key0: exact;
    }
    actions = {
      vector_table_1074136872_205_get_value;
    }
    size = 36;
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
      // BDD node 136:expire_items_single_map(chain:(w64 1074088232), vector:(w64 1074053880), time:(Add w64 (w64 18446744073609551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074040056))
      // EP node  4:ParserExtraction
      // BDD node 137:packet_borrow_next_chunk(p:(w64 1074251448), length:(w32 14), chunk:(w64 1074262112)[ -> (w64 1073760944)])
      if(hdr.hdr0.isValid()) {
        // EP node  12:ParserCondition
        // BDD node 138:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  13:Then
        // BDD node 138:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  22:ParserExtraction
        // BDD node 139:packet_borrow_next_chunk(p:(w64 1074251448), length:(w32 20), chunk:(w64 1074262848)[ -> (w64 1073761200)])
        if(hdr.hdr1.isValid()) {
          // EP node  50:GuardedMapTableLookup
          // BDD node 145:map_get(map:(w64 1074040056), key:(w64 1074264332)[(ReadLSB w32 (w32 268) packet_chunks) -> (ReadLSB w32 (w32 268) packet_chunks)], value_out:(w64 1074264392)[(w32 4294967295) -> (ReadLSB w32 (w32 0) allocated_index_r1)])
          guarded_map_table_1074040056_145_key0 = hdr.hdr1.data3[63:32];
          bool hit0 = guarded_map_table_1074040056_145.apply().hit;
          // EP node  107:ParserCondition
          // BDD node 140:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  108:Then
          // BDD node 140:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  178:VectorTableLookup
          // BDD node 142:vector_borrow(vector:(w64 1074119656), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074263784)[ -> (w64 1074133552)])
          vector_table_1074119656_142_key0 = meta.dev;
          vector_table_1074119656_142.apply();
          // EP node  404:ParserExtraction
          // BDD node 141:packet_borrow_next_chunk(p:(w64 1074251448), length:(w32 4), chunk:(w64 1074263504)[ -> (w64 1073761456)])
          if(hdr.hdr2.isValid()) {
            // EP node  550:Ignore
            // BDD node 143:vector_return(vector:(w64 1074119656), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074133552)[(ReadLSB w32 (w32 0) vector_data_r1)])
            // EP node  638:If
            // BDD node 144:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
            if ((32w0x00000000) == (vector_table_1074119656_142_get_value_param0)) {
              // EP node  639:Then
              // BDD node 144:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  2166:If
              // BDD node 146:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r1))
              if (!hit0) {
                // EP node  2167:Then
                // BDD node 146:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r1))
                // EP node  19613:GuardedMapTableGuardCheck
                // BDD node 147:dchain_allocate_new_index(chain:(w64 1074088232), index_out:(w64 1074264888)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                guarded_map_table_1074040056_guard_check_147();
                bool guarded_map_table_1074040056_guard_allow0 = false;
                if (guarded_map_table_1074040056_guard_value_1470 != 0) {
                  guarded_map_table_1074040056_guard_allow0 = true;
                }
                // EP node  19614:If
                // BDD node 147:dchain_allocate_new_index(chain:(w64 1074088232), index_out:(w64 1074264888)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                if (guarded_map_table_1074040056_guard_allow0) {
                  // EP node  19615:Then
                  // BDD node 147:dchain_allocate_new_index(chain:(w64 1074088232), index_out:(w64 1074264888)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  21418:SendToController
                  // BDD node 147:dchain_allocate_new_index(chain:(w64 1074088232), index_out:(w64 1074264888)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  send_to_controller(21418);
                  hdr.cpu.vector_table_1074119656_142_get_value_param0 = vector_table_1074119656_142_get_value_param0;
                  hdr.cpu.hit0 = hit0;
                  hdr.cpu.dev = meta.dev;
                } else {
                  // EP node  19616:Else
                  // BDD node 147:dchain_allocate_new_index(chain:(w64 1074088232), index_out:(w64 1074264888)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  20326:VectorTableLookup
                  // BDD node 315:vector_borrow(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074265168)[ -> (w64 1074150768)])
                  vector_table_1074136872_315_key0 = meta.dev;
                  vector_table_1074136872_315.apply();
                  // EP node  21162:If
                  // BDD node 320:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_1152)))
                  if ((16w0xffff) != (vector_table_1074136872_315_get_value_param0)) {
                    // EP node  21163:Then
                    // BDD node 320:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_1152)))
                    // EP node  26909:Ignore
                    // BDD node 316:vector_return(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074150768)[(ReadLSB w16 (w32 0) vector_data_1152)])
                    // EP node  30381:Forward
                    // BDD node 321:FORWARD
                    nf_dev[15:0] = vector_table_1074136872_315_get_value_param0;
                    trigger_forward = true;
                  } else {
                    // EP node  21164:Else
                    // BDD node 320:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_1152)))
                    // EP node  22756:Ignore
                    // BDD node 337:vector_return(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074150768)[(ReadLSB w16 (w32 0) vector_data_1152)])
                    // EP node  26180:Drop
                    // BDD node 322:DROP
                    drop();
                  }
                }
              } else {
                // EP node  2168:Else
                // BDD node 146:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_r1))
                // EP node  2474:DchainTableLookup
                // BDD node 174:dchain_rejuvenate_index(chain:(w64 1074088232), index:(ReadLSB w32 (w32 0) allocated_index_r1), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074088232_174_key0 = guarded_map_table_1074040056_145_get_value_param0;
                dchain_table_1074088232_174.apply();
                // EP node  2654:VectorRegisterLookup
                // BDD node 175:vector_borrow(vector:(w64 1074071096), index:(ReadLSB w32 (w32 0) allocated_index_r1), val_out:(w64 1074264464)[ -> (w64 1074084992)])
                bit<32> vector_reg_value0 = vector_register_1074071096_0_read_2654.execute(guarded_map_table_1074040056_145_get_value_param0);
                // EP node  2869:GuardedMapTableLookup
                // BDD node 176:map_get(map:(w64 1074088616), key:(w64 1074264482)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value_out:(w64 1074264412)[(w32 4294967295) -> (ReadLSB w32 (w32 0) allocated_index_1)])
                guarded_map_table_1074088616_176_key0 = hdr.hdr1.data3[63:32];
                guarded_map_table_1074088616_176_key1 = hdr.hdr2.data1;
                bool hit1 = guarded_map_table_1074088616_176.apply().hit;
                // EP node  3062:If
                // BDD node 177:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_1))
                if (!hit1) {
                  // EP node  3063:Then
                  // BDD node 177:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_1))
                  // EP node  6803:If
                  // BDD node 178:if ((Ult (ReadLSB w32 (w32 0) vector_data_768) (w32 16))
                  if ((vector_reg_value0) < (32w0x00000010)) {
                    // EP node  6804:Then
                    // BDD node 178:if ((Ult (ReadLSB w32 (w32 0) vector_data_768) (w32 16))
                    // EP node  7969:VectorTableLookup
                    // BDD node 183:vector_borrow(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074271888)[ -> (w64 1074150768)])
                    vector_table_1074136872_183_key0 = meta.dev;
                    vector_table_1074136872_183.apply();
                    // EP node  9575:VectorRegisterUpdate
                    // BDD node 182:vector_return(vector:(w64 1074071096), index:(ReadLSB w32 (w32 0) allocated_index_r1), value:(w64 1074084992)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data_768))])
                    reg_write0 = (32w0x00000001) + (vector_reg_value0);
                    vector_register_1074071096_0_write_9575.execute(guarded_map_table_1074040056_145_get_value_param0);
                    // EP node  10605:If
                    // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r9)))
                    if ((16w0xffff) != (vector_table_1074136872_183_get_value_param0)) {
                      // EP node  10606:Then
                      // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r9)))
                      // EP node  13502:Ignore
                      // BDD node 184:vector_return(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074150768)[(ReadLSB w16 (w32 0) vector_data_r9)])
                      // EP node  13633:SendToController
                      // BDD node 180:map_put(map:(w64 1074088616), key:(w64 1074116336)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(Extract w32 0 (Add w64 (w64 1) (SExt w64 (Extract w32 0 (Add w64 (w64 18446744073709551615) (SExt w64 (ReadLSB w32 (w32 0) vector_data_768))))))))
                      send_to_controller(13633);
                      hdr.cpu.vector_table_1074119656_142_get_value_param0 = vector_table_1074119656_142_get_value_param0;
                      hdr.cpu.hit0 = hit0;
                      hdr.cpu.hit1 = hit1;
                      hdr.cpu.vector_table_1074136872_183_get_value_param0 = vector_table_1074136872_183_get_value_param0;
                      hdr.cpu.vector_reg_value0 = vector_reg_value0;
                    } else {
                      // EP node  10607:Else
                      // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r9)))
                      // EP node  15114:Ignore
                      // BDD node 302:vector_return(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074150768)[(ReadLSB w16 (w32 0) vector_data_r9)])
                      // EP node  15303:SendToController
                      // BDD node 301:map_put(map:(w64 1074088616), key:(w64 1074116336)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(Extract w32 0 (Add w64 (w64 1) (SExt w64 (Extract w32 0 (Add w64 (w64 18446744073709551615) (SExt w64 (ReadLSB w32 (w32 0) vector_data_768))))))))
                      send_to_controller(15303);
                      hdr.cpu.vector_table_1074119656_142_get_value_param0 = vector_table_1074119656_142_get_value_param0;
                      hdr.cpu.hit0 = hit0;
                      hdr.cpu.hit1 = hit1;
                      hdr.cpu.vector_table_1074136872_183_get_value_param0 = vector_table_1074136872_183_get_value_param0;
                      hdr.cpu.vector_reg_value0 = vector_reg_value0;
                    }
                  } else {
                    // EP node  6805:Else
                    // BDD node 178:if ((Ult (ReadLSB w32 (w32 0) vector_data_768) (w32 16))
                    // EP node  16314:Ignore
                    // BDD node 191:vector_return(vector:(w64 1074071096), index:(ReadLSB w32 (w32 0) allocated_index_r1), value:(w64 1074084992)[(ReadLSB w32 (w32 0) vector_data_768)])
                    // EP node  19039:Drop
                    // BDD node 195:DROP
                    drop();
                  }
                } else {
                  // EP node  3064:Else
                  // BDD node 177:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_1))
                  // EP node  4396:VectorTableLookup
                  // BDD node 197:vector_borrow(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074274720)[ -> (w64 1074150768)])
                  vector_table_1074136872_197_key0 = meta.dev;
                  vector_table_1074136872_197.apply();
                  // EP node  4988:Ignore
                  // BDD node 196:vector_return(vector:(w64 1074071096), index:(ReadLSB w32 (w32 0) allocated_index_r1), value:(w64 1074084992)[(ReadLSB w32 (w32 0) vector_data_768)])
                  // EP node  5264:If
                  // BDD node 202:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r8)))
                  if ((16w0xffff) != (vector_table_1074136872_197_get_value_param0)) {
                    // EP node  5265:Then
                    // BDD node 202:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r8)))
                    // EP node  5526:Ignore
                    // BDD node 198:vector_return(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074150768)[(ReadLSB w16 (w32 0) vector_data_r8)])
                    // EP node  6574:Forward
                    // BDD node 203:FORWARD
                    nf_dev[15:0] = vector_table_1074136872_197_get_value_param0;
                    trigger_forward = true;
                  } else {
                    // EP node  5266:Else
                    // BDD node 202:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r8)))
                    // EP node  11000:Ignore
                    // BDD node 297:vector_return(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074150768)[(ReadLSB w16 (w32 0) vector_data_r8)])
                    // EP node  12510:Drop
                    // BDD node 204:DROP
                    drop();
                  }
                }
              }
            } else {
              // EP node  640:Else
              // BDD node 144:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  1046:VectorTableLookup
              // BDD node 205:vector_borrow(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074276128)[ -> (w64 1074150768)])
              vector_table_1074136872_205_key0 = meta.dev;
              vector_table_1074136872_205.apply();
              // EP node  1299:Ignore
              // BDD node 206:vector_return(vector:(w64 1074136872), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074150768)[(ReadLSB w16 (w32 0) vector_data_1152)])
              // EP node  1447:If
              // BDD node 210:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_1152)))
              if ((16w0xffff) != (vector_table_1074136872_205_get_value_param0)) {
                // EP node  1448:Then
                // BDD node 210:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_1152)))
                // EP node  2036:Forward
                // BDD node 211:FORWARD
                nf_dev[15:0] = vector_table_1074136872_205_get_value_param0;
                trigger_forward = true;
              } else {
                // EP node  1449:Else
                // BDD node 210:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_1152)))
                // EP node  3891:Drop
                // BDD node 212:DROP
                drop();
              }
            }
          }
          // EP node  109:Else
          // BDD node 140:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  946:ParserReject
          // BDD node 215:DROP
        }
        // EP node  14:Else
        // BDD node 138:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  4028:ParserReject
        // BDD node 217:DROP
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
