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
  bit<16> vector_reg_value0;

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
    transition select (hdr.hdr0.data1) {
      2048: parser_138;
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
    transition select (hdr.hdr1.data1) {
      6: parser_140;
      17: parser_140;
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

  bit<32> vector_table_1074092896_141_get_value_param0 = 32w0;
  action vector_table_1074092896_141_get_value(bit<32> _vector_table_1074092896_141_get_value_param0) {
    vector_table_1074092896_141_get_value_param0 = _vector_table_1074092896_141_get_value_param0;
  }

  bit<32> vector_table_1074092896_141_key0 = 32w0;
  table vector_table_1074092896_141 {
    key = {
      vector_table_1074092896_141_key0: exact;
    }
    actions = {
      vector_table_1074092896_141_get_value;
    }
    size = 32;
  }

  Register<bit<16>,_>(32, 0) vector_register_1074110112_0;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074110112_0) vector_register_1074110112_0_read_651 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  bit<32> map_table_1074047920_144_get_value_param0 = 32w0;
  action map_table_1074047920_144_get_value(bit<32> _map_table_1074047920_144_get_value_param0) {
    map_table_1074047920_144_get_value_param0 = _map_table_1074047920_144_get_value_param0;
  }

  bit<32> map_table_1074047920_144_key0 = 32w0;
  bit<32> map_table_1074047920_144_key1 = 32w0;
  bit<32> map_table_1074047920_144_key2 = 32w0;
  bit<8> map_table_1074047920_144_key3 = 8w0;
  table map_table_1074047920_144 {
    key = {
      map_table_1074047920_144_key0: exact;
      map_table_1074047920_144_key1: exact;
      map_table_1074047920_144_key2: exact;
      map_table_1074047920_144_key3: exact;
    }
    actions = {
      map_table_1074047920_144_get_value;
    }
    size = 65536;
    idle_timeout = true;
  }

  bit<32> dchain_table_1074079904_174_key0 = 32w0;
  table dchain_table_1074079904_174 {
    key = {
      dchain_table_1074079904_174_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 65536;
    idle_timeout = true;
  }


  apply {
    if (hdr.cpu.isValid()) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:Ignore
      // BDD node 134:expire_items_single_map(chain:(w64 1074079904), vector:(w64 1074061744), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074047920))
      // EP node  19:Ignore
      // BDD node 135:cms_periodic_cleanup(cms:(w64 1074080320), time:(ReadLSB w64 (w32 0) next_time))
      // EP node  49:ParserExtraction
      // BDD node 136:packet_borrow_next_chunk(p:(w64 1074212752), length:(w32 14), chunk:(w64 1074223512)[ -> (w64 1073763904)])
      if(hdr.hdr0.isValid()) {
        // EP node  95:VectorTableLookup
        // BDD node 141:vector_borrow(vector:(w64 1074092896), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074225184)[ -> (w64 1074106792)])
        vector_table_1074092896_141_key0 = meta.dev;
        vector_table_1074092896_141.apply();
        // EP node  269:ParserCondition
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  270:Then
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  468:ParserExtraction
        // BDD node 138:packet_borrow_next_chunk(p:(w64 1074212752), length:(w32 20), chunk:(w64 1074224248)[ -> (w64 1073764160)])
        if(hdr.hdr1.isValid()) {
          // EP node  651:VectorRegisterLookup
          // BDD node 183:vector_borrow(vector:(w64 1074110112), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074232504)[ -> (w64 1074124008)])
          bit<16> vector_reg_value0 = vector_register_1074110112_0_read_651.execute(meta.dev);
          // EP node  759:CMSQuery
          // BDD node 147:cms_count_min(cms:(w64 1074080320), key:(w64 1074225826)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
          // EP node  834:ParserCondition
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  835:Then
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  989:Ignore
          // BDD node 142:vector_return(vector:(w64 1074092896), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074106792)[(ReadLSB w32 (w32 0) vector_data_r21)])
          // EP node  1092:If
          // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r21))
          if ((32w0x00000000) == (vector_table_1074092896_141_get_value_param0)) {
            // EP node  1093:Then
            // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r21))
            // EP node  2610:ParserExtraction
            // BDD node 140:packet_borrow_next_chunk(p:(w64 1074212752), length:(w32 4), chunk:(w64 1074224904)[ -> (w64 1073764416)])
            if(hdr.hdr2.isValid()) {
              // EP node  2765:MapTableLookup
              // BDD node 144:map_get(map:(w64 1074047920), key:(w64 1074222730)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074225792)[(w32 4294967295) -> (ReadLSB w32 (w32 0) allocated_index)])
              map_table_1074047920_144_key0 = hdr.hdr2.data0;
              map_table_1074047920_144_key1 = hdr.hdr1.data3[63:32];
              map_table_1074047920_144_key2 = hdr.hdr1.data3[31:0];
              map_table_1074047920_144_key3 = hdr.hdr1.data1;
              bool hit0 = map_table_1074047920_144.apply().hit;
              // EP node  2952:If
              // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0) {
                // EP node  2953:Then
                // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  7283:SendToController
                // BDD node 146:cms_increment(cms:(w64 1074080320), key:(w64 1074225826)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
                send_to_controller(7283);
                hdr.cpu.dev = meta.dev;
                hdr.cpu.vector_reg_value0 = vector_reg_value0;
              } else {
                // EP node  2954:Else
                // BDD node 145:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  3280:DchainTableLookup
                // BDD node 174:dchain_rejuvenate_index(chain:(w64 1074079904), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074079904_174_key0 = map_table_1074047920_144_get_value_param0;
                dchain_table_1074079904_174.apply();
                // EP node  3619:Ignore
                // BDD node 176:vector_return(vector:(w64 1074110112), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124008)[(ReadLSB w16 (w32 0) vector_data_r79)])
                // EP node  3841:If
                // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r79)))
                if ((16w0xffff) != (vector_reg_value0)) {
                  // EP node  3842:Then
                  // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r79)))
                  // EP node  7204:Forward
                  // BDD node 181:FORWARD
                  nf_dev[15:0] = vector_reg_value0;
                  trigger_forward = true;
                } else {
                  // EP node  3843:Else
                  // BDD node 180:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r79)))
                  // EP node  6193:Drop
                  // BDD node 182:DROP
                  drop();
                }
              }
            }
          } else {
            // EP node  1094:Else
            // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r21))
            // EP node  1638:ParserExtraction
            // BDD node 251:packet_borrow_next_chunk(p:(w64 1074212752), length:(w32 4), chunk:(w64 1074224904)[ -> (w64 1073764416)])
            if(hdr.hdr2.isValid()) {
              // EP node  1776:If
              // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r79)))
              if ((16w0xffff) != (vector_reg_value0)) {
                // EP node  1777:Then
                // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r79)))
                // EP node  1913:Ignore
                // BDD node 184:vector_return(vector:(w64 1074110112), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124008)[(ReadLSB w16 (w32 0) vector_data_r79)])
                // EP node  2484:Forward
                // BDD node 189:FORWARD
                nf_dev[15:0] = vector_reg_value0;
                trigger_forward = true;
              } else {
                // EP node  1778:Else
                // BDD node 188:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r79)))
                // EP node  4439:Ignore
                // BDD node 252:vector_return(vector:(w64 1074110112), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124008)[(ReadLSB w16 (w32 0) vector_data_r79)])
                // EP node  5322:Drop
                // BDD node 190:DROP
                drop();
              }
            }
          }
          // EP node  836:Else
          // BDD node 139:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  1448:ParserReject
          // BDD node 193:DROP
        }
        // EP node  271:Else
        // BDD node 137:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  4258:ParserReject
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
