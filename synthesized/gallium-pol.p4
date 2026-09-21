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
  DONE    = 0x04
}

enum bit<2> fwd_op_t {
  FORWARD_NF_DEV  = 0,
  FORWARD_TO_CPU  = 1,
  RECIRCULATE     = 2,
  DROP            = 3
}

header cpu_h {
  bit<16> code_path;                  // Written by the data plane
  bit<16> egress_dev;                 // Written by the control plane
  bit<8> trigger_dataplane_execution; // Written by the control plane
  // Where the packet came in, for a packet the controller hands back to be executed again: it
  // returns on the CPU port, so ingress_port_to_nf_dev no longer knows either. What the
  // recirculation header carries for the same reason. Written by the data plane.
  bit<16> ingress_dev;
  bit<16> ingress_port;
  bit<32> time; // The ingress clock at the hand-off, the controller's now for the packet.
  bit<32> vector_reg_value0;
  bit<32> dev;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;


};

header egress_state_h {
  bit<16> code_path;
  bit<32> time; // The ingress clock, ingress_mac_tstamp[47:16]: the packet's time in the egress too.
  bit<32> e32_0;
  bit<16> e16_0;
}


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
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<16> data3;
}
header hdr1_h {
  bit<16> data0;
  bit<16> data1;
  bit<32> data2;
  bit<32> data3;
  bit<32> data4;
  bit<32> data5;
}
header hdr2_h {
  bit<32> data0;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  egress_state_h egress_state;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  // What the forwarding table decided: 0 the packet stays on the switch (recirculated or
  // dropped), 1 it leaves, 2 it goes to the controller. The headers carrying data-plane state are
  // dropped on the strength of this, after the table and outside its actions, because a packet
  // that still has the egress ahead of it must keep them: the egress parser extracts them.
  bit<2> leaving;
  bit<32> vector_reg_value0;
  bit<16> vector_reg_value1;
  bit<1> to_egress;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  egress_state_h egress_state;

  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

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

    meta.ingress_port = (bit<16>)ig_intr_md.ingress_port;
    meta.dev = 0;
    meta.leaving = 0;
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

    // A packet the controller declined re-enters the pipeline from the top, so its own headers
    // have to be parsed again -- every branch down there asks whether they are valid. It parsed
    // on the way in, or it would have been rejected before ever reaching the controller. One the
    // controller has already decided on is only forwarded, and its bytes stay payload.
    transition select(hdr.cpu.trigger_dataplane_execution) {
      8w1: parser_init;
      default: accept;
    }
  }

  state parse_recirc {
    pkt.extract(hdr.recirc);
    pkt.extract(hdr.egress_state);

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
    transition parser_133;
  }
  state parser_133 {
    transition parser_133_0;
  }
  state parser_133_0 {
    transition select (hdr.hdr0.data3) {
      16w0x0800: parser_134;
      default: parser_187;
    }
  }
  state parser_134 {
    pkt.extract(hdr.hdr1);
    transition parser_135;
  }
  state parser_187 {
    transition reject;
  }
  state parser_135 {
    transition parser_135_0;
  }
  state parser_135_0 {
    transition select (hdr.hdr1.data3[23:16]) {
      8w0x06: parser_136;
      8w0x11: parser_136;
      default: parser_185;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr2);
    transition parser_181;
  }
  state parser_185 {
    transition reject;
  }
  state parser_181 {
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
    hdr.cuckoo.setInvalid();
    meta.leaving = 2;
    fwd(CPU_PCIE_PORT);
  }

  action fwd_nf_dev(bit<16> port) {
    hdr.cpu.setInvalid();
    hdr.cuckoo.setInvalid();
    meta.leaving = 1;
    fwd(port);
  }

  action set_ingress_dev(bit<32> nf_dev) {
    meta.dev = nf_dev;
  }

  action set_ingress_dev_from_recirculation() {
    meta.ingress_port = hdr.recirc.ingress_port;
    meta.dev = hdr.recirc.dev;
  }

  action set_ingress_dev_from_cpu() {
    meta.ingress_port = hdr.cpu.ingress_port;
    meta.dev = (bit<32>)hdr.cpu.ingress_dev;
  }

  table ingress_port_to_nf_dev {
    key = {
      meta.ingress_port: exact;
    }
    actions = {
      set_ingress_dev;
      set_ingress_dev_from_recirculation;
      set_ingress_dev_from_cpu;
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

  // Swapping two fields a byte at a time forces byte-granular PHV slicing on both of them, and a
  // field sliced that way drags its neighbours into the same container group; bf-p4c then cannot
  // satisfy the action constraints. Swap whole fields where the byte pairs make one up.
  action swap16(inout bit<16> a, inout bit<16> b) {
    bit<16> tmp = a;
    a = b;
    b = tmp;
  }

  action swap24(inout bit<24> a, inout bit<24> b) {
    bit<24> tmp = a;
    a = b;
    b = tmp;
  }

  action swap32(inout bit<32> a, inout bit<32> b) {
    bit<32> tmp = a;
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
    hdr.cpu.ingress_dev = meta.dev[15:0];
    hdr.cpu.ingress_port = meta.ingress_port;
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

  Register<bit<32>,_>(32, 0) vector_register_1074054008_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1074054008_0) vector_register_1074054008_0_read_141 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action regexec_vector_register_1074054008_0_read_141() {
    meta.vector_reg_value0 = vector_register_1074054008_0_read_141.execute(meta.dev);
  }
  Register<bit<16>,_>(32, 0) vector_register_1074071224_0;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1074071224_0) vector_register_1074071224_0_read_388 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  action regexec_vector_register_1074071224_0_read_388() {
    meta.vector_reg_value1 = vector_register_1074071224_0_read_388.execute(meta.dev);
  }

  apply {
    meta.to_egress = 0;

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:Ignore
      // BDD node 131:tb_expire
      // EP node  4:ParserExtraction
      // BDD node 132:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  13:ParserCondition
        // BDD node 133:if
        // EP node  14:Then
        // BDD node 133:if
        // EP node  26:ParserExtraction
        // BDD node 134:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  58:ParserCondition
          // BDD node 135:if
          // EP node  59:Then
          // BDD node 135:if
          // EP node  98:ParserExtraction
          // BDD node 136:packet_borrow_next_chunk
          if(hdr.hdr2.isValid()) {
            // EP node  141:VectorRegisterLookup
            // BDD node 137:vector_borrow
            regexec_vector_register_1074054008_0_read_141();
            // EP node  211:Ignore
            // BDD node 138:vector_return
            // EP node  275:If
            // BDD node 139:if
            if ((32w0x00000000) == (meta.vector_reg_value0)){
              // EP node  276:Then
              // BDD node 139:if
              // EP node  363:SendToController
              // BDD node 140:tb_is_tracing
              fwd_op = fwd_op_t.FORWARD_TO_CPU;
              build_cpu_hdr(0);
              hdr.cpu.time = meta.time;
              hdr.cpu.vector_reg_value0 = meta.vector_reg_value0;
              hdr.cpu.dev = meta.dev;
            } else {
              // EP node  277:Else
              // BDD node 139:if
              // EP node  388:VectorRegisterLookup
              // BDD node 175:vector_borrow
              regexec_vector_register_1074071224_0_read_388();
              // EP node  517:Ignore
              // BDD node 176:vector_return
              // EP node  625:SendToEgress
              // BDD node 177:packet_return_chunk
              meta.to_egress = 1;
              hdr.recirc.setValid();
              hdr.egress_state.setValid();
              hdr.egress_state.code_path = 0;
              hdr.egress_state.time = meta.time;
              hdr.egress_state.e32_0 = meta.dev;
              hdr.egress_state.e16_0 = meta.vector_reg_value1;
              nf_dev[15:0] = hdr.egress_state.e16_0;
            }
          }
          // EP node  60:Else
          // BDD node 135:if
          // EP node  1437:ParserReject
          // BDD node 185:DROP
        }
        // EP node  15:Else
        // BDD node 133:if
        // EP node  1271:ParserReject
        // BDD node 187:DROP
      }

    }

    forwarding_tbl.apply();
    if (meta.leaving != 0 && meta.to_egress == 0) {
      hdr.recirc.setInvalid();
      hdr.egress_state.setInvalid();
    }
    if (meta.leaving == 0) {
      hdr.cpu.setInvalid();
    }

    if (meta.to_egress == 1) {
      ig_tm_md.bypass_egress = 0;
    } else {
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
    pkt.extract(hdr.recirc);
    pkt.extract(hdr.egress_state);
    pkt.extract(hdr.hdr0);
    pkt.extract(hdr.hdr1);
    pkt.extract(hdr.hdr2);
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
  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

  action swap16(inout bit<16> a, inout bit<16> b) {
    bit<16> tmp = a;
    a = b;
    b = tmp;
  }

  action swap24(inout bit<24> a, inout bit<24> b) {
    bit<24> tmp = a;
    a = b;
    b = tmp;
  }

  action swap32(inout bit<32> a, inout bit<32> b) {
    bit<32> tmp = a;
    a = b;
    b = tmp;
  }

  bit<1> diff_sign_bit;
  action calculate_diff_32b(bit<32> a, bit<32> b) { diff_sign_bit = (a - b)[31:31]; }
  action calculate_diff_16b(bit<16> a, bit<16> b) { diff_sign_bit = (a - b)[15:15]; }
  action calculate_diff_8b(bit<8> a, bit<8> b) { diff_sign_bit = (a - b)[7:7]; }


  apply {
    if (hdr.egress_state.code_path == 0) {
      // EP node  896:If
      // BDD node 180:if
      if ((hdr.egress_state.e32_0[15:0]) != (hdr.egress_state.e16_0)){
        // EP node  897:Then
        // BDD node 180:if
        // EP node  958:Forward
        // BDD node 181:FORWARD
        hdr.recirc.setInvalid();
        hdr.egress_state.setInvalid();
      } else {
        // EP node  898:Else
        // BDD node 180:if
        // EP node  1207:Drop
        // BDD node 182:DROP
        ig_intr_dprs_md.drop_ctl = 1;
      }
    }

  }
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
