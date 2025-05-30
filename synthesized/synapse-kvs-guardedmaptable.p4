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
  @padding bit<31> pad_hit0;
  bool hit0;
  bit<32> dev;
  bit<64> time;

}

header recirc_h {
  bit<16> code_path;

};

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
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;

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
    transition parser_6;
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr0.data2) {
      16w0x0800: parser_7;
      default: parser_73;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr1);
    transition parser_8;
  }
  state parser_73 {
    transition reject;
  }
  state parser_8 {
    transition parser_8_0;
  }
  state parser_8_0 {
    transition select (hdr.hdr1.data1[23:16]) {
      8w0x11: parser_9;
      default: parser_71;
    }
  }
  state parser_9 {
    pkt.extract(hdr.hdr2);
    transition parser_10;
  }
  state parser_71 {
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
      default: parser_68;
    }
  }
  state parser_11 {
    pkt.extract(hdr.hdr3);
    transition parser_64;
  }
  state parser_68 {
    transition reject;
  }
  state parser_64 {
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

  bit<32> map_table_1073923160_13_get_value_param0 = 32w0;
  action map_table_1073923160_13_get_value(bit<32> _map_table_1073923160_13_get_value_param0) {
    map_table_1073923160_13_get_value_param0 = _map_table_1073923160_13_get_value_param0;
  }

  bit<32> map_table_1073923160_13_key0 = 32w0;
  table map_table_1073923160_13 {
    key = {
      map_table_1073923160_13_key0: exact;
    }
    actions = {
      map_table_1073923160_13_get_value;
    }
    size = 9103;
    idle_timeout = true;
  }

  bit<32> dchain_table_1073971336_38_key0 = 32w0;
  table dchain_table_1073971336_38 {
    key = {
      dchain_table_1073971336_38_key0: exact;
    }
    actions = {
       NoAction;
    }
    size = 9103;
    idle_timeout = true;
  }

  Register<bit<32>,_>(8192, 0) vector_register_1073954200_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073954200_0) vector_register_1073954200_0_read_2135 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073954200_0) vector_register_1073954200_0_read_4413 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073954200_0) vector_register_1073954200_0_write_4789 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2;
    }
  };


  apply {
    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {

    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:Ignore
      // BDD node 4:expire_items_single_map(chain:(w64 1073971336), vector:(w64 1073936984), time:(Add w64 (w64 18446744073609551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073923160))
      // EP node  4:ParserExtraction
      // BDD node 5:packet_borrow_next_chunk(p:(w64 1074031024), length:(w32 14), chunk:(w64 1074041728)[ -> (w64 1073759520)])
      if(hdr.hdr0.isValid()) {
        // EP node  12:ParserCondition
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  13:Then
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  22:ParserExtraction
        // BDD node 7:packet_borrow_next_chunk(p:(w64 1074031024), length:(w32 20), chunk:(w64 1074042464)[ -> (w64 1073759776)])
        if(hdr.hdr1.isValid()) {
          // EP node  43:ParserCondition
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  44:Then
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  58:ParserExtraction
          // BDD node 9:packet_borrow_next_chunk(p:(w64 1074031024), length:(w32 8), chunk:(w64 1074043120)[ -> (w64 1073760032)])
          if(hdr.hdr2.isValid()) {
            // EP node  92:ParserCondition
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  93:Then
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  125:If
            // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
            if ((16w0x0000) != (meta.dev[15:0])) {
              // EP node  126:Then
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              // EP node  165:ParserExtraction
              // BDD node 11:packet_borrow_next_chunk(p:(w64 1074031024), length:(w32 12), chunk:(w64 1074043864)[ -> (w64 1073760288)])
              if(hdr.hdr3.isValid()) {
                // EP node  208:MapTableLookup
                // BDD node 13:map_get(map:(w64 1073923160), key:(w64 1073760289)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074041040)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                map_table_1073923160_13_key0 = hdr.hdr3.data1;
                bool hit0 = map_table_1073923160_13.apply().hit;
                // EP node  366:If
                // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                if (!hit0) {
                  // EP node  367:Then
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  510:If
                  // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr3.data0)) {
                    // EP node  511:Then
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  3641:SendToController
                    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971336), index_out:(w64 1074044784)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    send_to_controller(3641);
                    hdr.cpu.hit0 = hit0;
                    hdr.cpu.dev = meta.dev;
                    hdr.cpu.time[47:16] = meta.time;
                  } else {
                    // EP node  512:Else
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  700:ModifyHeader
                    // BDD node 33:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760288)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
                    hdr.hdr3.data4[15:8] = meta.dev[7:0];
                    hdr.hdr3.data4[7:0] = meta.dev[15:8];
                    // EP node  1346:Forward
                    // BDD node 37:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                    trigger_forward = true;
                  }
                } else {
                  // EP node  368:Else
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  1495:DchainTableLookup
                  // BDD node 38:dchain_rejuvenate_index(chain:(w64 1073971336), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  dchain_table_1073971336_38_key0 = map_table_1073923160_13_get_value_param0;
                  dchain_table_1073971336_38.apply();
                  // EP node  1675:If
                  // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                  if ((8w0x01) != (hdr.hdr3.data0)) {
                    // EP node  1676:Then
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  1877:If
                    // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                    if ((8w0x00) != (hdr.hdr3.data0)) {
                      // EP node  1878:Then
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  7702:ModifyHeader
                      // BDD node 41:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760288)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
                      hdr.hdr3.data3 = 8w0x01;
                      // EP node  8629:ModifyHeader
                      // BDD node 42:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760032)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  9439:ModifyHeader
                      // BDD node 43:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759776)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  10123:ModifyHeader
                      // BDD node 44:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759520)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  10648:Forward
                      // BDD node 45:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    } else {
                      // EP node  1879:Else
                      // BDD node 40:if ((Eq false (Eq (w8 0) (Read w8 (w32 768) packet_chunks)))
                      // EP node  2135:VectorRegisterLookup
                      // BDD node 46:vector_borrow(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074044304)[ -> (w64 1073968096)])
                      // EP node  2365:Ignore
                      // BDD node 47:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 0) vector_data_384)])
                      // EP node  2636:ModifyHeader
                      // BDD node 48:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760288)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (Concat w72 (Read w8 (w32 3) vector_data_384) (Concat w64 (Read w8 (w32 2) vector_data_384) (Concat w56 (Read w8 (w32 1) vector_data_384) (Concat w48 (Read w8 (w32 0) vector_data_384) (ReadLSB w40 (w32 768) packet_chunks))))))))])
                      hdr.hdr3.data2 = vector_register_1073954200_0_read_2135.execute(map_table_1073923160_13_get_value_param0);
                      hdr.hdr3.data3 = 8w0x01;
                      // EP node  2879:ModifyHeader
                      // BDD node 49:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760032)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                      swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                      swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                      // EP node  3128:ModifyHeader
                      // BDD node 50:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759776)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                      swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                      swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                      swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                      swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                      // EP node  3383:ModifyHeader
                      // BDD node 51:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759520)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                      swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                      swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                      swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                      swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                      swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                      swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                      // EP node  3583:Forward
                      // BDD node 52:FORWARD
                      nf_dev[15:0] = meta.dev[15:0];
                      trigger_forward = true;
                    }
                  } else {
                    // EP node  1677:Else
                    // BDD node 39:if ((Eq false (Eq (w8 1) (Read w8 (w32 768) packet_chunks)))
                    // EP node  4413:VectorRegisterLookup
                    // BDD node 53:vector_borrow(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074044336)[ -> (w64 1073968096)])
                    // EP node  4789:VectorRegisterUpdate
                    // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
                    vector_register_1073954200_0_write_4789.execute(map_table_1073923160_13_get_value_param0);
                    // EP node  5231:ModifyHeader
                    // BDD node 55:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760288)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
                    hdr.hdr3.data3 = 8w0x01;
                    // EP node  5620:ModifyHeader
                    // BDD node 56:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760032)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                    // EP node  6015:ModifyHeader
                    // BDD node 57:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759776)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                    swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                    swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                    swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                    // EP node  6416:ModifyHeader
                    // BDD node 58:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759520)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                    swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                    // EP node  6741:Forward
                    // BDD node 59:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                    trigger_forward = true;
                  }
                }
              }
            } else {
              // EP node  127:Else
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              // EP node  7214:ParserExtraction
              // BDD node 138:packet_borrow_next_chunk(p:(w64 1074031024), length:(w32 12), chunk:(w64 1074043864)[ -> (w64 1073760288)])
              if(hdr.hdr3.isValid()) {
                // EP node  10723:Forward
                // BDD node 64:FORWARD
                nf_dev[15:0] = bswap16(hdr.hdr3.data4);
                trigger_forward = true;
              }
            }
            // EP node  94:Else
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  9511:ParserReject
            // BDD node 68:DROP
          }
          // EP node  45:Else
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  8699:ParserReject
          // BDD node 71:DROP
        }
        // EP node  14:Else
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  7770:ParserReject
        // BDD node 73:DROP
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
