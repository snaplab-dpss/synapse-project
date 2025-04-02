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
  bit<16> vector_reg_value0;
  bit<32> dev;
  bit<32> vector_reg_value1;

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
    transition select (hdr.hdr0.data1) {
      2048: parser_139;
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
    transition select (hdr.hdr1.data1) {
      6: parser_141;
      17: parser_141;
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

  bool trigger_forward = false;
  bit<32> nf_dev = 0;
  table forward_nf_dev {
    key = { nf_dev: exact; }
    actions = { fwd; }
    size = 64;
  }

  Register<bit<16>,_>(32, 0) vector_register_1074137256_0;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074137256_0) vector_register_1074137256_0_read_54 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  bit<32> vector_table_1074120040_142_get_value_param0 = 32w0;
  action vector_table_1074120040_142_get_value(bit<32> _vector_table_1074120040_142_get_value_param0) {
    vector_table_1074120040_142_get_value_param0 = _vector_table_1074120040_142_get_value_param0;
  }

  bit<32> vector_table_1074120040_142_key0 = 32w0;
  table vector_table_1074120040_142 {
    key = {
      vector_table_1074120040_142_key0: exact;
    }
    actions = {
      vector_table_1074120040_142_get_value;
    }
    size = 32;
  }

  bit<32> map_table_1074040440_145_get_value_param0 = 32w0;
  action map_table_1074040440_145_get_value(bit<32> _map_table_1074040440_145_get_value_param0) {
    map_table_1074040440_145_get_value_param0 = _map_table_1074040440_145_get_value_param0;
  }

  bit<32> map_table_1074040440_145_key0 = 32w0;
  table map_table_1074040440_145 {
    key = {
      map_table_1074040440_145_key0: exact;
    }
    actions = {
      map_table_1074040440_145_get_value;
    }
    size = 65536;
    idle_timeout = true;
  }

  Register<bit<32>,_>(65536, 0) vector_register_1074071480_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074071480_0) vector_register_1074071480_0_read_2783 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  bit<32> dchain_table_1074088616_174_key0 = 32w0;
  table dchain_table_1074088616_174 {
    key = {
      dchain_table_1074088616_174_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 65536;
    idle_timeout = true;
  }

  bit<32> map_table_1074089000_176_get_value_param0 = 32w0;
  action map_table_1074089000_176_get_value(bit<32> _map_table_1074089000_176_get_value_param0) {
    map_table_1074089000_176_get_value_param0 = _map_table_1074089000_176_get_value_param0;
  }

  bit<32> map_table_1074089000_176_key0 = 32w0;
  bit<16> map_table_1074089000_176_key1 = 16w0;
  table map_table_1074089000_176 {
    key = {
      map_table_1074089000_176_key0: exact;
      map_table_1074089000_176_key1: exact;
    }
    actions = {
      map_table_1074089000_176_get_value;
    }
    size = 1048576;
  }


  RegisterAction<bit<32>, bit<32>, void>(vector_register_1074071480_0) vector_register_1074071480_0_write_7843 = {
    void apply(inout bit<32> value) {
      value = reg_write0;
    }
  };


  apply {
    if (hdr.cpu.isValid()) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:Ignore
      // BDD node 136:expire_items_single_map(chain:(w64 1074088616), vector:(w64 1074054264), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074040440))
      // EP node  22:ParserExtraction
      // BDD node 137:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 14), chunk:(w64 1074262496)[ -> (w64 1073761328)])
      if(hdr.hdr0.isValid()) {
        // EP node  54:VectorRegisterLookup
        // BDD node 205:vector_borrow(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074276512)[ -> (w64 1074151152)])
        bit<16> vector_reg_value0 = vector_register_1074137256_0_read_54.execute(meta.dev);
        // EP node  107:ParserCondition
        // BDD node 138:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  108:Then
        // BDD node 138:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  151:VectorTableLookup
        // BDD node 142:vector_borrow(vector:(w64 1074120040), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074264168)[ -> (w64 1074133936)])
        vector_table_1074120040_142_key0 = meta.dev;
        vector_table_1074120040_142.apply();
        // EP node  205:ParserExtraction
        // BDD node 139:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 20), chunk:(w64 1074263232)[ -> (w64 1073761584)])
        if(hdr.hdr1.isValid()) {
          // EP node  310:ParserCondition
          // BDD node 140:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  311:Then
          // BDD node 140:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  550:ParserExtraction
          // BDD node 141:packet_borrow_next_chunk(p:(w64 1074251832), length:(w32 4), chunk:(w64 1074263888)[ -> (w64 1073761840)])
          if(hdr.hdr2.isValid()) {
            // EP node  639:If
            // BDD node 144:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r28))
            if ((32w0x00000000) == (vector_table_1074120040_142_get_value_param0)) {
              // EP node  640:Then
              // BDD node 144:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r28))
              // EP node  2013:Ignore
              // BDD node 143:vector_return(vector:(w64 1074120040), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074133936)[(ReadLSB w32 (w32 0) vector_data_r28)])
              // EP node  2156:MapTableLookup
              // BDD node 145:map_get(map:(w64 1074040440), key:(w64 1074264716)[(ReadLSB w32 (w32 268) packet_chunks) -> (ReadLSB w32 (w32 268) packet_chunks)], value_out:(w64 1074264776)[(w32 4294967295) -> (ReadLSB w32 (w32 0) allocated_index)])
              map_table_1074040440_145_key0 = hdr.hdr1.data3[63:32];
              bool hit0 = map_table_1074040440_145.apply().hit;
              // EP node  2477:If
              // BDD node 146:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0) {
                // EP node  2478:Then
                // BDD node 146:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  13105:SendToController
                // BDD node 147:dchain_allocate_new_index(chain:(w64 1074088616), index_out:(w64 1074265272)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index_r1)], time:(ReadLSB w64 (w32 0) next_time))
                send_to_controller(13105);
                hdr.cpu.vector_reg_value0 = vector_reg_value0;
                hdr.cpu.dev = meta.dev;
              } else {
                // EP node  2479:Else
                // BDD node 146:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  2783:VectorRegisterLookup
                // BDD node 175:vector_borrow(vector:(w64 1074071480), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074264848)[ -> (w64 1074085376)])
                bit<32> vector_reg_value1 = vector_register_1074071480_0_read_2783.execute(map_table_1074040440_145_get_value_param0);
                // EP node  2956:DchainTableLookup
                // BDD node 174:dchain_rejuvenate_index(chain:(w64 1074088616), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074088616_174_key0 = map_table_1074040440_145_get_value_param0;
                dchain_table_1074088616_174.apply();
                // EP node  3135:MapTableLookup
                // BDD node 176:map_get(map:(w64 1074089000), key:(w64 1074264866)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value_out:(w64 1074264796)[(w32 4294967295) -> (ReadLSB w32 (w32 0) allocated_index_1)])
                map_table_1074089000_176_key0 = hdr.hdr1.data3[63:32];
                map_table_1074089000_176_key1 = hdr.hdr2.data1;
                bool hit1 = map_table_1074089000_176.apply().hit;
                // EP node  3320:If
                // BDD node 177:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_1))
                if (!hit1) {
                  // EP node  3321:Then
                  // BDD node 177:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_1))
                  // EP node  7065:If
                  // BDD node 178:if ((Ult (ReadLSB w32 (w32 0) vector_data_r35) (w32 16))
                  if ((vector_reg_value1) < (32w0x00000010)) {
                    // EP node  7066:Then
                    // BDD node 178:if ((Ult (ReadLSB w32 (w32 0) vector_data_r35) (w32 16))
                    // EP node  7843:VectorRegisterUpdate
                    // BDD node 182:vector_return(vector:(w64 1074071480), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1074085376)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data_r35))])
                    reg_write0 = (32w0x00000001) + (vector_reg_value1);
                    vector_register_1074071480_0_write_7843.execute(map_table_1074040440_145_get_value_param0);
                    // EP node  8216:If
                    // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                    if ((16w0xffff) != (vector_reg_value0)) {
                      // EP node  8217:Then
                      // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                      // EP node  10428:Ignore
                      // BDD node 184:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
                      // EP node  10530:SendToController
                      // BDD node 180:map_put(map:(w64 1074089000), key:(w64 1074116720)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(Extract w32 0 (Add w64 (w64 1) (SExt w64 (Extract w32 0 (Add w64 (w64 18446744073709551615) (SExt w64 (ReadLSB w32 (w32 0) vector_data_r35))))))))
                      send_to_controller(10530);
                      hdr.cpu.vector_reg_value1 = vector_reg_value1;
                      hdr.cpu.vector_reg_value0 = vector_reg_value0;
                    } else {
                      // EP node  8218:Else
                      // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                      // EP node  11056:Ignore
                      // BDD node 303:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
                      // EP node  11162:SendToController
                      // BDD node 302:map_put(map:(w64 1074089000), key:(w64 1074116720)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))], value:(Extract w32 0 (Add w64 (w64 1) (SExt w64 (Extract w32 0 (Add w64 (w64 18446744073709551615) (SExt w64 (ReadLSB w32 (w32 0) vector_data_r35))))))))
                      send_to_controller(11162);
                      hdr.cpu.vector_reg_value1 = vector_reg_value1;
                    }
                  } else {
                    // EP node  7067:Else
                    // BDD node 178:if ((Ult (ReadLSB w32 (w32 0) vector_data_r35) (w32 16))
                    // EP node  11709:Ignore
                    // BDD node 191:vector_return(vector:(w64 1074071480), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1074085376)[(ReadLSB w32 (w32 0) vector_data_r35)])
                    // EP node  13048:Drop
                    // BDD node 195:DROP
                    drop();
                  }
                } else {
                  // EP node  3322:Else
                  // BDD node 177:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key_1))
                  // EP node  5159:Ignore
                  // BDD node 196:vector_return(vector:(w64 1074071480), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1074085376)[(ReadLSB w32 (w32 0) vector_data_r35)])
                  // EP node  5575:Ignore
                  // BDD node 198:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
                  // EP node  5846:If
                  // BDD node 202:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                  if ((16w0xffff) != (vector_reg_value0)) {
                    // EP node  5847:Then
                    // BDD node 202:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                    // EP node  6849:Forward
                    // BDD node 203:FORWARD
                    nf_dev[15:0] = vector_reg_value0;
                    trigger_forward = true;
                  } else {
                    // EP node  5848:Else
                    // BDD node 202:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                    // EP node  9561:Drop
                    // BDD node 204:DROP
                    drop();
                  }
                }
              }
            } else {
              // EP node  641:Else
              // BDD node 144:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r28))
              // EP node  1125:Ignore
              // BDD node 294:vector_return(vector:(w64 1074120040), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074133936)[(ReadLSB w32 (w32 0) vector_data_r28)])
              // EP node  1249:If
              // BDD node 210:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
              if ((16w0xffff) != (vector_reg_value0)) {
                // EP node  1250:Then
                // BDD node 210:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                // EP node  1374:Ignore
                // BDD node 206:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
                // EP node  1897:Forward
                // BDD node 211:FORWARD
                nf_dev[15:0] = vector_reg_value0;
                trigger_forward = true;
              } else {
                // EP node  1251:Else
                // BDD node 210:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r14)))
                // EP node  3523:Ignore
                // BDD node 295:vector_return(vector:(w64 1074137256), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074151152)[(ReadLSB w16 (w32 0) vector_data_r14)])
                // EP node  4358:Drop
                // BDD node 212:DROP
                drop();
              }
            }
          }
          // EP node  312:Else
          // BDD node 140:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  955:ParserReject
          // BDD node 215:DROP
        }
        // EP node  109:Else
        // BDD node 138:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  4789:ParserReject
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
