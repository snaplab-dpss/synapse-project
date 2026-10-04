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
  bit<32> data0;
  bit<32> data1;
  bit<16> data2;
  bit<16> data3;
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
  bit<32> key_32b_0;
  bit<32> vector_reg_value0;
  bit<32> regexec_vector_register_1249835474944_0_read_1403_index0;
  bit<16> vector_reg_value1;
  bit<16> pkt_len;
  bit<1> pc_145;
  bit<1> pc_147;
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
    meta.pc_145 = 0;
    meta.pc_147 = 0;
    pkt.extract(hdr.hdr0);
    transition parser_145;
  }
  state parser_145 {
    transition parser_145_0;
  }
  state parser_145_0 {
    transition select (hdr.hdr0.data3) {
      16w0x0800: parser_146;
      default: parser_196;
    }
  }
  state parser_146 {
    meta.pc_145 = 1;
    pkt.extract(hdr.hdr1);
    transition parser_147;
  }
  state parser_196 {
    transition reject;
  }
  state parser_147 {
    transition parser_147_0;
  }
  state parser_147_0 {
    transition select (hdr.hdr1.data2[7:0]) {
      8w0x06: parser_148;
      8w0x11: parser_148;
      default: parser_194;
    }
  }
  state parser_148 {
    meta.pc_147 = 1;
    pkt.extract(hdr.hdr2);
    transition parser_190;
  }
  state parser_194 {
    transition reject;
  }
  state parser_190 {
    transition accept;
  }

}


@pa_no_overlay("ingress", "meta.regexec_vector_register_1249835474944_0_read_1403_index0")
@pa_solitary("ingress", "meta.pc_145")
@pa_solitary("ingress", "meta.pc_147")


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

  bit<32> vector_table_1246077374464_157_get_value_param0 = 32w0;
  action vector_table_1246077374464_157_get_value(bit<32> _vector_table_1246077374464_157_get_value_param0) {
    vector_table_1246077374464_157_get_value_param0 = _vector_table_1246077374464_157_get_value_param0;
  }

  table vector_table_1246077374464_157 {
    key = {
      meta.key_32b_0: exact;
    }
    actions = {
      vector_table_1246077374464_157_get_value;
    }
    size = 36;
  }

  Register<bit<32>,_>(256, 0) vector_register_1249835474944_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1249835474944_0) vector_register_1249835474944_0_read_1403 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };


  action regexec_vector_register_1249835474944_0_read_1403() {
    meta.vector_reg_value0 = vector_register_1249835474944_0_read_1403.execute(meta.regexec_vector_register_1249835474944_0_read_1403_index0);
  }
  Register<bit<16>,_>(32, 0) vector_register_1252519837696_0;

  RegisterAction<bit<16>, bit<32>, bit<16>>(vector_register_1252519837696_0) vector_register_1252519837696_0_read_1481 = {
    void apply(inout bit<16> value, out bit<16> out_value) {
      out_value = value;
    }
  };


  action regexec_vector_register_1252519837696_0_read_1481() {
    meta.vector_reg_value1 = vector_register_1252519837696_0_read_1481.execute(meta.dev);
  }

  apply {
    meta.pkt_len = 0;
    if (hdr.hdr1.isValid()) {
      meta.pkt_len = hdr.hdr1.data0[15:0] + 14;
    }
    meta.to_egress = 0;

    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:ParserExtraction
      // BDD node 144:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  5:ParserCondition
        // BDD node 145:if
        if (meta.pc_145 == 1) {
          // EP node  6:Then
          // BDD node 145:if
          // EP node  16:ParserExtraction
          // BDD node 146:packet_borrow_next_chunk
          if(hdr.hdr1.isValid()) {
            // EP node  43:ParserCondition
            // BDD node 147:if
            if (meta.pc_147 == 1) {
              // EP node  44:Then
              // BDD node 147:if
              // EP node  143:ParserExtraction
              // BDD node 148:packet_borrow_next_chunk
              if(hdr.hdr2.isValid()) {
                // EP node  254:Ignore
                // BDD node 149:vector_periodic_clear
                // EP node  368:Ignore
                // BDD node 150:vector_periodic_clear
                // EP node  482:Ignore
                // BDD node 151:vector_periodic_clear
                // EP node  594:Ignore
                // BDD node 152:vector_periodic_clear
                // EP node  702:Ignore
                // BDD node 153:vector_periodic_clear
                // EP node  804:Ignore
                // BDD node 154:vector_periodic_clear
                // EP node  898:Ignore
                // BDD node 155:vector_periodic_clear
                // EP node  982:Ignore
                // BDD node 156:vector_periodic_clear
                // EP node  1090:VectorTableLookup
                // BDD node 157:vector_borrow
                meta.key_32b_0 = meta.dev;
                vector_table_1246077374464_157.apply();
                // EP node  1166:Ignore
                // BDD node 158:vector_return
                // EP node  1265:If
                // BDD node 159:if
                if ((32w0x00000000) == (vector_table_1246077374464_157_get_value_param0)){
                  // EP node  1266:Then
                  // BDD node 159:if
                  // EP node  1403:VectorRegisterLookup
                  // BDD node 160:vector_borrow
                  meta.regexec_vector_register_1249835474944_0_read_1403_index0 = (bit<32>)(hdr.hdr1.data4[31:24]);
                  regexec_vector_register_1249835474944_0_read_1403();
                  // EP node  1762:SendToController
                  // BDD node 161:vector_return
                  fwd_op = fwd_op_t.FORWARD_TO_CPU;
                  build_cpu_hdr(0);
                  hdr.cpu.time = meta.time;
                  hdr.cpu.vector_reg_value0 = meta.vector_reg_value0;
                  hdr.cpu.dev = meta.dev;
                } else {
                  // EP node  1267:Else
                  // BDD node 159:if
                  // EP node  1481:VectorRegisterLookup
                  // BDD node 184:vector_borrow
                  regexec_vector_register_1252519837696_0_read_1481();
                  // EP node  1889:Ignore
                  // BDD node 185:vector_return
                  // EP node  2034:SendToEgress
                  // BDD node 186:packet_return_chunk
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
            } else {
              // EP node  45:Else
              // BDD node 147:if
              // EP node  3090:ParserReject
              // BDD node 194:DROP
            }
          }
        } else {
          // EP node  7:Else
          // BDD node 145:if
          // EP node  2879:ParserReject
          // BDD node 196:DROP
        }
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
      // EP node  2396:If
      // BDD node 189:if
      if ((hdr.egress_state.e32_0[15:0]) != (hdr.egress_state.e16_0)){
        // EP node  2397:Then
        // BDD node 189:if
        // EP node  2476:Forward
        // BDD node 190:FORWARD
        hdr.recirc.setInvalid();
        hdr.egress_state.setInvalid();
      } else {
        // EP node  2398:Else
        // BDD node 189:if
        // EP node  2797:Drop
        // BDD node 191:DROP
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
