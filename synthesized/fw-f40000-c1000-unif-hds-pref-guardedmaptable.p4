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
  bit<32> guarded_map_table_1074044752_159_get_value_param0;
  bit<32> dev;
  bit<64> time;

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

  bit<32> vector_table_1074077160_139_key0 = 32w0;
  table vector_table_1074077160_139 {
    key = {
      vector_table_1074077160_139_key0: exact;
    }
    actions = {
      vector_table_1074077160_139_get_value;
    }
    size = 36;
  }

  bit<32> guarded_map_table_1074044752_142_get_value_param0 = 32w0;
  action guarded_map_table_1074044752_142_get_value(bit<32> _guarded_map_table_1074044752_142_get_value_param0) {
    guarded_map_table_1074044752_142_get_value_param0 = _guarded_map_table_1074044752_142_get_value_param0;
  }

  bit<16> guarded_map_table_1074044752_142_key0 = 16w0;
  bit<16> guarded_map_table_1074044752_142_key1 = 16w0;
  bit<32> guarded_map_table_1074044752_142_key2 = 32w0;
  bit<32> guarded_map_table_1074044752_142_key3 = 32w0;
  bit<8> guarded_map_table_1074044752_142_key4 = 8w0;
  table guarded_map_table_1074044752_142 {
    key = {
      guarded_map_table_1074044752_142_key0: exact;
      guarded_map_table_1074044752_142_key1: exact;
      guarded_map_table_1074044752_142_key2: exact;
      guarded_map_table_1074044752_142_key3: exact;
      guarded_map_table_1074044752_142_key4: exact;
    }
    actions = {
      guarded_map_table_1074044752_142_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<8>,_>(1, 0) guarded_map_table_1074044752_guard;
  bit<16> vector_table_1074094376_149_get_value_param0 = 16w0;
  action vector_table_1074094376_149_get_value(bit<16> _vector_table_1074094376_149_get_value_param0) {
    vector_table_1074094376_149_get_value_param0 = _vector_table_1074094376_149_get_value_param0;
  }

  bit<32> vector_table_1074094376_149_key0 = 32w0;
  table vector_table_1074094376_149 {
    key = {
      vector_table_1074094376_149_key0: exact;
    }
    actions = {
      vector_table_1074094376_149_get_value;
    }
    size = 36;
  }

  bit<32> guarded_map_table_1074044752_159_get_value_param0 = 32w0;
  action guarded_map_table_1074044752_159_get_value(bit<32> _guarded_map_table_1074044752_159_get_value_param0) {
    guarded_map_table_1074044752_159_get_value_param0 = _guarded_map_table_1074044752_159_get_value_param0;
  }

  bit<16> guarded_map_table_1074044752_159_key0 = 16w0;
  bit<16> guarded_map_table_1074044752_159_key1 = 16w0;
  bit<32> guarded_map_table_1074044752_159_key2 = 32w0;
  bit<32> guarded_map_table_1074044752_159_key3 = 32w0;
  bit<8> guarded_map_table_1074044752_159_key4 = 8w0;
  table guarded_map_table_1074044752_159 {
    key = {
      guarded_map_table_1074044752_159_key0: exact;
      guarded_map_table_1074044752_159_key1: exact;
      guarded_map_table_1074044752_159_key2: exact;
      guarded_map_table_1074044752_159_key3: exact;
      guarded_map_table_1074044752_159_key4: exact;
    }
    actions = {
      guarded_map_table_1074044752_159_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  RegisterAction<bit<8>, bit<1>, bit<8>>(guarded_map_table_1074044752_guard) guarded_map_table_1074044752_guard_read_4587 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
    }
  };

  bit<8> guarded_map_table_1074044752_guard_value_1610 = 0;
  action guarded_map_table_1074044752_guard_check_161() {
    guarded_map_table_1074044752_guard_value_1610 = guarded_map_table_1074044752_guard_read_4587.execute(0);
  }
  bit<16> vector_table_1074094376_277_get_value_param0 = 16w0;
  action vector_table_1074094376_277_get_value(bit<16> _vector_table_1074094376_277_get_value_param0) {
    vector_table_1074094376_277_get_value_param0 = _vector_table_1074094376_277_get_value_param0;
  }

  bit<32> vector_table_1074094376_277_key0 = 32w0;
  table vector_table_1074094376_277 {
    key = {
      vector_table_1074094376_277_key0: exact;
    }
    actions = {
      vector_table_1074094376_277_get_value;
    }
    size = 36;
  }

  bit<16> vector_table_1074094376_187_get_value_param0 = 16w0;
  action vector_table_1074094376_187_get_value(bit<16> _vector_table_1074094376_187_get_value_param0) {
    vector_table_1074094376_187_get_value_param0 = _vector_table_1074094376_187_get_value_param0;
  }

  bit<32> vector_table_1074094376_187_key0 = 32w0;
  table vector_table_1074094376_187 {
    key = {
      vector_table_1074094376_187_key0: exact;
    }
    actions = {
      vector_table_1074094376_187_get_value;
    }
    size = 36;
  }

  bit<32> dchain_table_1074076736_186_key0 = 32w0;
  table dchain_table_1074076736_186 {
    key = {
      dchain_table_1074076736_186_key0: exact;
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
      // EP node  1:Ignore
      // BDD node 133:expire_items_single_map(chain:(w64 1074076736), vector:(w64 1074058576), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074044752))
      // EP node  6:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 14), chunk:(w64 1074212456)[ -> (w64 1073762880)])
      if(hdr.hdr0.isValid()) {
        // EP node  17:ParserCondition
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  18:Then
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  33:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 20), chunk:(w64 1074213192)[ -> (w64 1073763136)])
        if(hdr.hdr1.isValid()) {
          // EP node  61:ParserCondition
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  62:Then
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  95:VectorTableLookup
          // BDD node 139:vector_borrow(vector:(w64 1074077160), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074214128)[ -> (w64 1074091056)])
          vector_table_1074077160_139_key0 = meta.dev;
          vector_table_1074077160_139.apply();
          // EP node  202:ParserExtraction
          // BDD node 138:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 4), chunk:(w64 1074213848)[ -> (w64 1073763392)])
          if(hdr.hdr2.isValid()) {
            // EP node  264:Ignore
            // BDD node 140:vector_return(vector:(w64 1074077160), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074091056)[(ReadLSB w32 (w32 0) vector_data_r1)])
            // EP node  306:If
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
            if ((32w0x00000000) == (vector_table_1074077160_139_get_value_param0)) {
              // EP node  307:Then
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  377:GuardedMapTableLookup
              // BDD node 142:map_get(map:(w64 1074044752), key:(w64 1074211753)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks))))))))))))], value_out:(w64 1074214760)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              guarded_map_table_1074044752_142_key0 = hdr.hdr2.data1;
              guarded_map_table_1074044752_142_key1 = hdr.hdr2.data0;
              guarded_map_table_1074044752_142_key2 = hdr.hdr1.data4;
              guarded_map_table_1074044752_142_key3 = hdr.hdr1.data3;
              guarded_map_table_1074044752_142_key4 = hdr.hdr1.data1;
              bool hit0 = guarded_map_table_1074044752_142.apply().hit;
              // EP node  641:If
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0) {
                // EP node  642:Then
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  4439:Drop
                // BDD node 147:DROP
                fwd_op = fwd_op_t.DROP;
              } else {
                // EP node  643:Else
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  2541:VectorTableLookup
                // BDD node 149:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074215976)[ -> (w64 1074108272)])
                vector_table_1074094376_149_key0 = meta.dev;
                vector_table_1074094376_149.apply();
                // EP node  2797:Ignore
                // BDD node 148:dchain_rejuvenate_index(chain:(w64 1074076736), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                // EP node  3023:If
                // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r12)))
                if ((meta.dev[15:0]) != (vector_table_1074094376_149_get_value_param0)) {
                  // EP node  3024:Then
                  // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r12)))
                  // EP node  3195:If
                  // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r12)))
                  if ((16w0xffff) != (vector_table_1074094376_149_get_value_param0)) {
                    // EP node  3196:Then
                    // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r12)))
                    // EP node  3342:Ignore
                    // BDD node 150:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r12)])
                    // EP node  3919:Forward
                    // BDD node 156:FORWARD
                    nf_dev[15:0] = vector_table_1074094376_149_get_value_param0;
                  } else {
                    // EP node  3197:Else
                    // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r12)))
                    // EP node  6220:Ignore
                    // BDD node 270:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r12)])
                    // EP node  11129:Drop
                    // BDD node 157:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  3025:Else
                  // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r12)))
                  // EP node  5989:Ignore
                  // BDD node 266:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r12)])
                  // EP node  10959:Drop
                  // BDD node 158:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  308:Else
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  579:GuardedMapTableLookup
              // BDD node 159:map_get(map:(w64 1074044752), key:(w64 1074211730)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074218072)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              guarded_map_table_1074044752_159_key0 = hdr.hdr2.data0;
              guarded_map_table_1074044752_159_key1 = hdr.hdr2.data1;
              guarded_map_table_1074044752_159_key2 = hdr.hdr1.data3;
              guarded_map_table_1074044752_159_key3 = hdr.hdr1.data4;
              guarded_map_table_1074044752_159_key4 = hdr.hdr1.data1;
              bool hit1 = guarded_map_table_1074044752_159.apply().hit;
              // EP node  742:If
              // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit1) {
                // EP node  743:Then
                // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  4587:GuardedMapTableGuardCheck
                // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                guarded_map_table_1074044752_guard_check_161();
                bool guarded_map_table_1074044752_guard_allow0 = false;
                if (guarded_map_table_1074044752_guard_value_1610 != 0) {
                  guarded_map_table_1074044752_guard_allow0 = true;
                }
                // EP node  4588:If
                // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                if (guarded_map_table_1074044752_guard_allow0) {
                  // EP node  4589:Then
                  // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  4657:SendToController
                  // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(4657);
                  hdr.cpu.vector_table_1074077160_139_get_value_param0 = vector_table_1074077160_139_get_value_param0;
                  hdr.cpu.hit1 = hit1;
                  hdr.cpu.guarded_map_table_1074044752_159_get_value_param0 = guarded_map_table_1074044752_159_get_value_param0;
                  hdr.cpu.dev = meta.dev;
                  hdr.cpu.time[47:16] = meta.time;
                } else {
                  // EP node  4590:Else
                  // BDD node 161:dchain_allocate_new_index(chain:(w64 1074076736), index_out:(w64 1074218072)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  6450:VectorTableLookup
                  // BDD node 277:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074218536)[ -> (w64 1074108272)])
                  vector_table_1074094376_277_key0 = meta.dev;
                  vector_table_1074094376_277.apply();
                  // EP node  7923:If
                  // BDD node 282:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                  if ((meta.dev[15:0]) != (vector_table_1074094376_277_get_value_param0)) {
                    // EP node  7924:Then
                    // BDD node 282:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  9093:If
                    // BDD node 283:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                    if ((16w0xffff) != (vector_table_1074094376_277_get_value_param0)) {
                      // EP node  9094:Then
                      // BDD node 283:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                      // EP node  10160:Ignore
                      // BDD node 278:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_640)])
                      // EP node  13739:Forward
                      // BDD node 284:FORWARD
                      nf_dev[15:0] = vector_table_1074094376_277_get_value_param0;
                    } else {
                      // EP node  9095:Else
                      // BDD node 283:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                      // EP node  10435:Ignore
                      // BDD node 306:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_640)])
                      // EP node  13915:Drop
                      // BDD node 285:DROP
                      fwd_op = fwd_op_t.DROP;
                    }
                  } else {
                    // EP node  7925:Else
                    // BDD node 282:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  9358:Ignore
                    // BDD node 302:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_640)])
                    // EP node  13565:Drop
                    // BDD node 286:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                }
              } else {
                // EP node  744:Else
                // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  910:VectorTableLookup
                // BDD node 187:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074223696)[ -> (w64 1074108272)])
                vector_table_1074094376_187_key0 = meta.dev;
                vector_table_1074094376_187.apply();
                // EP node  1207:Ignore
                // BDD node 188:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_r3)])
                // EP node  1389:DchainTableLookup
                // BDD node 186:dchain_rejuvenate_index(chain:(w64 1074076736), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                dchain_table_1074076736_186_key0 = guarded_map_table_1074044752_159_get_value_param0;
                dchain_table_1074076736_186.apply();
                // EP node  1926:If
                // BDD node 192:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r3)))
                if ((meta.dev[15:0]) != (vector_table_1074094376_187_get_value_param0)) {
                  // EP node  1927:Then
                  // BDD node 192:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r3)))
                  // EP node  2295:If
                  // BDD node 193:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r3)))
                  if ((16w0xffff) != (vector_table_1074094376_187_get_value_param0)) {
                    // EP node  2296:Then
                    // BDD node 193:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r3)))
                    // EP node  2392:Forward
                    // BDD node 194:FORWARD
                    nf_dev[15:0] = vector_table_1074094376_187_get_value_param0;
                  } else {
                    // EP node  2297:Else
                    // BDD node 193:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r3)))
                    // EP node  5832:Drop
                    // BDD node 195:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  1928:Else
                  // BDD node 192:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r3)))
                  // EP node  7030:Drop
                  // BDD node 196:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            }
          }
          // EP node  63:Else
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  8152:ParserReject
          // BDD node 199:DROP
        }
        // EP node  19:Else
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  6664:ParserReject
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
