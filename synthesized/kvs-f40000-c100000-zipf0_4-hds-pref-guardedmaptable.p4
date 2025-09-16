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
  @padding bit<31> pad_hit0;
  bool hit0;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> guarded_map_table_1073923800_13_get_value_param0;
  @padding bit<7> pad_hit0;
  bool hit0;
  @padding bit<7> pad_guarded_map_table_1073923800_guard_allow0;
  bool guarded_map_table_1073923800_guard_allow0;

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
  bit<48> data0;
  bit<48> data1;
  bit<16> data2;
}
header hdr1_h {
  bit<72> data0;
  bit<24> data1;
  bit<32> data2;
  bit<32> data3;
}
header hdr2_h {
  bit<16> data0;
  bit<16> data1;
  bit<32> data2;
}
header hdr3_h {
  bit<8> data0;
  bit<32> data1;
  bit<32> data2;
  bit<8> data3;
  bit<16> data4;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  bool hit0;
  bool guarded_map_table_1073923800_guard_allow0;

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
    transition parser_6;
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr0.data2) {
      16w0x0800: parser_7;
      default: parser_66;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr1);
    transition parser_8;
  }
  state parser_66 {
    transition reject;
  }
  state parser_8 {
    transition parser_8_0;
  }
  state parser_8_0 {
    transition select (hdr.hdr1.data1[23:16]) {
      8w0x11: parser_9;
      default: parser_64;
    }
  }
  state parser_9 {
    pkt.extract(hdr.hdr2);
    transition parser_10;
  }
  state parser_64 {
    transition reject;
  }
  state parser_10 {
    transition parser_10_0;
  }
  state parser_10_0 {
    transition select (hdr.hdr2.data0) {
      16w0x029e: parser_11;
      default: parser_10_1;
    }
  }
  state parser_10_1 {
    transition select (hdr.hdr2.data1) {
      16w0x029e: parser_11;
      default: parser_61;
    }
  }
  state parser_11 {
    pkt.extract(hdr.hdr3);
    transition parser_57;
  }
  state parser_61 {
    transition reject;
  }
  state parser_57 {
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

  bit<32> guarded_map_table_1073923800_13_get_value_param0 = 32w0;
  action guarded_map_table_1073923800_13_get_value(bit<32> _guarded_map_table_1073923800_13_get_value_param0) {
    guarded_map_table_1073923800_13_get_value_param0 = _guarded_map_table_1073923800_13_get_value_param0;
  }

  bit<32> guarded_map_table_1073923800_13_key0 = 32w0;
  table guarded_map_table_1073923800_13 {
    key = {
      guarded_map_table_1073923800_13_key0: exact;
    }
    actions = {
      guarded_map_table_1073923800_13_get_value;
    }
    size = 9103;
  }

  Register<bit<8>,_>(1, 0) guarded_map_table_1073923800_guard;
  RegisterAction<bit<8>, bit<1>, bit<8>>(guarded_map_table_1073923800_guard) guarded_map_table_1073923800_guard_read_2494 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = value;
    }
  };

  bit<8> guarded_map_table_1073923800_guard_value_160 = 0;
  action guarded_map_table_1073923800_guard_check_16() {
    guarded_map_table_1073923800_guard_value_160 = guarded_map_table_1073923800_guard_read_2494.execute(0);
  }
  bit<32> dchain_table_1073971976_38_key0 = 32w0;
  table dchain_table_1073971976_38 {
    key = {
      dchain_table_1073971976_38_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 9103;
    idle_timeout = true;
  }

  Register<bit<32>,_>(8192, 0) vector_register_1073954840_0;

  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073954840_0) vector_register_1073954840_0_write_6264 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2;
    }
  };


  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073954840_0) vector_register_1073954840_0_read_1671 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };



  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  7306:SendToController
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        fwd_op = fwd_op_t.FORWARD_TO_CPU;
        build_cpu_hdr(7306);
        hdr.cpu.hit0 = hdr.recirc.hit0;
        hdr.cpu.dev = meta.dev;
      }

    } else {
      // EP node  0:Ignore
      // BDD node 4:expire_items_single_map(chain:(w64 1073971976), vector:(w64 1073937624), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073923800))
      // EP node  4:ParserExtraction
      // BDD node 5:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 14), chunk:(w64 1074039328)[ -> (w64 1073760160)])
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  14:Then
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  26:ParserExtraction
        // BDD node 7:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 20), chunk:(w64 1074040064)[ -> (w64 1073760416)])
        if(hdr.hdr1.isValid()) {
          // EP node  52:ParserCondition
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  53:Then
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  74:ParserExtraction
          // BDD node 9:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 8), chunk:(w64 1074040720)[ -> (w64 1073760672)])
          if(hdr.hdr2.isValid()) {
            // EP node  117:ParserCondition
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  118:Then
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  175:ParserExtraction
            // BDD node 11:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 12), chunk:(w64 1074041464)[ -> (w64 1073760928)])
            if(hdr.hdr3.isValid()) {
              // EP node  250:If
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              if ((16w0x0000) != (meta.dev[15:0])){
                // EP node  251:Then
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  322:GuardedMapTableLookup
                // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                guarded_map_table_1073923800_13_key0 = hdr.hdr3.data1;
                meta.hit0 = guarded_map_table_1073923800_13.apply().hit;
                // EP node  574:If
                // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                if (!meta.hit0){
                  // EP node  575:Then
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  645:If
                  // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr3.data0)){
                    // EP node  646:Then
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  2494:GuardedMapTableGuardCheck
                    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    guarded_map_table_1073923800_guard_check_16();
                    meta.guarded_map_table_1073923800_guard_allow0 = false;
                    if (guarded_map_table_1073923800_guard_value_160 != 0) {
                      meta.guarded_map_table_1073923800_guard_allow0 = true;
                    }
                    // EP node  2495:If
                    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    if (meta.guarded_map_table_1073923800_guard_allow0){
                      // EP node  2496:Then
                      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                      // EP node  7159:Recirculate
                      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                      fwd_op = fwd_op_t.RECIRCULATE;
                      build_recirc_hdr(0);
                      hdr.recirc.guarded_map_table_1073923800_13_get_value_param0 = guarded_map_table_1073923800_13_get_value_param0;
                      hdr.recirc.hit0 = meta.hit0;
                      hdr.recirc.guarded_map_table_1073923800_guard_allow0 = meta.guarded_map_table_1073923800_guard_allow0;
                    } else {
                      // EP node  2497:Else
                      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                      // EP node  5425:ModifyHeader
                      // BDD node 128:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
                      hdr.hdr3.data4[15:8] = meta.dev[7:0];
                      hdr.hdr3.data4[7:0] = meta.dev[15:8];
                      // EP node  6025:Forward
                      // BDD node 132:FORWARD
                      nf_dev[15:0] = 16w0x0000;
                    }
                  } else {
                    // EP node  647:Else
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  788:ModifyHeader
                    // BDD node 33:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
                    hdr.hdr3.data4[15:8] = meta.dev[7:0];
                    hdr.hdr3.data4[7:0] = meta.dev[15:8];
                    // EP node  1335:Forward
                    // BDD node 37:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                  }
                } else {
                  // EP node  576:Else
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  1416:DchainTableLookup
                  // BDD node 38:dchain_rejuvenate_index(chain:(w64 1073971976), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  dchain_table_1073971976_38_key0 = guarded_map_table_1073923800_13_get_value_param0;
                  dchain_table_1073971976_38.apply();
                  // EP node  1538:If
                  // BDD node 40:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr3.data0)){
                    // EP node  1539:Then
                    // BDD node 40:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  6070:Ignore
                    // BDD node 39:vector_borrow(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074041936)[ -> (w64 1073968736)])
                    // EP node  6264:VectorRegisterUpdate
                    // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
                    vector_register_1073954840_0_write_6264.execute(guarded_map_table_1073923800_13_get_value_param0);
                    // EP node  6462:ModifyHeader
                    // BDD node 42:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
                    hdr.hdr3.data3 = 8w0x01;
                    // EP node  6619:ModifyHeader
                    // BDD node 43:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                    // EP node  6779:ModifyHeader
                    // BDD node 44:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                    swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                    swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                    swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                    // EP node  6942:ModifyHeader
                    // BDD node 45:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                    swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                    // EP node  7108:Forward
                    // BDD node 46:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  } else {
                    // EP node  1540:Else
                    // BDD node 40:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  1671:VectorRegisterLookup
                    // BDD node 124:vector_borrow(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074041936)[ -> (w64 1073968736)])
                    bit<32> vector_reg_value0 = vector_register_1073954840_0_read_1671.execute(guarded_map_table_1073923800_13_get_value_param0);
                    // EP node  1782:Ignore
                    // BDD node 47:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 0) vector_data_384)])
                    // EP node  1928:ModifyHeader
                    // BDD node 48:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (Concat w72 (Read w8 (w32 3) vector_data_384) (Concat w64 (Read w8 (w32 2) vector_data_384) (Concat w56 (Read w8 (w32 1) vector_data_384) (Concat w48 (Read w8 (w32 0) vector_data_384) (ReadLSB w40 (w32 768) packet_chunks))))))))])
                    hdr.hdr3.data2 = vector_reg_value0;
                    hdr.hdr3.data3 = 8w0x01;
                    // EP node  2046:ModifyHeader
                    // BDD node 49:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                    // EP node  2167:ModifyHeader
                    // BDD node 50:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                    swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                    swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                    swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                    // EP node  2291:ModifyHeader
                    // BDD node 51:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                    swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                    // EP node  2418:Forward
                    // BDD node 52:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  }
                }
              } else {
                // EP node  252:Else
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  10234:Forward
                // BDD node 57:FORWARD
                nf_dev[15:0] = bswap16(hdr.hdr3.data4);
              }
            }
            // EP node  119:Else
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  9837:ParserReject
            // BDD node 61:DROP
          }
          // EP node  54:Else
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  9212:ParserReject
          // BDD node 64:DROP
        }
        // EP node  15:Else
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  8366:ParserReject
        // BDD node 66:DROP
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
