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
  bit<32> vector_table_1074077160_139_get_value_param0;
  @padding bit<31> pad_hit1;
  bool hit1;
  bit<32> map_table_1074044752_159_get_value_param0;
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
  bit<16> key_16b_0;
  bit<16> key_16b_1;
  bit<32> key_32b_2;
  bit<32> key_32b_3;
  bit<8> key_8b_4;

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
      default: parser_201;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_201 {
    transition reject;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_138;
      8w0x11: parser_138;
      default: parser_199;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_147;
  }
  state parser_199 {
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

  bit<32> vector_table_1074077160_139_get_value_param0 = 32w0;
  action vector_table_1074077160_139_get_value(bit<32> _vector_table_1074077160_139_get_value_param0) {
    vector_table_1074077160_139_get_value_param0 = _vector_table_1074077160_139_get_value_param0;
  }

  table vector_table_1074077160_139 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074077160_139_get_value;
    }
    size = 36;
  }

  bit<32> map_table_1074044752_142_get_value_param0 = 32w0;
  action map_table_1074044752_142_get_value(bit<32> _map_table_1074044752_142_get_value_param0) {
    map_table_1074044752_142_get_value_param0 = _map_table_1074044752_142_get_value_param0;
  }

  table map_table_1074044752_142 {
    key = {
      meta.key_16b_0: exact;
      meta.key_16b_1: exact;
      meta.key_32b_2: exact;
      meta.key_32b_3: exact;
      meta.key_8b_4: exact;
    }
    actions = {
      map_table_1074044752_142_get_value;
    }
    size = 72818;
  }

  bit<16> vector_table_1074094376_149_get_value_param0 = 16w0;
  action vector_table_1074094376_149_get_value(bit<16> _vector_table_1074094376_149_get_value_param0) {
    vector_table_1074094376_149_get_value_param0 = _vector_table_1074094376_149_get_value_param0;
  }

  table vector_table_1074094376_149 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074094376_149_get_value;
    }
    size = 36;
  }

  table dchain_table_1074076736_148 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<32> map_table_1074044752_159_get_value_param0 = 32w0;
  action map_table_1074044752_159_get_value(bit<32> _map_table_1074044752_159_get_value_param0) {
    map_table_1074044752_159_get_value_param0 = _map_table_1074044752_159_get_value_param0;
  }

  table map_table_1074044752_159 {
    key = {
      meta.key_16b_0: exact;
      meta.key_16b_1: exact;
      meta.key_32b_2: exact;
      meta.key_32b_3: exact;
      meta.key_8b_4: exact;
    }
    actions = {
      map_table_1074044752_159_get_value;
    }
    size = 72818;
  }

  bit<16> vector_table_1074094376_187_get_value_param0 = 16w0;
  action vector_table_1074094376_187_get_value(bit<16> _vector_table_1074094376_187_get_value_param0) {
    vector_table_1074094376_187_get_value_param0 = _vector_table_1074094376_187_get_value_param0;
  }

  table vector_table_1074094376_187 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074094376_187_get_value;
    }
    size = 36;
  }

  table dchain_table_1074076736_186 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:Ignore
      // BDD node 133:expire_items_single_map(chain:(w64 1074076736), vector:(w64 1074058576), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074044752))
      // EP node  4:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 14), chunk:(w64 1074212456)[ -> (w64 1073762880)])
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  14:Then
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  26:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 20), chunk:(w64 1074213192)[ -> (w64 1073763136)])
        if(hdr.hdr1.isValid()) {
          // EP node  52:ParserCondition
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  53:Then
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  83:VectorTableLookup
          // BDD node 139:vector_borrow(vector:(w64 1074077160), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074214128)[ -> (w64 1074091056)])
          meta.key_32b_0 = meta.dev;
          vector_table_1074077160_139.apply();
          // EP node  188:ParserExtraction
          // BDD node 138:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 4), chunk:(w64 1074213848)[ -> (w64 1073763392)])
          if(hdr.hdr2.isValid()) {
            // EP node  294:Ignore
            // BDD node 140:vector_return(vector:(w64 1074077160), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074091056)[(ReadLSB w32 (w32 0) vector_data_r1)])
            // EP node  385:If
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
            if ((32w0x00000000) == (vector_table_1074077160_139_get_value_param0)){
              // EP node  386:Then
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  433:MapTableLookup
              // BDD node 142:map_get(map:(w64 1074044752), key:(w64 1074211753)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks))))))))))))], value_out:(w64 1074214760)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              meta.key_16b_0 = hdr.hdr2.data1;
              meta.key_16b_1 = hdr.hdr2.data0;
              meta.key_32b_2 = hdr.hdr1.data4;
              meta.key_32b_3 = hdr.hdr1.data3;
              meta.key_8b_4 = hdr.hdr1.data1;
              bool hit0 = map_table_1074044752_142.apply().hit;
              // EP node  566:If
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0){
                // EP node  567:Then
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  4321:Drop
                // BDD node 147:DROP
                fwd_op = fwd_op_t.DROP;
              } else {
                // EP node  568:Else
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  2446:VectorTableLookup
                // BDD node 149:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074215976)[ -> (w64 1074108272)])
                meta.key_32b_0 = meta.dev;
                vector_table_1074094376_149.apply();
                // EP node  2700:DchainTableLookup
                // BDD node 148:dchain_rejuvenate_index(chain:(w64 1074076736), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                meta.key_32b_0 = map_table_1074044752_142_get_value_param0;
                dchain_table_1074076736_148.apply();
                // EP node  2855:If
                // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r12)))
                if ((meta.dev[15:0]) != (vector_table_1074094376_149_get_value_param0)){
                  // EP node  2856:Then
                  // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r12)))
                  // EP node  3020:If
                  // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r12)))
                  if ((16w0xffff) != (vector_table_1074094376_149_get_value_param0)){
                    // EP node  3021:Then
                    // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r12)))
                    // EP node  3160:Ignore
                    // BDD node 150:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r12)])
                    // EP node  3801:Forward
                    // BDD node 156:FORWARD
                    nf_dev[15:0] = vector_table_1074094376_149_get_value_param0;
                  } else {
                    // EP node  3022:Else
                    // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r12)))
                    // EP node  5351:Ignore
                    // BDD node 270:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r12)])
                    // EP node  7602:Drop
                    // BDD node 157:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  2857:Else
                  // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r12)))
                  // EP node  5152:Ignore
                  // BDD node 266:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r12)])
                  // EP node  7470:Drop
                  // BDD node 158:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  387:Else
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  506:MapTableLookup
              // BDD node 159:map_get(map:(w64 1074044752), key:(w64 1074211730)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074218072)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              meta.key_16b_0 = hdr.hdr2.data0;
              meta.key_16b_1 = hdr.hdr2.data1;
              meta.key_32b_2 = hdr.hdr1.data3;
              meta.key_32b_3 = hdr.hdr1.data4;
              meta.key_8b_4 = hdr.hdr1.data1;
              bool hit1 = map_table_1074044752_159.apply().hit;
              // EP node  634:If
              // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit1){
                // EP node  635:Then
                // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  4424:SendToController
                // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                fwd_op = fwd_op_t.FORWARD_TO_CPU;
                build_cpu_hdr(4424);
                hdr.cpu.vector_table_1074077160_139_get_value_param0 = vector_table_1074077160_139_get_value_param0;
                hdr.cpu.hit1 = hit1;
                hdr.cpu.map_table_1074044752_159_get_value_param0 = map_table_1074044752_159_get_value_param0;
                hdr.cpu.dev = meta.dev;
              } else {
                // EP node  636:Else
                // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  788:VectorTableLookup
                // BDD node 187:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074223696)[ -> (w64 1074108272)])
                meta.key_32b_0 = meta.dev;
                vector_table_1074094376_187.apply();
                // EP node  1081:Ignore
                // BDD node 188:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r2)])
                // EP node  1261:DchainTableLookup
                // BDD node 186:dchain_rejuvenate_index(chain:(w64 1074076736), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                meta.key_32b_0 = map_table_1074044752_159_get_value_param0;
                dchain_table_1074076736_186.apply();
                // EP node  1796:If
                // BDD node 192:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r2)))
                if ((meta.dev[15:0]) != (vector_table_1074094376_187_get_value_param0)){
                  // EP node  1797:Then
                  // BDD node 192:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r2)))
                  // EP node  2151:If
                  // BDD node 193:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                  if ((16w0xffff) != (vector_table_1074094376_187_get_value_param0)){
                    // EP node  2152:Then
                    // BDD node 193:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                    // EP node  2344:Forward
                    // BDD node 194:FORWARD
                    nf_dev[15:0] = vector_table_1074094376_187_get_value_param0;
                  } else {
                    // EP node  2153:Else
                    // BDD node 193:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                    // EP node  5091:Drop
                    // BDD node 195:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  1798:Else
                  // BDD node 192:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r2)))
                  // EP node  5934:Drop
                  // BDD node 196:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            }
          }
          // EP node  54:Else
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  6484:ParserReject
          // BDD node 199:DROP
        }
        // EP node  15:Else
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  5618:ParserReject
        // BDD node 201:DROP
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
