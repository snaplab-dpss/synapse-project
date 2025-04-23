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

}

header recirc_h {
  bit<16> code_path;

};



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;

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

  bit<16> vector_table_1074013320_65_get_value_param0 = 16w0;
  action vector_table_1074013320_65_get_value(bit<16> _vector_table_1074013320_65_get_value_param0) {
    vector_table_1074013320_65_get_value_param0 = _vector_table_1074013320_65_get_value_param0;
  }

  bit<32> vector_table_1074013320_65_key0 = 32w0;
  table vector_table_1074013320_65 {
    key = {
      vector_table_1074013320_65_key0: exact;
    }
    actions = {
      vector_table_1074013320_65_get_value;
    }
    size = 36;
  }


  apply {
    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
      trigger_forward = true;
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1:VectorTableLookup
      // BDD node 65:vector_borrow(vector:(w64 1074013320), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074082720)[ -> (w64 1074027216)])
      vector_table_1074013320_65_key0 = meta.dev;
      vector_table_1074013320_65.apply();
      // EP node  25:Ignore
      // BDD node 66:vector_return(vector:(w64 1074013320), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074027216)[(ReadLSB w16 (w32 0) vector_data_128)])
      // EP node  42:If
      // BDD node 67:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_128)))
      if ((16w0xffff) != (vector_table_1074013320_65_get_value_param0)) {
        // EP node  43:Then
        // BDD node 67:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_128)))
        // EP node  76:Forward
        // BDD node 68:FORWARD
        nf_dev[15:0] = vector_table_1074013320_65_get_value_param0;
        trigger_forward = true;
      } else {
        // EP node  44:Else
        // BDD node 67:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_128)))
        // EP node  111:Drop
        // BDD node 69:DROP
        drop();
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
