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
  bit<16> code_path;
  bit<16> egress_dev;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;
  bit<32> dev;

};

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
    transition parser_135;
  }
  state parser_135 {
    transition select (hdr.hdr0.data1) {
      2048: parser_136;
      default: parser_199;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_199 {
    transition reject;
  }
  state parser_137 {
    transition select (hdr.hdr1.data1) {
      6: parser_138;
      17: parser_138;
      default: parser_197;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_193;
  }
  state parser_197 {
    transition reject;
  }
  state parser_193 {
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

  Register<bit<32>,_>(32, 0) vector_register_1074085496_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074085496_0) vector_register_1074085496_0_read_84 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };
  bit<32> dchain_table_1074085072_142_key0 = 32w0;
  table dchain_table_1074085072_142 {
    key = {
      dchain_table_1074085072_142_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 65536;
    idle_timeout = true;
  }

  Register<bit<16>,_>(65536, 0) vector_register_1074066912_0;
  Register<bit<16>,_>(65536, 0) vector_register_1074066912_1;
  Register<bit<32>,_>(65536, 0) vector_register_1074066912_2;
  Register<bit<32>,_>(65536, 0) vector_register_1074066912_3;
  Register<bit<8>,_>(65536, 0) vector_register_1074066912_4;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074066912_0) vector_register_1074066912_0_read_2338 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074066912_1) vector_register_1074066912_1_read_2338 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074066912_2) vector_register_1074066912_2_read_2338 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074066912_3) vector_register_1074066912_3_read_2338 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  RegisterAction<bit<8>, bit<32>, bit<8>>(vector_register_1074066912_4) vector_register_1074066912_4_read_2338 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
    }
  };

  Register<bit<16>,_>(32, 0) vector_register_1074102712_0;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074102712_0) vector_register_1074102712_0_read_3467 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };

  bit<32> map_table_1074053088_165_get_value_param0 = 32w0;
  action map_table_1074053088_165_get_value(bit<32> _map_table_1074053088_165_get_value_param0) {
    map_table_1074053088_165_get_value_param0 = _map_table_1074053088_165_get_value_param0;
  }

  bit<16> map_table_1074053088_165_key0 = 16w0;
  bit<16> map_table_1074053088_165_key1 = 16w0;
  bit<32> map_table_1074053088_165_key2 = 32w0;
  bit<32> map_table_1074053088_165_key3 = 32w0;
  bit<8> map_table_1074053088_165_key4 = 8w0;
  table map_table_1074053088_165 {
    key = {
      map_table_1074053088_165_key0: exact;
      map_table_1074053088_165_key1: exact;
      map_table_1074053088_165_key2: exact;
      map_table_1074053088_165_key3: exact;
      map_table_1074053088_165_key4: exact;
    }
    actions = {
      map_table_1074053088_165_get_value;
    }
    size = 65536;
    idle_timeout = true;
  }

  bit<32> dchain_table_1074085072_185_key0 = 32w0;
  table dchain_table_1074085072_185 {
    key = {
      dchain_table_1074085072_185_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 65536;
    idle_timeout = true;
  }


  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074102712_0) vector_register_1074102712_0_read_825 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  apply {
    if (hdr.cpu.isValid()) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  7027:SendToController
        // BDD node 167:dchain_allocate_new_index(chain:(w64 1074085072), index_out:(w64 1074223792)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        send_to_controller(7027);
        hdr.cpu.dev = hdr.recirc.dev;
      }
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:Ignore
      // BDD node 133:expire_items_single_map(chain:(w64 1074085072), vector:(w64 1074066912), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074053088))
      // EP node  4:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074206600), length:(w32 14), chunk:(w64 1074217352)[ -> (w64 1073763264)])
      if(hdr.hdr0.isValid()) {
        // EP node  11:ParserCondition
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  12:Then
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  20:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074206600), length:(w32 20), chunk:(w64 1074218088)[ -> (w64 1073763520)])
        if(hdr.hdr1.isValid()) {
          // EP node  39:ParserCondition
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  40:Then
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  52:ParserExtraction
          // BDD node 138:packet_borrow_next_chunk(p:(w64 1074206600), length:(w32 4), chunk:(w64 1074218744)[ -> (w64 1073763776)])
          if(hdr.hdr2.isValid()) {
            // EP node  84:VectorRegisterLookup
            // BDD node 139:vector_borrow(vector:(w64 1074085496), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074219024)[ -> (w64 1074099392)])
            // EP node  155:Ignore
            // BDD node 140:vector_return(vector:(w64 1074085496), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074099392)[(ReadLSB w32 (w32 0) vector_data_512)])
            // EP node  232:If
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
            if ((32w0x00000000) == (vector_register_1074085496_0_read_84.execute(meta.dev))) {
              // EP node  233:Then
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  1916:DchainTableLookup
              // BDD node 142:dchain_is_index_allocated(chain:(w64 1074085072), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)))
              dchain_table_1074085072_142_key0 = (bit<32>)(bswap16(hdr.hdr2.data1));
              bool hit0 = dchain_table_1074085072_142.apply().hit;
              // EP node  2101:If
              // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
              if (hit0) {
                // EP node  2102:Then
                // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
                // EP node  2338:VectorRegisterLookup
                // BDD node 144:vector_borrow(vector:(w64 1074066912), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)), val_out:(w64 1074219688)[ -> (w64 1074080808)])
                bit<16> vector_reg_value0 = vector_register_1074066912_0_read_2338.execute((bit<32>)(bswap16(hdr.hdr2.data1)));
                bit<16> vector_reg_value1 = vector_register_1074066912_1_read_2338.execute((bit<32>)(bswap16(hdr.hdr2.data1)));
                bit<32> vector_reg_value2 = vector_register_1074066912_2_read_2338.execute((bit<32>)(bswap16(hdr.hdr2.data1)));
                bit<32> vector_reg_value3 = vector_register_1074066912_3_read_2338.execute((bit<32>)(bswap16(hdr.hdr2.data1)));
                bit<8> vector_reg_value4 = vector_register_1074066912_4_read_2338.execute((bit<32>)(bswap16(hdr.hdr2.data1)));
                // EP node  2547:Ignore
                // BDD node 145:vector_return(vector:(w64 1074066912), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)), value:(w64 1074080808)[(ReadLSB w104 (w32 0) vector_data_384)])
                // EP node  2762:Ignore
                // BDD node 146:dchain_rejuvenate_index(chain:(w64 1074085072), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)), time:(ReadLSB w64 (w32 0) next_time))
                // EP node  2983:If
                // BDD node 147:if ((And (And (Eq (ReadLSB w32 (w32 8) vector_data_384) (ReadLSB w32 (w32 268) packet_chunks)) (Eq (ReadLSB w16 (w32 2) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks))) (Eq (Read w8 (w32 12) vector_data_384) (Read w8 (w32 265) packet_chunks)))
                bool cond0 = false;
                if ((vector_reg_value3) == (hdr.hdr1.data3)) {
                  if ((vector_reg_value1) == (hdr.hdr2.data0)) {
                    if ((vector_reg_value4) == (hdr.hdr1.data1)) {
                      cond0 = true;
                    }
                  }
                }
                if (cond0) {
                  // EP node  2984:Then
                  // BDD node 147:if ((And (And (Eq (ReadLSB w32 (w32 8) vector_data_384) (ReadLSB w32 (w32 268) packet_chunks)) (Eq (ReadLSB w16 (w32 2) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks))) (Eq (Read w8 (w32 12) vector_data_384) (Read w8 (w32 265) packet_chunks)))
                  // EP node  3222:Ignore
                  // BDD node 148:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763520), l4_header:(w64 1073763776), packet:(w64 1074155224))
                  // EP node  3467:VectorRegisterLookup
                  // BDD node 149:vector_borrow(vector:(w64 1074102712), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074220584)[ -> (w64 1074116608)])
                  bit<16> vector_reg_value5 = vector_register_1074102712_0_read_3467.execute(meta.dev);
                  // EP node  3718:Ignore
                  // BDD node 150:vector_return(vector:(w64 1074102712), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116608)[(ReadLSB w16 (w32 0) vector_data_640)])
                  // EP node  4017:ModifyHeader
                  // BDD node 151:packet_return_chunk(p:(w64 1074206600), the_chunk:(w64 1073763776)[(Concat w32 (Read w8 (w32 1) vector_data_384) (Concat w24 (Read w8 (w32 0) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks)))])
                  hdr.hdr2.data1 = vector_reg_value0;
                  // EP node  4281:ModifyHeader
                  // BDD node 152:packet_return_chunk(p:(w64 1074206600), the_chunk:(w64 1073763520)[(Concat w160 (Read w8 (w32 7) vector_data_384) (Concat w152 (Read w8 (w32 6) vector_data_384) (Concat w144 (Read w8 (w32 5) vector_data_384) (Concat w136 (Read w8 (w32 4) vector_data_384) (Concat w128 (Read w8 (w32 271) packet_chunks) (Concat w120 (Read w8 (w32 270) packet_chunks) (Concat w112 (Read w8 (w32 269) packet_chunks) (Concat w104 (Read w8 (w32 268) packet_chunks) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                  hdr.hdr1.data4 = vector_reg_value2;
                  // EP node  4776:If
                  // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                  if ((16w0xffff) != (vector_reg_value5)) {
                    // EP node  4777:Then
                    // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  5110:Forward
                    // BDD node 155:FORWARD
                    nf_dev[15:0] = vector_reg_value5;
                    trigger_forward = true;
                  } else {
                    // EP node  4778:Else
                    // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  6765:Drop
                    // BDD node 156:DROP
                    drop();
                  }
                } else {
                  // EP node  2985:Else
                  // BDD node 147:if ((And (And (Eq (ReadLSB w32 (w32 8) vector_data_384) (ReadLSB w32 (w32 268) packet_chunks)) (Eq (ReadLSB w16 (w32 2) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks))) (Eq (Read w8 (w32 12) vector_data_384) (Read w8 (w32 265) packet_chunks)))
                  // EP node  9105:Drop
                  // BDD node 160:DROP
                  drop();
                }
              } else {
                // EP node  2103:Else
                // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
                // EP node  8769:Drop
                // BDD node 164:DROP
                drop();
              }
            } else {
              // EP node  234:Else
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  327:MapTableLookup
              // BDD node 165:map_get(map:(w64 1074053088), key:(w64 1074216578)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074223360)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              map_table_1074053088_165_key0 = hdr.hdr2.data0;
              map_table_1074053088_165_key1 = hdr.hdr2.data1;
              map_table_1074053088_165_key2 = hdr.hdr1.data3;
              map_table_1074053088_165_key3 = hdr.hdr1.data4;
              map_table_1074053088_165_key4 = hdr.hdr1.data1;
              bool hit1 = map_table_1074053088_165.apply().hit;
              // EP node  428:If
              // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit1) {
                // EP node  429:Then
                // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  5500:Recirculate
                // BDD node 167:dchain_allocate_new_index(chain:(w64 1074085072), index_out:(w64 1074223792)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                meta.recirculate = true;
                hdr.recirc.setValid();
                hdr.recirc.code_path = 0;
                hdr.recirc.dev = meta.dev;
                fwd(256);
              } else {
                // EP node  430:Else
                // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  547:DchainTableLookup
                // BDD node 185:dchain_rejuvenate_index(chain:(w64 1074085072), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074085072_185_key0 = map_table_1074053088_165_get_value_param0;
                dchain_table_1074085072_185.apply();
                // EP node  672:Ignore
                // BDD node 186:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763520), l4_header:(w64 1073763776), packet:(w64 1074155224))
                // EP node  825:VectorRegisterLookup
                // BDD node 187:vector_borrow(vector:(w64 1074102712), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074227432)[ -> (w64 1074116608)])
                bit<16> vector_reg_value6 = vector_register_1074102712_0_read_825.execute(meta.dev);
                // EP node  962:Ignore
                // BDD node 188:vector_return(vector:(w64 1074102712), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116608)[(ReadLSB w16 (w32 0) vector_data_640)])
                // EP node  1128:ModifyHeader
                // BDD node 189:packet_return_chunk(p:(w64 1074206600), the_chunk:(w64 1073763776)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) allocated_index)))])
                hdr.hdr2.data0[15:8] = map_table_1074053088_165_get_value_param0[7:0];
                hdr.hdr2.data0[7:0] = map_table_1074053088_165_get_value_param0[15:8];
                // EP node  1278:ModifyHeader
                // BDD node 190:packet_return_chunk(p:(w64 1074206600), the_chunk:(w64 1073763520)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                hdr.hdr1.data3[31:24] = 8w0x01;
                hdr.hdr1.data3[23:16] = 8w0x02;
                hdr.hdr1.data3[15:8] = 8w0x03;
                hdr.hdr1.data3[7:0] = 8w0x04;
                // EP node  1564:If
                // BDD node 192:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                if ((16w0xffff) != (vector_reg_value6)) {
                  // EP node  1565:Then
                  // BDD node 192:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                  // EP node  1765:Forward
                  // BDD node 193:FORWARD
                  nf_dev[15:0] = vector_reg_value6;
                  trigger_forward = true;
                } else {
                  // EP node  1566:Else
                  // BDD node 192:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                  // EP node  5849:Drop
                  // BDD node 194:DROP
                  drop();
                }
              }
            }
          }
          // EP node  41:Else
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  7781:ParserReject
          // BDD node 197:DROP
        }
        // EP node  13:Else
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  6869:ParserReject
        // BDD node 199:DROP
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
