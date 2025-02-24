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
  bit<16> ingress_dev;
  bit<16> egress_dev;

}

header recirc_h {
  bit<16> code_path;

};



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;

}

struct synapse_ingress_metadata_t {
  bit<16> dev;
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
    hdr.cpu.ingress_dev = meta.dev;
    fwd(CPU_PCIE_PORT);
  }

  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

  action set_ingress_dev(bit<16> nf_dev) { meta.dev = nf_dev; }
  table ingress_port_to_nf_dev {
    key = { ig_intr_md.ingress_port: exact; }
    actions = { set_ingress_dev; }
    size = 32;
  }

  bool trigger_forward = false;
  bit<16> nf_dev = 16w0;
  table forward_nf_dev {
    key = { nf_dev: exact; }
    actions = { fwd; }
    size = 64;
  }

  bit<16> vector_table_1074013064_65_get_value_param_0 = 16w0;
  action vector_table_1074013064_65_get_value(bit<16> _vector_table_1074013064_65_get_value_param_0) {
    vector_table_1074013064_65_get_value_param_0 = _vector_table_1074013064_65_get_value_param_0;
  }

  bit<32> vector_table_1074013064_65_key_0 = 32w0;
  table vector_table_1074013064_65 {
    key = {
      vector_table_1074013064_65_key_0: exact;
    }
    actions = {
      vector_table_1074013064_65_get_value;
    }
    size = 32;
  }


  apply {
    if (hdr.cpu.isValid()) {
      nf_dev = hdr.cpu.egress_dev;
      hdr.cpu.setInvalid();
    } else if (hdr.recirc.isValid()) {
      
    } else {
      ingress_port_to_nf_dev.apply();
      // EP node  1
      // BDD node 65:vector_borrow(vector:(w64 1074013064), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074082464)[ -> (w64 1074026960)])
      vector_table_1074013064_65_key_0 = (bit<32>)(meta.dev);
      vector_table_1074013064_65.apply();
      // EP node  13
      // BDD node 66:vector_return(vector:(w64 1074013064), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074026960)[(ReadLSB w16 (w32 0) vector_data_128)])
      // EP node  30
      // BDD node 67:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_128)))))
      if (16w0xffff != vector_table_1074013064_65_get_value_param_0) {
        // EP node  31
        // BDD node 67:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_128)))))
        // EP node  70
        // BDD node 68:FORWARD
        nf_dev = vector_table_1074013064_65_get_value_param_0;
        trigger_forward = true;
      } else {
        // EP node  32
        // BDD node 67:if ((Eq false (Eq (w16 65535) (Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) vector_data_128)))))
        // EP node  112
        // BDD node 69:DROP
        drop();
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
