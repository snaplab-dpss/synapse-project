#include <core.p4>

#if __TARGET_TOFINO__ == 2
  #include <t2na.p4>
  #define CPU_PCIE_PORT 0
  #define RECIRCULATION_PORT 6
#else
  #include <tna.p4>
  #define CPU_PCIE_PORT 192
  #define RECIRCULATION_PORT 320
#endif

header cpu_h {
  bit<16> code_path;
  bit<16> egress_dev;
  bit<104> vector_table_1074066176_144_get_value_param_0;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;

};

header hdr_0_h {
  bit<112> data;
}
header hdr_1_h {
  bit<160> data;
}
header hdr_2_h {
  bit<32> data;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  hdr_0_h hdr_0;
  hdr_1_h hdr_1;
  hdr_2_h hdr_2;

}

struct synapse_ingress_metadata_t {
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
    
    transition select(ig_intr_md.ingress_port) {
      CPU_PCIE_PORT: parse_cpu;
      RECIRCULATION_PORT: parse_recirc;
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
    pkt.extract(hdr.hdr_0);
    transition parser_135;
  }
  state parser_135 {
    transition select (hdr.hdr_0.data[111:96]) {
      2048: parser_136;
      default: parser_199;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr_1);
    transition parser_137;
  }
  state parser_199 {
    transition reject;
  }
  state parser_137 {
    transition select (hdr.hdr_1.data[79:72]) {
      6: parser_138;
      17: parser_138;
      default: parser_197;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr_2);
    transition parser_167;
  }
  state parser_197 {
    transition reject;
  }
  state parser_167 {
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

  Register<bit<32>,_>(32, 0) vector_register_1074084760_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074084760_0) vector_register_1074084760_0_read_84 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };

  bit<32> dchain_table_1074084336_142_key_0 = 32w0;
  table dchain_table_1074084336_142 {
    key = {
      dchain_table_1074084336_142_key_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 65536;
    idle_timeout = true;
  }

  bit<104> vector_table_1074066176_144_get_value_param_0 = 104w0;
  action vector_table_1074066176_144_get_value(bit<104> _vector_table_1074066176_144_get_value_param_0) {
    vector_table_1074066176_144_get_value_param_0 = _vector_table_1074066176_144_get_value_param_0;
  }

  bit<32> vector_table_1074066176_144_key_0 = 32w0;
  table vector_table_1074066176_144 {
    key = {
      vector_table_1074066176_144_key_0: exact;
    }
    actions = {
      vector_table_1074066176_144_get_value;
    }
    size = 65536;
  }

  bit<32> map_table_1074052352_165_get_value_param_0 = 32w0;
  action map_table_1074052352_165_get_value(bit<32> _map_table_1074052352_165_get_value_param_0) {
    map_table_1074052352_165_get_value_param_0 = _map_table_1074052352_165_get_value_param_0;
  }

  bit<8> map_table_1074052352_165_key_0 = 8w0;
  bit<64> map_table_1074052352_165_key_1 = 64w0;
  bit<32> map_table_1074052352_165_key_2 = 32w0;
  table map_table_1074052352_165 {
    key = {
      map_table_1074052352_165_key_0: exact;
      map_table_1074052352_165_key_1: exact;
      map_table_1074052352_165_key_2: exact;
    }
    actions = {
      map_table_1074052352_165_get_value;
    }
    size = 65536;
  }

  bit<32> dchain_table_1074084336_185_key_0 = 32w0;
  table dchain_table_1074084336_185 {
    key = {
      dchain_table_1074084336_185_key_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 65536;
    idle_timeout = true;
  }

  bit<16> vector_table_1074101976_187_get_value_param_0 = 16w0;
  action vector_table_1074101976_187_get_value(bit<16> _vector_table_1074101976_187_get_value_param_0) {
    vector_table_1074101976_187_get_value_param_0 = _vector_table_1074101976_187_get_value_param_0;
  }

  bit<32> vector_table_1074101976_187_key_0 = 32w0;
  table vector_table_1074101976_187 {
    key = {
      vector_table_1074101976_187_key_0: exact;
    }
    actions = {
      vector_table_1074101976_187_get_value;
    }
    size = 32;
  }


  apply {
    if (hdr.cpu.isValid()) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1
      // BDD node 133:expire_items_single_map(chain:(w64 1074084336), vector:(w64 1074066176), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074052352))
      // EP node  4
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074205864), length:(w32 14), chunk:(w64 1074216616)[ -> (w64 1073763072)])
      if(hdr.hdr_0.isValid()) {
        // EP node  11
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  12
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  20
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074205864), length:(w32 20), chunk:(w64 1074217352)[ -> (w64 1073763219)])
        if(hdr.hdr_1.isValid()) {
          // EP node  39
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 156) packet_chunks)) (Eq (w8 17) (Read w8 (w32 156) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  40
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 156) packet_chunks)) (Eq (w8 17) (Read w8 (w32 156) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  52
          // BDD node 138:packet_borrow_next_chunk(p:(w64 1074205864), length:(w32 4), chunk:(w64 1074218008)[ -> (w64 1073763366)])
          if(hdr.hdr_2.isValid()) {
            // EP node  84
            // BDD node 139:vector_borrow(vector:(w64 1074084760), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074218288)[ -> (w64 1074098656)])
            bit<32> vector_reg_value_0 = 32w0;
            vector_reg_value_0 = vector_register_1074084760_0_read_84.execute((bit<32>)(meta.dev[15:0]));
            // EP node  155
            // BDD node 140:vector_return(vector:(w64 1074084760), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074098656)[(ReadLSB w32 (w32 0) vector_data_512)])
            // EP node  232
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
            if ((32w0x00000000) == (vector_reg_value_0)) {
              // EP node  233
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  768
              // BDD node 142:dchain_is_index_allocated(chain:(w64 1074084336), index:(ZExt w32 (ReadLSB w16 (w32 296) packet_chunks)))
              dchain_table_1074084336_142_key_0 = (bit<32>)(hdr.hdr_2.data[31:16]);
              bool hit_0 = dchain_table_1074084336_142.apply().hit;
              // EP node  905
              // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
              if (hit_0) {
                // EP node  906
                // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
                // EP node  3840
                // BDD node 144:vector_borrow(vector:(w64 1074066176), index:(ZExt w32 (ReadLSB w16 (w32 296) packet_chunks)), val_out:(w64 1074218952)[ -> (w64 1074080072)])
                vector_table_1074066176_144_key_0 = (bit<32>)(hdr.hdr_2.data[31:16]);
                vector_table_1074066176_144.apply();
                // EP node  4118
                // BDD node 145:vector_return(vector:(w64 1074066176), index:(ZExt w32 (ReadLSB w16 (w32 296) packet_chunks)), value:(w64 1074080072)[(ReadLSB w104 (w32 0) vector_data_384)])
                // EP node  4198
                // BDD node 146:dchain_rejuvenate_index(chain:(w64 1074084336), index:(ZExt w32 (ReadLSB w16 (w32 296) packet_chunks)), time:(ReadLSB w64 (w32 0) next_time))
                hdr.cpu.vector_table_1074066176_144_get_value_param_0 = vector_table_1074066176_144_get_value_param_0;
                hdr.cpu.dev = meta.dev;
                send_to_controller(4198);
              } else {
                // EP node  907
                // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
                // EP node  1550
                // BDD node 164:DROP
                drop();
              }
            } else {
              // EP node  234
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_512))
              // EP node  426
              // BDD node 165:map_get(map:(w64 1074052352), key:(w64 1074215842)[(Concat w104 (Read w8 (w32 156) packet_chunks) (Concat w96 (Read w8 (w32 166) packet_chunks) (Concat w88 (Read w8 (w32 165) packet_chunks) (Concat w80 (Read w8 (w32 164) packet_chunks) (Concat w72 (Read w8 (w32 163) packet_chunks) (Concat w64 (Read w8 (w32 162) packet_chunks) (Concat w56 (Read w8 (w32 161) packet_chunks) (Concat w48 (Read w8 (w32 160) packet_chunks) (Concat w40 (Read w8 (w32 159) packet_chunks) (ReadLSB w32 (w32 294) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 156) packet_chunks) (Concat w96 (Read w8 (w32 166) packet_chunks) (Concat w88 (Read w8 (w32 165) packet_chunks) (Concat w80 (Read w8 (w32 164) packet_chunks) (Concat w72 (Read w8 (w32 163) packet_chunks) (Concat w64 (Read w8 (w32 162) packet_chunks) (Concat w56 (Read w8 (w32 161) packet_chunks) (Concat w48 (Read w8 (w32 160) packet_chunks) (Concat w40 (Read w8 (w32 159) packet_chunks) (ReadLSB w32 (w32 294) packet_chunks))))))))))], value_out:(w64 1074222624)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              map_table_1074052352_165_key_0 = hdr.hdr_1.data[79:72];
              map_table_1074052352_165_key_1 = hdr.hdr_1.data[159:96];
              hdr.hdr_2.data = hdr.hdr_2.data;
              bool hit_1 = map_table_1074052352_165.apply().hit;
              // EP node  533
              // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit_1) {
                // EP node  534
                // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  573
                // BDD node 167:dchain_allocate_new_index(chain:(w64 1074084336), index_out:(w64 1074223056)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                hdr.cpu.dev = meta.dev;
                send_to_controller(573);
              } else {
                // EP node  535
                // BDD node 166:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  1798
                // BDD node 185:dchain_rejuvenate_index(chain:(w64 1074084336), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074084336_185_key_0 = dchain_table_1074084336_185_key_0;
                dchain_table_1074084336_185.apply();
                // EP node  1971
                // BDD node 186:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763219), l4_header:(w64 1073763366), packet:(w64 1074154488))
                // EP node  2150
                // BDD node 187:vector_borrow(vector:(w64 1074101976), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074226696)[ -> (w64 1074115872)])
                vector_table_1074101976_187_key_0 = (bit<32>)(vector_table_1074101976_187_key_0[15:0]);
                vector_table_1074101976_187.apply();
                // EP node  2365
                // BDD node 188:vector_return(vector:(w64 1074101976), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074115872)[(ReadLSB w16 (w32 0) vector_data_640)])
                // EP node  2587
                // BDD node 189:packet_return_chunk(p:(w64 1074205864), the_chunk:(w64 1073763366)[(Concat w32 (Read w8 (w32 297) packet_chunks) (Concat w24 (Read w8 (w32 296) packet_chunks) (ReadLSB w16 (w32 0) allocated_index)))])
                hdr.hdr_2.data[7:0] = dchain_table_1074084336_185_key_0[7:0];
                hdr.hdr_2.data[15:8] = dchain_table_1074084336_185_key_0[15:8];
                // EP node  2785
                // BDD node 190:packet_return_chunk(p:(w64 1074205864), the_chunk:(w64 1073763219)[(Concat w160 (Read w8 (w32 166) packet_chunks) (Concat w152 (Read w8 (w32 165) packet_chunks) (Concat w144 (Read w8 (w32 164) packet_chunks) (Concat w136 (Read w8 (w32 163) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 147) packet_chunks)))))))))))])
                hdr.hdr_1.data[103:96] = 8w0x01;
                hdr.hdr_1.data[111:104] = 8w0x02;
                hdr.hdr_1.data[119:112] = 8w0x03;
                hdr.hdr_1.data[127:120] = 8w0x04;
                // EP node  3159
                // BDD node 192:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                if ((16w0xffff) != (vector_table_1074101976_187_get_value_param_0)) {
                  // EP node  3160
                  // BDD node 192:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                  // EP node  3416
                  // BDD node 193:FORWARD
                  nf_dev[15:0] = vector_table_1074101976_187_get_value_param_0;
                  trigger_forward = true;
                } else {
                  // EP node  3161
                  // BDD node 192:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                  // EP node  3644
                  // BDD node 194:DROP
                  drop();
                }
              }
            }
          }
          // EP node  41
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 156) packet_chunks)) (Eq (w8 17) (Read w8 (w32 156) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  340
          // BDD node 197:DROP
        }
        // EP node  13
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  1657
        // BDD node 199:DROP
      }
      ig_tm_md.bypass_egress = 1;
    }

    if (trigger_forward) {
      forward_nf_dev.apply();
    }
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
