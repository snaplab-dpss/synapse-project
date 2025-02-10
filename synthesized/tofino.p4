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
  @padding bit<7> pad_in_port;
  bit<9> in_port;
  @padding bit<7> pad_out_port;
  bit<9> out_port;

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


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  hdr_0_h hdr_0;
  hdr_1_h hdr_1;

}

struct synapse_ingress_metadata_t {
  bit<16> port;
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

    meta.port = 7w0 ++ ig_intr_md.ingress_port;
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
    transition parser_1;
  }
  state parser_1 {
    transition select (hdr.hdr_0.data[111:96]) {
      2048: parser_2;
      default: parser_10;
    }
  }
  state parser_2 {
    pkt.extract(hdr.hdr_1);
    transition parser_8;
  }
  state parser_10 {
    transition reject;
  }
  state parser_8 {
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

  action fwd(bit<9> port) {
    ig_tm_md.ucast_egress_port = port;
  }

  action send_to_controller(bit<16> code_path) {
    hdr.cpu.setValid();
    hdr.cpu.code_path = code_path;
    hdr.cpu.in_port = ig_intr_md.ingress_port;
    fwd(CPU_PCIE_PORT);
  }

  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

  bit<16> lpm_1073911200_get_device_param_0 = 16w0;
  action lpm_1073911200_get_device(bit<16> _lpm_1073911200_get_device_param_0) {
    lpm_1073911200_get_device_param_0 = _lpm_1073911200_get_device_param_0;
  }

  bit<32> ipv4_addr_0 = 32w0;
  table lpm_1073911200 {
    key = {
      ipv4_addr_0: ternary;
    }
    actions = { lpm_1073911200_get_device; }
    size = 32;
  }


  apply {
    if (hdr.cpu.isValid()) {
      bit<9> out_port = hdr.cpu.out_port;
      hdr.cpu.setInvalid();
      fwd(out_port);
    } else if (hdr.recirc.isValid()) {
      
    } else {
      // EP node  0
      // BDD node 0:packet_borrow_next_chunk(p:(w64 1073939392), length:(w32 14), chunk:(w64 1073949664)[ -> (w64 1073755936)])
      if(hdr.hdr_0.isValid()) {
        // EP node  2
        // BDD node 1:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  3
        // BDD node 1:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  5
        // BDD node 2:packet_borrow_next_chunk(p:(w64 1073939392), length:(w32 20), chunk:(w64 1073950400)[ -> (w64 1073756083)])
        if(hdr.hdr_1.isValid()) {
          // EP node  10
          // BDD node 3:lpm_lookup(lpm:(w64 1073911200), prefix:(ReadLSB w32 (w32 159) packet_chunks), value_out:(w64 1073949340)[(w16 43947) -> (ReadLSB w16 (w32 0) lpm_lookup_result)])
          ipv4_addr_0 = hdr.hdr_1.data[127:96];
          bool hit_0 = lpm_1073911200.apply().hit;
          // EP node  180
          // BDD node 6:if ((Eq (w32 0) (ReadLSB w32 (w32 0) lpm_lookup_match))
          if (!hit_0) {
            // EP node  181
            // BDD node 6:if ((Eq (w32 0) (ReadLSB w32 (w32 0) lpm_lookup_match))
            // EP node  303
            // BDD node 7:DROP
            drop();
          } else {
            // EP node  182
            // BDD node 6:if ((Eq (w32 0) (ReadLSB w32 (w32 0) lpm_lookup_match))
            // EP node  238
            // BDD node 8:FORWARD
            fwd(lpm_1073911200_get_device_param_0);
          }
        }
        // EP node  4
        // BDD node 1:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  315
        // BDD node 10:DROP
      }
      ig_tm_md.bypass_egress = 1;
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
