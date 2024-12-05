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

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

typedef bit<8> match_counter_t;

header cpu_h {
  bit<16> code_path;
  
  @padding port_pad_t pad_in_port;
  port_t in_port;

  @padding port_pad_t pad_out_port;
  port_t out_port;

  @padding bit<7> pad_hit_table_1073966360_20;
  bool hit_table_1073966360_20;
  bit<32> table_1073966360_20_value_0;

}

header hdr_0_h {
  bit<112> data;
}
header hdr_1_h {
  bit<160> data;
}
header hdr_2_h {
  bit<32> data;
}
header hdr_3_h {
  bit<32> data;
}


struct my_ingress_headers_t {
  cpu_h cpu;
  hdr_0_h hdr_0;
  hdr_1_h hdr_1;
  hdr_2_h hdr_2;
  hdr_3_h hdr_3;

}

struct my_ingress_metadata_t {

}

struct my_egress_headers_t {
  cpu_h cpu;

}

struct my_egress_metadata_t {

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

  /* User */    
  out my_ingress_headers_t hdr,
  out my_ingress_metadata_t meta,

  /* Intrinsic */
  out ingress_intrinsic_metadata_t ig_intr_md
) {
  TofinoIngressParser() tofino_parser;
  
  /* This is a mandatory state, required by Tofino Architecture */
  state start {
    tofino_parser.apply(pkt, ig_intr_md);

    transition select(ig_intr_md.ingress_port) {
      CPU_PCIE_PORT: parse_cpu;
      default: parser_init;
    }
  }

  state parse_cpu {
    pkt.extract(hdr.cpu);
    transition accept;
  }

  state parser_init {
    pkt.extract(hdr.hdr_0);
    transition parser_2;
  }
  state parser_2 {
    transition select (hdr.hdr_0.data[111:96]) {
      2048: parser_3;
      default: parser_46;
    }
  }
  state parser_3 {
    pkt.extract(hdr.hdr_1);
    transition parser_4;
  }
  state parser_46 {
    transition reject;
  }
  state parser_4 {
    transition select (hdr.hdr_1.data[79:72]) {
      6: parser_6;
      17: parser_6;
      default: parser_44;
    }
  }
  state parser_6 {
    transition select (ig_intr_md.ingress_port) {
      0: parser_5;
      default: parser_88;
    }
  }
  state parser_44 {
    transition reject;
  }
  state parser_5 {
    pkt.extract(hdr.hdr_2);
    transition parser_19;
  }
  state parser_88 {
    pkt.extract(hdr.hdr_2);
    transition parser_41;
  }
  state parser_19 {
    transition accept;
  }
  state parser_41 {
    transition accept;
  }

}

control Ingress(
  /* User */
  inout my_ingress_headers_t hdr,
  inout my_ingress_metadata_t meta,

  /* Intrinsic */
  in    ingress_intrinsic_metadata_t ig_intr_md,
  in    ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
  inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
  inout ingress_intrinsic_metadata_for_tm_t ig_tm_md
) {
  action drop() {
    ig_dprsr_md.drop_ctl = 1;
  }

  action fwd(port_t port) {
    ig_tm_md.ucast_egress_port = port;
  }

  action send_to_controller(bit<16> code_path) {
    hdr.cpu.setValid();
    hdr.cpu.code_path = code_path;
    hdr.cpu.in_port = ig_intr_md.ingress_port;
    fwd(CPU_PCIE_PORT);
  }

  bit<32> table_1073966360_7_value_0;
  action table_1073966360_7_get_value(bit<32> _table_1073966360_7_value_0) {
    table_1073966360_7_value_0 = _table_1073966360_7_value_0;
  }
  
  table table_1073966360_7 {
    key = {
      hdr.hdr_1.data[79:72]: exact;
      hdr.hdr_1.data[127:96]: exact;
      hdr.hdr_1.data[159:128]: exact;
      hdr.hdr_2.data[15:0]: exact;
      hdr.hdr_2.data[31:16]: exact;
    }
    actions = {
      table_1073966360_7_get_value;
    }
    size = 65536;
  }

  bit<32> table_1073966360_20_value_0;
  action table_1073966360_20_get_value(bit<32> _table_1073966360_20_value_0) {
    table_1073966360_20_value_0 = _table_1073966360_20_value_0;
  }
  
  table table_1073966360_20 {
    key = {
      hdr.hdr_1.data[79:72]: exact;
      hdr.hdr_1.data[159:96]: exact;
      hdr.hdr_3.data: exact;
    }
    actions = {
      table_1073966360_20_get_value;
    }
    size = 65536;
  }

  table table_1074005440_37 {
    key = {
      table_1073966360_20_value_0: exact;
    }
    actions = {}
    size = 65536;
  }



  apply {
    if(hdr.hdr_0.isValid()) {
      if(hdr.hdr_1.isValid()) {
        if(hdr.hdr_2.isValid()) {
          bool hit_table_1073966360_7 = table_1073966360_7.apply().hit;
          if (!hit_table_1073966360_7) {
            drop();
          } else {
            hdr.hdr_0.data[95:0] = 96w000000000000008967452301;
            fwd(0);
          }
        }
        if(hdr.hdr_3.isValid()) {
          bool hit_table_1073966360_20 = table_1073966360_20.apply().hit;
          if (!hit_table_1073966360_20) {
            hdr.cpu.hit_table_1073966360_20 = hit_table_1073966360_20;
            hdr.cpu.table_1073966360_20_value_0 = table_1073966360_20_value_0;
            send_to_controller(2528);
          } else {
            table_1074005440_37.apply();
            hdr.hdr_0.data[95:0] = 96w000000000000018967452301;
            fwd(1);
          }
        }
      }
    }

  }
}

control IngressDeparser(
  packet_out pkt,

  /* User */
  inout my_ingress_headers_t hdr,
  in    my_ingress_metadata_t meta,

  /* Intrinsic */
  in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md
) {
  apply {

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

  /* User */
  out my_egress_headers_t hdr,
  out my_egress_metadata_t eg_md,

  /* Intrinsic */
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
  /* User */
  inout my_egress_headers_t hdr,
  inout my_egress_metadata_t eg_md,

  /* Intrinsic */
  in    egress_intrinsic_metadata_t eg_intr_md,
  in    egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
  inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
  inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
  apply {}
}

control EgressDeparser(
  packet_out pkt,

  /* User */
  inout my_egress_headers_t hdr,
  in    my_egress_metadata_t eg_md,

  /* Intrinsic */
  in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md
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
