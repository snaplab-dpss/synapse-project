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
  bit<32> vector_table_1074076424_139_get_value_param0;
  @padding bit<31> pad_hit1;
  bool hit1;
  bit<32> map_table_1074044016_157_get_value_param0;
  bit<16> vector_table_1074093640_149_get_value_param0;
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
    transition parser_135_0;
  }
  state parser_135_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_136;
      default: parser_193;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_193 {
    transition reject;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_138;
      8w0x11: parser_138;
      default: parser_191;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_147;
  }
  state parser_191 {
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

  bit<32> vector_table_1074076424_139_get_value_param0 = 32w0;
  action vector_table_1074076424_139_get_value(bit<32> _vector_table_1074076424_139_get_value_param0) {
    vector_table_1074076424_139_get_value_param0 = _vector_table_1074076424_139_get_value_param0;
  }

  bit<32> vector_table_1074076424_139_key0 = 32w0;
  table vector_table_1074076424_139 {
    key = {
      vector_table_1074076424_139_key0: exact;
    }
    actions = {
      vector_table_1074076424_139_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074093640_149_get_value_param0 = 16w0;
  action vector_table_1074093640_149_get_value(bit<16> _vector_table_1074093640_149_get_value_param0) {
    vector_table_1074093640_149_get_value_param0 = _vector_table_1074093640_149_get_value_param0;
  }

  bit<32> vector_table_1074093640_149_key0 = 32w0;
  table vector_table_1074093640_149 {
    key = {
      vector_table_1074093640_149_key0: exact;
    }
    actions = {
      vector_table_1074093640_149_get_value;
    }
    size = 36;
  }

  bit<32> map_table_1074044016_142_get_value_param0 = 32w0;
  action map_table_1074044016_142_get_value(bit<32> _map_table_1074044016_142_get_value_param0) {
    map_table_1074044016_142_get_value_param0 = _map_table_1074044016_142_get_value_param0;
  }

  bit<16> map_table_1074044016_142_key0 = 16w0;
  bit<16> map_table_1074044016_142_key1 = 16w0;
  bit<32> map_table_1074044016_142_key2 = 32w0;
  bit<32> map_table_1074044016_142_key3 = 32w0;
  bit<8> map_table_1074044016_142_key4 = 8w0;
  table map_table_1074044016_142 {
    key = {
      map_table_1074044016_142_key0: exact;
      map_table_1074044016_142_key1: exact;
      map_table_1074044016_142_key2: exact;
      map_table_1074044016_142_key3: exact;
      map_table_1074044016_142_key4: exact;
    }
    actions = {
      map_table_1074044016_142_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<32> map_table_1074044016_157_get_value_param0 = 32w0;
  action map_table_1074044016_157_get_value(bit<32> _map_table_1074044016_157_get_value_param0) {
    map_table_1074044016_157_get_value_param0 = _map_table_1074044016_157_get_value_param0;
  }

  bit<16> map_table_1074044016_157_key0 = 16w0;
  bit<16> map_table_1074044016_157_key1 = 16w0;
  bit<32> map_table_1074044016_157_key2 = 32w0;
  bit<32> map_table_1074044016_157_key3 = 32w0;
  bit<8> map_table_1074044016_157_key4 = 8w0;
  table map_table_1074044016_157 {
    key = {
      map_table_1074044016_157_key0: exact;
      map_table_1074044016_157_key1: exact;
      map_table_1074044016_157_key2: exact;
      map_table_1074044016_157_key3: exact;
      map_table_1074044016_157_key4: exact;
    }
    actions = {
      map_table_1074044016_157_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<32> dchain_table_1074076000_180_key0 = 32w0;
  table dchain_table_1074076000_180 {
    key = {
      dchain_table_1074076000_180_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
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
      // BDD node 133:expire_items_single_map(chain:(w64 1074076000), vector:(w64 1074057840), time:(Add w64 (w64 18446744073609551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074044016))
      // EP node  19:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074195848), length:(w32 14), chunk:(w64 1074206552)[ -> (w64 1073762144)])
      if(hdr.hdr0.isValid()) {
        // EP node  55:VectorTableLookup
        // BDD node 139:vector_borrow(vector:(w64 1074076424), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074208224)[ -> (w64 1074090320)])
        vector_table_1074076424_139_key0 = meta.dev;
        vector_table_1074076424_139.apply();
        // EP node  198:ParserCondition
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  199:Then
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  374:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074195848), length:(w32 20), chunk:(w64 1074207288)[ -> (w64 1073762400)])
        if(hdr.hdr1.isValid()) {
          // EP node  484:VectorTableLookup
          // BDD node 149:vector_borrow(vector:(w64 1074093640), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074210056)[ -> (w64 1074107536)])
          vector_table_1074093640_149_key0 = meta.dev;
          vector_table_1074093640_149.apply();
          // EP node  557:ParserCondition
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  558:Then
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  694:Ignore
          // BDD node 140:vector_return(vector:(w64 1074076424), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074090320)[(ReadLSB w32 (w32 0) vector_data_r11)])
          // EP node  787:If
          // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r11))
          if ((32w0x00000000) == (vector_table_1074076424_139_get_value_param0)) {
            // EP node  788:Then
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r11))
            // EP node  3116:ParserExtraction
            // BDD node 138:packet_borrow_next_chunk(p:(w64 1074195848), length:(w32 4), chunk:(w64 1074207944)[ -> (w64 1073762656)])
            if(hdr.hdr2.isValid()) {
              // EP node  3382:MapTableLookup
              // BDD node 142:map_get(map:(w64 1074044016), key:(w64 1074205849)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks))))))))))))], value_out:(w64 1074208856)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              map_table_1074044016_142_key0 = hdr.hdr2.data1;
              map_table_1074044016_142_key1 = hdr.hdr2.data0;
              map_table_1074044016_142_key2 = hdr.hdr1.data4;
              map_table_1074044016_142_key3 = hdr.hdr1.data3;
              map_table_1074044016_142_key4 = hdr.hdr1.data1;
              bool hit0 = map_table_1074044016_142.apply().hit;
              // EP node  3655:If
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0) {
                // EP node  3656:Then
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  10180:Drop
                // BDD node 147:DROP
                drop();
              } else {
                // EP node  3657:Else
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  4184:Ignore
                // BDD node 148:dchain_rejuvenate_index(chain:(w64 1074076000), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                // EP node  4818:Ignore
                // BDD node 150:vector_return(vector:(w64 1074093640), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107536)[(ReadLSB w16 (w32 0) vector_data_r68)])
                // EP node  6778:If
                // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
                if ((16w0xffff) != (vector_table_1074093640_149_get_value_param0)) {
                  // EP node  6779:Then
                  // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
                  // EP node  8578:Forward
                  // BDD node 155:FORWARD
                  nf_dev[15:0] = vector_table_1074093640_149_get_value_param0;
                  trigger_forward = true;
                } else {
                  // EP node  6780:Else
                  // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
                  // EP node  10465:Drop
                  // BDD node 156:DROP
                  drop();
                }
              }
            }
          } else {
            // EP node  789:Else
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r11))
            // EP node  890:ParserExtraction
            // BDD node 249:packet_borrow_next_chunk(p:(w64 1074195848), length:(w32 4), chunk:(w64 1074207944)[ -> (w64 1073762656)])
            if(hdr.hdr2.isValid()) {
              // EP node  1000:MapTableLookup
              // BDD node 157:map_get(map:(w64 1074044016), key:(w64 1074205826)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074211520)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              map_table_1074044016_157_key0 = hdr.hdr2.data0;
              map_table_1074044016_157_key1 = hdr.hdr2.data1;
              map_table_1074044016_157_key2 = hdr.hdr1.data3;
              map_table_1074044016_157_key3 = hdr.hdr1.data4;
              map_table_1074044016_157_key4 = hdr.hdr1.data1;
              bool hit1 = map_table_1074044016_157.apply().hit;
              // EP node  1151:If
              // BDD node 158:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit1) {
                // EP node  1152:Then
                // BDD node 158:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  2789:SendToController
                // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                send_to_controller(2789);
                hdr.cpu.vector_table_1074076424_139_get_value_param0 = vector_table_1074076424_139_get_value_param0;
                hdr.cpu.hit1 = hit1;
                hdr.cpu.map_table_1074044016_157_get_value_param0 = map_table_1074044016_157_get_value_param0;
                hdr.cpu.vector_table_1074093640_149_get_value_param0 = vector_table_1074093640_149_get_value_param0;
                hdr.cpu.dev = meta.dev;
              } else {
                // EP node  1153:Else
                // BDD node 158:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  1394:DchainTableLookup
                // BDD node 180:dchain_rejuvenate_index(chain:(w64 1074076000), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074076000_180_key0 = map_table_1074044016_157_get_value_param0;
                dchain_table_1074076000_180.apply();
                // EP node  1648:Ignore
                // BDD node 182:vector_return(vector:(w64 1074093640), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107536)[(ReadLSB w16 (w32 0) vector_data_r68)])
                // EP node  2594:If
                // BDD node 186:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
                if ((16w0xffff) != (vector_table_1074093640_149_get_value_param0)) {
                  // EP node  2595:Then
                  // BDD node 186:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
                  // EP node  2748:Forward
                  // BDD node 187:FORWARD
                  nf_dev[15:0] = vector_table_1074093640_149_get_value_param0;
                  trigger_forward = true;
                } else {
                  // EP node  2596:Else
                  // BDD node 186:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
                  // EP node  8853:Drop
                  // BDD node 188:DROP
                  drop();
                }
              }
            }
          }
          // EP node  559:Else
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  8308:ParserReject
          // BDD node 191:DROP
        }
        // EP node  200:Else
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  7401:ParserReject
        // BDD node 193:DROP
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
