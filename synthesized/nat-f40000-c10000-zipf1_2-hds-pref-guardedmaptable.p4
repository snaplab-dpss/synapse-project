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
  bit<32> vector_table_1074085576_139_get_value_param0;
  @padding bit<31> pad_hit1;
  bool hit1;
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
  bit<32> key_32b_1;
  bit<16> key_16b_2;
  bit<16> key_16b_3;
  bool hit1;
  bool guarded_map_table_1074053168_guard_allow0;

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
      default: parser_205;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_205 {
    transition reject;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_138;
      8w0x11: parser_138;
      default: parser_203;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_166;
  }
  state parser_203 {
    transition reject;
  }
  state parser_166 {
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

  bit<32> vector_table_1074085576_139_get_value_param0 = 32w0;
  action vector_table_1074085576_139_get_value(bit<32> _vector_table_1074085576_139_get_value_param0) {
    vector_table_1074085576_139_get_value_param0 = _vector_table_1074085576_139_get_value_param0;
  }

  table vector_table_1074085576_139 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074085576_139_get_value;
    }
    size = 36;
  }

  table dchain_table_1074085152_142 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  bit<96> vector_table_1074066992_144_get_value_param0 = 96w0;
  action vector_table_1074066992_144_get_value(bit<96> _vector_table_1074066992_144_get_value_param0) {
    vector_table_1074066992_144_get_value_param0 = _vector_table_1074066992_144_get_value_param0;
  }

  table vector_table_1074066992_144 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1074066992_144_get_value;
    }
    size = 72818;
  }

  table dchain_table_1074085152_146 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<16>,_>(32, 0) vector_register_1074102792_0;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074102792_0) vector_register_1074102792_0_read_3940 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  bit<32> guarded_map_table_1074053168_167_get_value_param0 = 32w0;
  action guarded_map_table_1074053168_167_get_value(bit<32> _guarded_map_table_1074053168_167_get_value_param0) {
    guarded_map_table_1074053168_167_get_value_param0 = _guarded_map_table_1074053168_167_get_value_param0;
  }

  table guarded_map_table_1074053168_167 {
    key = {
      meta.key_32b_0: exact;
      meta.key_32b_1: exact;
      meta.key_16b_2: exact;
      meta.key_16b_3: exact;
    }
    actions = {
      guarded_map_table_1074053168_167_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<8>,_>(1, 0) guarded_map_table_1074053168_guard;
  RegisterAction<bit<8>, bit<1>, bit<8>>(guarded_map_table_1074053168_guard) guarded_map_table_1074053168_guard_read_5730 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
    }
  };

  bit<8> guarded_map_table_1074053168_guard_value_1690 = 0;
  action guarded_map_table_1074053168_guard_check_169() {
    guarded_map_table_1074053168_guard_value_1690 = guarded_map_table_1074053168_guard_read_5730.execute(0);
  }

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074102792_0) vector_register_1074102792_0_read_670 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  table dchain_table_1074085152_189 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
       NoAction;
    }
    size = 72818;
    idle_timeout = true;
  }

  table dchain_table_1074085152_273 {
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
      // BDD node 133:expire_items_single_map(chain:(w64 1074085152), vector:(w64 1074066992), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074053168))
      // EP node  4:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074210520), length:(w32 14), chunk:(w64 1074221272)[ -> (w64 1073763344)])
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  14:Then
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  26:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074210520), length:(w32 20), chunk:(w64 1074222008)[ -> (w64 1073763600)])
        if(hdr.hdr1.isValid()) {
          // EP node  52:ParserCondition
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  53:Then
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  83:VectorTableLookup
          // BDD node 139:vector_borrow(vector:(w64 1074085576), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074222944)[ -> (w64 1074099472)])
          meta.key_32b_0 = meta.dev;
          vector_table_1074085576_139.apply();
          // EP node  188:ParserExtraction
          // BDD node 138:packet_borrow_next_chunk(p:(w64 1074210520), length:(w32 4), chunk:(w64 1074222664)[ -> (w64 1073763856)])
          if(hdr.hdr2.isValid()) {
            // EP node  294:Ignore
            // BDD node 140:vector_return(vector:(w64 1074085576), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074099472)[(ReadLSB w32 (w32 0) vector_data_r1)])
            // EP node  385:If
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
            if ((32w0x00000000) == (vector_table_1074085576_139_get_value_param0)){
              // EP node  386:Then
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  1834:DchainTableLookup
              // BDD node 142:dchain_is_index_allocated(chain:(w64 1074085152), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)))
              meta.key_32b_0 = (bit<32>)(bswap16(hdr.hdr2.data1));
              bool hit0 = dchain_table_1074085152_142.apply().hit;
              // EP node  1942:If
              // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
              if (hit0){
                // EP node  1943:Then
                // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
                // EP node  2131:VectorTableLookup
                // BDD node 144:vector_borrow(vector:(w64 1074066992), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)), val_out:(w64 1074223608)[ -> (w64 1074080888)])
                meta.key_32b_0 = (bit<32>)(bswap16(hdr.hdr2.data1));
                vector_table_1074066992_144.apply();
                // EP node  2517:DchainTableLookup
                // BDD node 146:dchain_rejuvenate_index(chain:(w64 1074085152), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)), time:(ReadLSB w64 (w32 0) next_time))
                meta.key_32b_0 = (bit<32>)(bswap16(hdr.hdr2.data1));
                dchain_table_1074085152_146.apply();
                // EP node  2723:Ignore
                // BDD node 145:vector_return(vector:(w64 1074066992), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)), value:(w64 1074080888)[(ReadLSB w96 (w32 0) vector_data_384)])
                // EP node  2855:If
                // BDD node 147:if ((And (Eq (ReadLSB w32 (w32 4) vector_data_384) (ReadLSB w32 (w32 268) packet_chunks)) (Eq (ReadLSB w16 (w32 10) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks)))
                bool cond0 = false;
                if ((vector_table_1074066992_144_get_value_param0[63:32]) == (hdr.hdr1.data3)){
                  if ((vector_table_1074066992_144_get_value_param0[15:0]) == (hdr.hdr2.data0)){
                    cond0 = true;
                  }
                }
                if (cond0) {
                  // EP node  2856:Then
                  // BDD node 147:if ((And (Eq (ReadLSB w32 (w32 4) vector_data_384) (ReadLSB w32 (w32 268) packet_chunks)) (Eq (ReadLSB w16 (w32 10) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks)))
                  // EP node  3789:Ignore
                  // BDD node 148:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763600), l4_header:(w64 1073763856), packet:(w64 1074155304))
                  // EP node  3940:VectorRegisterLookup
                  // BDD node 149:vector_borrow(vector:(w64 1074102792), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074224512)[ -> (w64 1074116688)])
                  bit<16> vector_reg_value0 = vector_register_1074102792_0_read_3940.execute(meta.dev);
                  // EP node  4185:Ignore
                  // BDD node 150:vector_return(vector:(w64 1074102792), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116688)[(ReadLSB w16 (w32 0) vector_data_640)])
                  // EP node  4388:If
                  // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                  if ((meta.dev[15:0]) != (vector_reg_value0)){
                    // EP node  4389:Then
                    // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  4700:ModifyHeader
                    // BDD node 151:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 9) vector_data_384) (Concat w24 (Read w8 (w32 8) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks)))])
                    hdr.hdr2.data1[15:8] = vector_table_1074066992_144_get_value_param0[31:24];
                    hdr.hdr2.data1[7:0] = vector_table_1074066992_144_get_value_param0[23:16];
                    // EP node  4975:ModifyHeader
                    // BDD node 152:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 3) vector_data_384) (Concat w152 (Read w8 (w32 2) vector_data_384) (Concat w144 (Read w8 (w32 1) vector_data_384) (Concat w136 (Read w8 (w32 0) vector_data_384) (Concat w128 (Read w8 (w32 271) packet_chunks) (Concat w120 (Read w8 (w32 270) packet_chunks) (Concat w112 (Read w8 (w32 269) packet_chunks) (Concat w104 (Read w8 (w32 268) packet_chunks) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                    hdr.hdr1.data4[31:24] = vector_table_1074066992_144_get_value_param0[95:88];
                    hdr.hdr1.data4[23:16] = vector_table_1074066992_144_get_value_param0[87:80];
                    hdr.hdr1.data4[15:8] = vector_table_1074066992_144_get_value_param0[79:72];
                    hdr.hdr1.data4[7:0] = vector_table_1074066992_144_get_value_param0[71:64];
                    // EP node  5381:If
                    // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                    if ((16w0xffff) != (vector_reg_value0)){
                      // EP node  5382:Then
                      // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                      // EP node  5618:Forward
                      // BDD node 156:FORWARD
                      nf_dev[15:0] = vector_reg_value0;
                    } else {
                      // EP node  5383:Else
                      // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                      // EP node  7693:Drop
                      // BDD node 157:DROP
                      fwd_op = fwd_op_t.DROP;
                    }
                  } else {
                    // EP node  4390:Else
                    // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  7426:ModifyHeader
                    // BDD node 281:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 9) vector_data_384) (Concat w24 (Read w8 (w32 8) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks)))])
                    hdr.hdr2.data1[15:8] = vector_table_1074066992_144_get_value_param0[31:24];
                    hdr.hdr2.data1[7:0] = vector_table_1074066992_144_get_value_param0[23:16];
                    // EP node  9365:ModifyHeader
                    // BDD node 282:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 3) vector_data_384) (Concat w152 (Read w8 (w32 2) vector_data_384) (Concat w144 (Read w8 (w32 1) vector_data_384) (Concat w136 (Read w8 (w32 0) vector_data_384) (Concat w128 (Read w8 (w32 271) packet_chunks) (Concat w120 (Read w8 (w32 270) packet_chunks) (Concat w112 (Read w8 (w32 269) packet_chunks) (Concat w104 (Read w8 (w32 268) packet_chunks) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                    hdr.hdr1.data4[31:24] = vector_table_1074066992_144_get_value_param0[95:88];
                    hdr.hdr1.data4[23:16] = vector_table_1074066992_144_get_value_param0[87:80];
                    hdr.hdr1.data4[15:8] = vector_table_1074066992_144_get_value_param0[79:72];
                    hdr.hdr1.data4[7:0] = vector_table_1074066992_144_get_value_param0[71:64];
                    // EP node  12174:Drop
                    // BDD node 158:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  2857:Else
                  // BDD node 147:if ((And (Eq (ReadLSB w32 (w32 4) vector_data_384) (ReadLSB w32 (w32 268) packet_chunks)) (Eq (ReadLSB w16 (w32 10) vector_data_384) (ReadLSB w16 (w32 512) packet_chunks)))
                  // EP node  3657:Drop
                  // BDD node 162:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              } else {
                // EP node  1944:Else
                // BDD node 143:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) is_index_allocated)))
                // EP node  11990:Drop
                // BDD node 166:DROP
                fwd_op = fwd_op_t.DROP;
              }
            } else {
              // EP node  387:Else
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  449:GuardedMapTableLookup
              // BDD node 167:map_get(map:(w64 1074053168), key:(w64 1074220498)[(Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks))))) -> (Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks)))))], value_out:(w64 1074227920)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              meta.key_32b_0 = hdr.hdr1.data3;
              meta.key_32b_1 = hdr.hdr1.data4;
              meta.key_16b_2 = hdr.hdr2.data0;
              meta.key_16b_3 = hdr.hdr2.data1;
              meta.hit1 = guarded_map_table_1074053168_167.apply().hit;
              // EP node  506:If
              // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!meta.hit1){
                // EP node  507:Then
                // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  5730:GuardedMapTableGuardCheck
                // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                guarded_map_table_1074053168_guard_check_169();
                meta.guarded_map_table_1074053168_guard_allow0 = false;
                if (guarded_map_table_1074053168_guard_value_1690 != 0) {
                  meta.guarded_map_table_1074053168_guard_allow0 = true;
                }
                // EP node  5731:If
                // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                if (meta.guarded_map_table_1074053168_guard_allow0){
                  // EP node  5732:Then
                  // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  5940:SendToController
                  // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(5940);
                  hdr.cpu.vector_table_1074085576_139_get_value_param0 = vector_table_1074085576_139_get_value_param0;
                  hdr.cpu.hit1 = meta.hit1;
                  hdr.cpu.dev = meta.dev;
                } else {
                  // EP node  5733:Else
                  // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  12360:Drop
                  // BDD node 290:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              } else {
                // EP node  508:Else
                // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  670:VectorRegisterLookup
                // BDD node 191:vector_borrow(vector:(w64 1074102792), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074232640)[ -> (w64 1074116688)])
                bit<16> vector_reg_value1 = vector_register_1074102792_0_read_670.execute(meta.dev);
                // EP node  826:Ignore
                // BDD node 192:vector_return(vector:(w64 1074102792), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116688)[(ReadLSB w16 (w32 0) vector_data_r2)])
                // EP node  924:If
                // BDD node 196:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r2)))
                if ((meta.dev[15:0]) != (vector_reg_value1)){
                  // EP node  925:Then
                  // BDD node 196:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r2)))
                  // EP node  1057:DchainTableLookup
                  // BDD node 189:dchain_rejuvenate_index(chain:(w64 1074085152), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  meta.key_32b_0 = guarded_map_table_1074053168_167_get_value_param0;
                  dchain_table_1074085152_189.apply();
                  // EP node  1201:Ignore
                  // BDD node 190:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763600), l4_header:(w64 1073763856), packet:(w64 1074155304))
                  // EP node  1323:If
                  // BDD node 197:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                  if ((16w0xffff) != (vector_reg_value1)){
                    // EP node  1324:Then
                    // BDD node 197:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                    // EP node  1455:ModifyHeader
                    // BDD node 193:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) allocated_index)))])
                    hdr.hdr2.data0[15:8] = guarded_map_table_1074053168_167_get_value_param0[7:0];
                    hdr.hdr2.data0[7:0] = guarded_map_table_1074053168_167_get_value_param0[15:8];
                    // EP node  1568:ModifyHeader
                    // BDD node 194:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                    hdr.hdr1.data3[31:24] = 8w0x01;
                    hdr.hdr1.data3[23:16] = 8w0x02;
                    hdr.hdr1.data3[15:8] = 8w0x03;
                    hdr.hdr1.data3[7:0] = 8w0x04;
                    // EP node  1800:Forward
                    // BDD node 198:FORWARD
                    nf_dev[15:0] = vector_reg_value1;
                  } else {
                    // EP node  1325:Else
                    // BDD node 197:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r2)))
                    // EP node  6908:ModifyHeader
                    // BDD node 278:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) allocated_index)))])
                    hdr.hdr2.data0[15:8] = guarded_map_table_1074053168_167_get_value_param0[7:0];
                    hdr.hdr2.data0[7:0] = guarded_map_table_1074053168_167_get_value_param0[15:8];
                    // EP node  8817:ModifyHeader
                    // BDD node 279:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                    hdr.hdr1.data3[31:24] = 8w0x01;
                    hdr.hdr1.data3[23:16] = 8w0x02;
                    hdr.hdr1.data3[15:8] = 8w0x03;
                    hdr.hdr1.data3[7:0] = 8w0x04;
                    // EP node  11808:Drop
                    // BDD node 199:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  926:Else
                  // BDD node 196:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r2)))
                  // EP node  6574:DchainTableLookup
                  // BDD node 273:dchain_rejuvenate_index(chain:(w64 1074085152), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  meta.key_32b_0 = guarded_map_table_1074053168_167_get_value_param0;
                  dchain_table_1074085152_273.apply();
                  // EP node  8463:Ignore
                  // BDD node 274:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763600), l4_header:(w64 1073763856), packet:(w64 1074155304))
                  // EP node  10101:ModifyHeader
                  // BDD node 275:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) allocated_index)))])
                  hdr.hdr2.data0[15:8] = guarded_map_table_1074053168_167_get_value_param0[7:0];
                  hdr.hdr2.data0[7:0] = guarded_map_table_1074053168_167_get_value_param0[15:8];
                  // EP node  11522:ModifyHeader
                  // BDD node 276:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                  hdr.hdr1.data3[31:24] = 8w0x01;
                  hdr.hdr1.data3[23:16] = 8w0x02;
                  hdr.hdr1.data3[15:8] = 8w0x03;
                  hdr.hdr1.data3[7:0] = 8w0x04;
                  // EP node  12846:Drop
                  // BDD node 200:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            }
          }
          // EP node  54:Else
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  9925:ParserReject
          // BDD node 203:DROP
        }
        // EP node  15:Else
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  8125:ParserReject
        // BDD node 205:DROP
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
