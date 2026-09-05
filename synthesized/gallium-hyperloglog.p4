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
  bit<32> find_first_set_bit_7_out;
  bit<32> hll_hash0;
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
  bit<48> data0;
  bit<48> data1;
  bit<16> data2;
}
header hdr1_h {
  bit<96> data0;
  bit<64> data1;
}


struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;
  bit<32> find_first_set_bit_7_key;
  bit<32> find_first_set_bit_7_out;

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
    transition parser_4;
  }
  state parser_4 {
    transition parser_4_0;
  }
  state parser_4_0 {
    transition select (hdr.hdr0.data2) {
      16w0x0800: parser_5;
      default: parser_81;
    }
  }
  state parser_5 {
    pkt.extract(hdr.hdr1);
    transition parser_31;
  }
  state parser_81 {
    transition reject;
  }
  state parser_31 {
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

  Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_6;
  bit<32> hash_6_value;
  action hash_6_calc() {
    hash_6_value = hash_6.get({
      hdr.hdr1.data1
      });
  }
  action find_first_set_bit_7_get_value(bit<32> v) {
    meta.find_first_set_bit_7_out = v;
  }
  table find_first_set_bit_7 {
    key = { meta.find_first_set_bit_7_key: ternary; }
    actions = { find_first_set_bit_7_get_value; }
    size = 37;
    default_action = find_first_set_bit_7_get_value(0);
    const entries = {
      32w1 &&& 32w1 : find_first_set_bit_7_get_value(1);
      32w2 &&& 32w3 : find_first_set_bit_7_get_value(2);
      32w4 &&& 32w7 : find_first_set_bit_7_get_value(3);
      32w8 &&& 32w15 : find_first_set_bit_7_get_value(4);
      32w16 &&& 32w31 : find_first_set_bit_7_get_value(5);
      32w32 &&& 32w63 : find_first_set_bit_7_get_value(6);
      32w64 &&& 32w127 : find_first_set_bit_7_get_value(7);
      32w128 &&& 32w255 : find_first_set_bit_7_get_value(8);
      32w256 &&& 32w511 : find_first_set_bit_7_get_value(9);
      32w512 &&& 32w1023 : find_first_set_bit_7_get_value(10);
      32w1024 &&& 32w2047 : find_first_set_bit_7_get_value(11);
      32w2048 &&& 32w4095 : find_first_set_bit_7_get_value(12);
      32w4096 &&& 32w8191 : find_first_set_bit_7_get_value(13);
      32w8192 &&& 32w16383 : find_first_set_bit_7_get_value(14);
      32w16384 &&& 32w32767 : find_first_set_bit_7_get_value(15);
      32w32768 &&& 32w65535 : find_first_set_bit_7_get_value(16);
      32w65536 &&& 32w131071 : find_first_set_bit_7_get_value(17);
      32w131072 &&& 32w262143 : find_first_set_bit_7_get_value(18);
      32w262144 &&& 32w524287 : find_first_set_bit_7_get_value(19);
      32w524288 &&& 32w1048575 : find_first_set_bit_7_get_value(20);
      32w1048576 &&& 32w2097151 : find_first_set_bit_7_get_value(21);
      32w2097152 &&& 32w4194303 : find_first_set_bit_7_get_value(22);
      32w4194304 &&& 32w8388607 : find_first_set_bit_7_get_value(23);
      32w8388608 &&& 32w16777215 : find_first_set_bit_7_get_value(24);
      32w16777216 &&& 32w33554431 : find_first_set_bit_7_get_value(25);
      32w33554432 &&& 32w67108863 : find_first_set_bit_7_get_value(26);
      32w67108864 &&& 32w134217727 : find_first_set_bit_7_get_value(27);
      32w134217728 &&& 32w268435455 : find_first_set_bit_7_get_value(28);
      32w268435456 &&& 32w536870911 : find_first_set_bit_7_get_value(29);
      32w536870912 &&& 32w1073741823 : find_first_set_bit_7_get_value(30);
      32w1073741824 &&& 32w2147483647 : find_first_set_bit_7_get_value(31);
      32w2147483648 &&& 32w4294967295 : find_first_set_bit_7_get_value(32);
    }
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  0:ParserExtraction
      // BDD node 3:packet_borrow_next_chunk
      if(hdr.hdr0.isValid()) {
        // EP node  5:ParserCondition
        // BDD node 4:if
        // EP node  6:Then
        // BDD node 4:if
        // EP node  16:ParserExtraction
        // BDD node 5:packet_borrow_next_chunk
        if(hdr.hdr1.isValid()) {
          // EP node  33:HashObj
          // BDD node 6:hash_obj
          hash_6_calc();
          bit<32> hll_hash0 = hash_6_value;
          // EP node  60:FindFirstSetBit
          // BDD node 7:find_first_set_bit
          meta.find_first_set_bit_7_key = (hll_hash0) & (32w0x000fffff);
          find_first_set_bit_7.apply();
          // EP node  125:SendToController
          // BDD node 8:vector_borrow
          fwd_op = fwd_op_t.FORWARD_TO_CPU;
          build_cpu_hdr(125);
          hdr.cpu.find_first_set_bit_7_out = meta.find_first_set_bit_7_out;
          hdr.cpu.hll_hash0 = hll_hash0;
          hdr.cpu.dev = meta.dev;
        }
        // EP node  7:Else
        // BDD node 4:if
        // EP node  180:ParserReject
        // BDD node 81:DROP
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
