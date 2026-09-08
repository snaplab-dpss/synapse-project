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

const bit<16> SIP_CODE_PATH  = 0xff00; // ethertype marking a packet that carries hash state
const bit<16> SERVER_DEV     = 136;
const bit<8>  CB_SYNACK      = 1;
const bit<8>  CB_TAGACK      = 2;
const bit<8>  SIP_ROUNDS     = 12;

// HalfSipHash constants already xored with the key
const bit<32> SIP_V0 = 0x33323130 ^ 0x70736575;
const bit<32> SIP_V1 = 0x42413938 ^ 0x6e646f6d;
const bit<32> SIP_V2 = 0x33323130 ^ 0x6e657261;
const bit<32> SIP_V3 = 0x42413938 ^ 0x79746573;

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
}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<16> dev;
}

// The state the hash carries from lap to lap.
header recirc_state_h {
  bit<32> v0;
  bit<32> v1;
  bit<32> v2;
  bit<32> v3;
  bit<32> ctime;
  bit<8>  round;
  bit<8>  cb;
}

header hdr0_h {          // ethernet
  bit<96> data0;
  bit<16> data1;         // ethertype
}
header hdr1_h {          // ipv4
  bit<8>  data0;         // version + ihl
  bit<24> data1;         // tos + total length
  bit<40> data2;         // id + flags/frag + ttl
  bit<24> data3;         // protocol + header checksum
  bit<32> data4;         // source address
  bit<32> data5;         // destination address
}
header hdr2_h {          // tcp
  bit<32> ports;         // source port ++ destination port
  bit<32> data2;         // sequence number
  bit<32> data3;         // acknowledgement number
  bit<16> data4;         // data offset + flags
  bit<48> data5;         // window + checksum + urgent pointer
}
header hdr3_h {          // udp
  bit<16> data0;
  bit<48> data1;
}
header hdr4_h {          // the server agent's clock update
  bit<32> data0;
}

struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  recirc_state_h recirc_state;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;
  hdr4_h hdr4;
}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<16> dev;
  bit<32> time;
  bit<32> a0;
  bit<32> a1;
  bit<32> a2;
  bit<32> a3;
  bit<32> msg;
  bit<32> delta;
  bit<32> ctime;
  bit<1>  bf_estimate;
  bit<1>  is_server;
}

struct synapse_egress_headers_t {
  recirc_h recirc;
  recirc_state_h recirc_state;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
}

struct synapse_egress_metadata_t {
  bit<32> a0;
  bit<32> a1;
  bit<32> a2;
  bit<32> a3;
  bit<32> msg;
  bit<32> cookie_val;
  bit<32> age;
  bit<16> tcp_len;
  bit<8>  proto;      // staged out of hdr1.data3: a checksum input cannot be a slice
  bit<16> window;     // staged out of hdr2.data5
  bit<16> urgent;     // staged out of hdr2.data5
  bit<1>  redo_checksum;
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
      SIP_CODE_PATH: parse_recirc_state;
      default: parser_init;
    }
  }

  state parse_recirc_state {
    pkt.extract(hdr.recirc_state);
    transition parser_init;
  }

  state parser_init {
    pkt.extract(hdr.hdr0);
    transition parser_3;
  }
  state parser_3 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_4;
      default: parser_reject;
    }
  }
  state parser_reject {
    transition reject;
  }
  state parser_4 {
    pkt.extract(hdr.hdr1);
    transition parser_5;
  }
  state parser_5 {
    transition select (hdr.hdr1.data3[23:16]) {
      8w0x06: parser_tcp;
      8w0x11: parser_udp;
      default: accept;
    }
  }
  state parser_tcp {
    pkt.extract(hdr.hdr2);
    transition accept;
  }
  state parser_udp {
    pkt.extract(hdr.hdr3);
    transition select (hdr.hdr3.data0) {
      16w5555: parser_timesync;
      default: accept;
    }
  }
  state parser_timesync {
    pkt.extract(hdr.hdr4);
    transition accept;
  }
}

@pa_container_size("ingress", "meta.a0", 32)
@pa_container_size("ingress", "meta.a1", 32)
@pa_container_size("ingress", "meta.a2", 32)
@pa_container_size("ingress", "meta.a3", 32)
@pa_container_size("ingress", "hdr.recirc_state.v0", 32)
@pa_container_size("ingress", "hdr.recirc_state.v1", 32)
@pa_container_size("ingress", "hdr.recirc_state.v2", 32)
@pa_container_size("ingress", "hdr.recirc_state.v3", 32)
control Ingress(
  inout synapse_ingress_headers_t hdr,
  inout synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_t ig_intr_md,
  in    ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr,
  inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
  inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md
) {
  action drop() {
    ig_intr_dprsr_md.drop_ctl = 1;
    ig_intr_tm_md.bypass_egress = 1;
  }

  action fwd(bit<16> port) {
    ig_intr_tm_md.ucast_egress_port = (bit<9>)port;
    ig_intr_tm_md.bypass_egress = 1;
  }

  action fwd_nf_dev(bit<16> port) {
    fwd(port);
  }

  action set_ingress_dev(bit<16> nf_dev) {
    meta.dev = nf_dev;
  }

  action set_ingress_dev_from_recirculation() {
    meta.ingress_port = hdr.recirc.ingress_port;
    meta.dev = hdr.recirc.dev;
  }

  table ingress_port_to_nf_dev {
    key = { meta.ingress_port: exact; }
    actions = { set_ingress_dev; set_ingress_dev_from_recirculation; }
    default_action = set_ingress_dev_from_recirculation();
    size = 64;
  }

  // ---------------------------------------------------------------------
  // One SipRound. This is the only copy of it in the ingress pipeline; the
  // lap counter in the recirculation state decides which round it is.
  // ---------------------------------------------------------------------
  action sip_1_odd() { hdr.recirc_state.v3 = hdr.recirc_state.v3 ^ meta.msg; }
  action sip_1_a() {
    meta.a0 = hdr.recirc_state.v0 + hdr.recirc_state.v1;
    meta.a2 = hdr.recirc_state.v2 + hdr.recirc_state.v3;
    @in_hash { meta.a1 = hdr.recirc_state.v1[26:0] ++ hdr.recirc_state.v1[31:27]; }
  }
  action sip_1_b() { meta.a3 = hdr.recirc_state.v3[23:0] ++ hdr.recirc_state.v3[31:24]; }
  action sip_2_a() {
    hdr.recirc_state.v1 = meta.a1 ^ meta.a0;
    hdr.recirc_state.v3 = meta.a3 ^ meta.a2;
    hdr.recirc_state.v0 = meta.a0[15:0] ++ meta.a0[31:16];
    hdr.recirc_state.v2 = meta.a2;
  }
  action sip_3_a() {
    meta.a2 = hdr.recirc_state.v2 + hdr.recirc_state.v1;
    meta.a0 = hdr.recirc_state.v0 + hdr.recirc_state.v3;
    @in_hash { meta.a1 = hdr.recirc_state.v1[18:0] ++ hdr.recirc_state.v1[31:19]; }
  }
  action sip_3_b() { @in_hash { meta.a3 = hdr.recirc_state.v3[24:0] ++ hdr.recirc_state.v3[31:25]; } }
  action sip_4_a() {
    hdr.recirc_state.v1 = meta.a1 ^ meta.a2;
    hdr.recirc_state.v3 = meta.a3 ^ meta.a0;
    hdr.recirc_state.v2 = meta.a2[15:0] ++ meta.a2[31:16];
  }
  action sip_4_b_odd()  { hdr.recirc_state.v0 = meta.a0; }
  action sip_4_b_even() { hdr.recirc_state.v0 = meta.a0 ^ meta.msg; }

  action msg_src()   { meta.msg = hdr.hdr1.data4; }
  action msg_ports() { meta.msg = hdr.hdr2.ports; }
  action msg_zero()  { meta.msg = 0; }
  table sip_message {
    key = { hdr.recirc_state.round: exact; }
    actions = { msg_src; msg_ports; msg_zero; }
    default_action = msg_zero();
    size = 8;
    const entries = { 0: msg_src(); 4: msg_ports(); }
  }
  action sip_bump() { hdr.recirc_state.round = hdr.recirc_state.round + 2; }

  // ---------------------------------------------------------------------
  // cookie_time = (ticks(now) - stored delta) >> 12
  // ---------------------------------------------------------------------
  Register<bit<32>, _>(1) timedelta;
  RegisterAction<bit<32>, bit<32>, bit<32>>(timedelta) timedelta_read = {
    void apply(inout bit<32> value, out bit<32> ret) { ret = value; }
  };
  RegisterAction<bit<32>, bit<32>, bit<32>>(timedelta) timedelta_write = {
    void apply(inout bit<32> value, out bit<32> ret) { value = meta.delta; ret = 0; }
  };
  action time_read()   { @in_hash { meta.ctime = (bit<32>) ig_intr_md.ingress_mac_tstamp[47:16]; } }
  action delta_read()  { meta.delta = timedelta_read.execute(0); }
  action ctime_sub()   { meta.ctime = meta.ctime - meta.delta; }
  action ctime_shift() { meta.ctime = meta.ctime >> 12; }
  action delta_calc()  { meta.delta = meta.ctime - hdr.hdr4.data0; }
  action delta_write() { timedelta_write.execute(0); }

  // ---------------------------------------------------------------------
  // Bloom filter over the flows the server has confirmed
  // ---------------------------------------------------------------------
  Register<bit<1>, _>(32w1048576) bf_row_0;
  Register<bit<1>, _>(32w1048576) bf_row_1;
  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_row_0) bf_row_0_read = {
    void apply(inout bit<1> value, out bit<1> ret) { ret = value; } };
  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_row_0) bf_row_0_set = {
    void apply(inout bit<1> value, out bit<1> ret) { value = 1; ret = 0; } };
  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_row_1) bf_row_1_read = {
    void apply(inout bit<1> value, out bit<1> ret) { ret = value; } };
  RegisterAction<bit<1>, bit<20>, bit<1>>(bf_row_1) bf_row_1_set = {
    void apply(inout bit<1> value, out bit<1> ret) { value = 1; ret = 0; } };
  Hash<bit<20>>(HashAlgorithm_t.CRC16) bf_hash_0;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) bf_hash_1;
  bit<1> bf_read_0 = 0;
  bit<1> bf_read_1 = 0;
  action bf_query_0() { bf_read_0 = bf_row_0_read.execute(bf_hash_0.get({ hdr.hdr1.data4, hdr.hdr1.data5, hdr.hdr2.ports })); }
  action bf_query_1() { bf_read_1 = bf_row_1_read.execute(bf_hash_1.get({ 3w1, hdr.hdr1.data4, 3w1, hdr.hdr1.data5, 3w1, hdr.hdr2.ports })); }
  action bf_add_0()   { bf_row_0_set.execute(bf_hash_0.get({ hdr.hdr1.data4, hdr.hdr1.data5, hdr.hdr2.ports })); }
  action bf_add_1()   { bf_row_1_set.execute(bf_hash_1.get({ 3w1, hdr.hdr1.data4, 3w1, hdr.hdr1.data5, 3w1, hdr.hdr2.ports })); }
  action bf_estimate()      { meta.bf_estimate = bf_read_0 & bf_read_1; }
  action bf_estimate_zero() { meta.bf_estimate = 0; }

  // ---------------------------------------------------------------------
  // Forwarding
  // ---------------------------------------------------------------------
  fwd_op_t fwd_op = fwd_op_t.DROP;
  bit<16> nf_dev = 0;

  action route()     { @in_hash { ig_intr_tm_md.ucast_egress_port = (bit<9>) hdr.hdr1.data5[31:24]; }
                       ig_intr_tm_md.bypass_egress = 1; }
  action to_server() { fwd(SERVER_DEV); }
  action recirculate() {
    ig_intr_tm_md.ucast_egress_port = RECIRCULATION_PORT_0;
    ig_intr_tm_md.bypass_egress = 0;
  }
  action deliver() {
    ig_intr_tm_md.ucast_egress_port = (bit<9>)hdr.recirc.dev;
    ig_intr_tm_md.bypass_egress = 0;
  }

  action build_recirc_hdr(bit<16> code_path) {
    hdr.recirc.setValid();
    hdr.recirc.code_path = code_path;
    hdr.recirc.ingress_port = meta.ingress_port;
    hdr.recirc.dev = meta.dev;
  }

  action sip_start(bit<8> cb, bit<16> dev) {
    hdr.recirc_state.setValid();
    hdr.recirc_state.v0 = SIP_V0;
    hdr.recirc_state.v1 = SIP_V1;
    hdr.recirc_state.v2 = SIP_V2;
    hdr.recirc_state.v3 = SIP_V3;
    hdr.recirc_state.round = 0;
    hdr.recirc_state.cb = cb;
    hdr.recirc_state.ctime = meta.ctime;
    meta.dev = dev;
  }
  action start_synack() { sip_start(CB_SYNACK, meta.ingress_port); }
  action start_tagack() { sip_start(CB_TAGACK, SERVER_DEV); }
  action nop() {}

  // The branch structure of nf_process, as one table.
  table triage {
    key = {
      hdr.hdr2.isValid()  : exact;
      hdr.hdr4.isValid()  : exact;
      meta.is_server      : ternary;
      hdr.hdr2.data4[1:1] : ternary; // SYN
      hdr.hdr2.data4[4:4] : ternary; // ACK
      hdr.hdr2.data4[6:6] : ternary; // ECE
      meta.bf_estimate    : ternary;
    }
    actions = { drop; route; to_server; start_synack; start_tagack; nop; }
    default_action = drop();
    size = 32;
    const entries = {
      // the server's clock update was applied before this table
      (false, true,  1, _, _, _, _) : drop();
      // any other non-TCP packet is routed by the destination address
      (false, false, _, _, _, _, _) : route();
      // from the server: an ECE tag records the flow, anything else is plain traffic
      (true,  false, 1, _, _, 1, _) : drop();
      (true,  false, 1, _, _, 0, _) : route();
      // from a client: a SYN earns a cookie, a SYN-ACK is dropped
      (true,  false, 0, 1, 0, _, _) : start_synack();
      (true,  false, 0, 1, 1, _, _) : drop();
      // an already verified flow goes straight to the server
      (true,  false, 0, 0, _, _, 1) : to_server();
      // otherwise the cookie carried in the ACK is checked
      (true,  false, 0, 0, _, _, 0) : start_tagack();
    }
  }

  action mark_server()     { meta.is_server = 1; }
  action mark_not_server() { meta.is_server = 0; }

  apply {
    ingress_port_to_nf_dev.apply();

    if (meta.ingress_port == SERVER_DEV) { mark_server(); } else { mark_not_server(); }

    if (!hdr.recirc_state.isValid()) {
      // first pass: work out the cookie epoch, consult the bloom filter, then triage
      time_read();

      if (hdr.hdr4.isValid() && meta.is_server == 1) {
        delta_calc();
        delta_write();
        drop();
      } else {
        delta_read();
        ctime_sub();
        ctime_shift();

        bf_estimate_zero();
        if (hdr.hdr2.isValid()) {
          if (meta.is_server == 1 && hdr.hdr2.data4[6:6] == 1) {
            bf_add_0();
            bf_add_1();
          } else {
            bf_query_0();
            bf_query_1();
            bf_estimate();
          }
        }

        triage.apply();
      }
    }

    if (hdr.recirc_state.isValid()) {
      // two of the twelve rounds, then hand the packet to egress for two more
      sip_message.apply();
      sip_1_odd();
      sip_1_a(); sip_1_b(); sip_2_a(); sip_3_a(); sip_3_b(); sip_4_a(); sip_4_b_odd();
      sip_1_a(); sip_1_b(); sip_2_a(); sip_3_a(); sip_3_b(); sip_4_a(); sip_4_b_even();
      sip_bump();

      build_recirc_hdr(SIP_CODE_PATH);
      hdr.hdr0.data1 = SIP_CODE_PATH;
      if (hdr.recirc_state.round >= SIP_ROUNDS) { deliver(); } else { recirculate(); }
    }
  }
}

control IngressDeparser(
  packet_out pkt,
  inout synapse_ingress_headers_t hdr,
  in    synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md
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
    transition parse_recirc;
  }

  // Only packets carrying hash state reach egress; everything else bypasses it.
  state parse_recirc {
    pkt.extract(hdr.recirc);
    pkt.extract(hdr.recirc_state);
    pkt.extract(hdr.hdr0);
    pkt.extract(hdr.hdr1);
    pkt.extract(hdr.hdr2);
    transition accept;
  }
}

@pa_container_size("egress", "eg_md.a0", 32)
@pa_container_size("egress", "eg_md.a1", 32)
@pa_container_size("egress", "eg_md.a2", 32)
@pa_container_size("egress", "eg_md.a3", 32)
@pa_container_size("egress", "hdr.recirc_state.v0", 32)
@pa_container_size("egress", "hdr.recirc_state.v1", 32)
@pa_container_size("egress", "hdr.recirc_state.v2", 32)
@pa_container_size("egress", "hdr.recirc_state.v3", 32)
control Egress(
  inout synapse_egress_headers_t hdr,
  inout synapse_egress_metadata_t eg_md,
  in    egress_intrinsic_metadata_t eg_intr_md,
  in    egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
  inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
  inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
  // The same SipRound again: two more of the twelve happen here.
  action sip_1_odd() { hdr.recirc_state.v3 = hdr.recirc_state.v3 ^ eg_md.msg; }
  action sip_1_a() {
    eg_md.a0 = hdr.recirc_state.v0 + hdr.recirc_state.v1;
    eg_md.a2 = hdr.recirc_state.v2 + hdr.recirc_state.v3;
    @in_hash { eg_md.a1 = hdr.recirc_state.v1[26:0] ++ hdr.recirc_state.v1[31:27]; }
  }
  action sip_1_b() { eg_md.a3 = hdr.recirc_state.v3[23:0] ++ hdr.recirc_state.v3[31:24]; }
  action sip_2_a() {
    hdr.recirc_state.v1 = eg_md.a1 ^ eg_md.a0;
    hdr.recirc_state.v3 = eg_md.a3 ^ eg_md.a2;
    hdr.recirc_state.v0 = eg_md.a0[15:0] ++ eg_md.a0[31:16];
    hdr.recirc_state.v2 = eg_md.a2;
  }
  action sip_3_a() {
    eg_md.a2 = hdr.recirc_state.v2 + hdr.recirc_state.v1;
    eg_md.a0 = hdr.recirc_state.v0 + hdr.recirc_state.v3;
    @in_hash { eg_md.a1 = hdr.recirc_state.v1[18:0] ++ hdr.recirc_state.v1[31:19]; }
  }
  action sip_3_b() { @in_hash { eg_md.a3 = hdr.recirc_state.v3[24:0] ++ hdr.recirc_state.v3[31:25]; } }
  action sip_4_a() {
    hdr.recirc_state.v1 = eg_md.a1 ^ eg_md.a2;
    hdr.recirc_state.v3 = eg_md.a3 ^ eg_md.a0;
    hdr.recirc_state.v2 = eg_md.a2[15:0] ++ eg_md.a2[31:16];
  }
  action sip_4_b_odd()  { hdr.recirc_state.v0 = eg_md.a0; }
  action sip_4_b_even() { hdr.recirc_state.v0 = eg_md.a0 ^ eg_md.msg; }

  // The SYN path hashes the sequence number, the ACK path hashes it minus one.
  action msg_dst()    { eg_md.msg = hdr.hdr1.data5; }
  action msg_seq()    { eg_md.msg = hdr.hdr2.data2; }
  action msg_seq_m1() { eg_md.msg = hdr.hdr2.data2 - 1; }
  action msg_zero()   { eg_md.msg = 0; }
  table sip_message {
    key = { hdr.recirc_state.round: exact; hdr.recirc_state.cb: ternary; }
    actions = { msg_dst; msg_seq; msg_seq_m1; msg_zero; }
    default_action = msg_zero();
    size = 8;
    const entries = {
      (2, _)         : msg_dst();
      (6, CB_SYNACK) : msg_seq();
      (6, CB_TAGACK) : msg_seq_m1();
    }
  }
  action sip_bump() { hdr.recirc_state.round = hdr.recirc_state.round + 2; }

  // The final xor needs the hash unit, so it goes in a table; and two hash-producing
  // actions cannot share one table, so there is one table per callback type.
  action final_synack() {
    @in_hash { hdr.hdr2.data2 = hdr.recirc_state.ctime ^ hdr.recirc_state.v0 ^ hdr.recirc_state.v1
                                                       ^ hdr.recirc_state.v2 ^ hdr.recirc_state.v3; }
  }
  action final_tagack() {
    @in_hash { eg_md.cookie_val = hdr.hdr2.data3 ^ hdr.recirc_state.v0 ^ hdr.recirc_state.v1
                                                 ^ hdr.recirc_state.v2 ^ hdr.recirc_state.v3; }
  }
  action nop() {}
  table sip_final_synack {
    key = { hdr.recirc_state.round: exact; hdr.recirc_state.cb: ternary; }
    actions = { final_synack; nop; }
    default_action = nop();
    size = 8;
    const entries = { (12, CB_SYNACK): final_synack(); }
  }
  table sip_final_tagack {
    key = { hdr.recirc_state.round: exact; hdr.recirc_state.cb: ternary; }
    actions = { final_tagack; nop; }
    default_action = nop();
    size = 8;
    const entries = { (12, CB_TAGACK): final_tagack(); }
  }

  action ack_from_seq() { hdr.hdr2.data3 = hdr.hdr2.data2 + 1; }
  action craft_synack() {
    // An action reads all its sources before writing any destination, so these two
    // statements swap the addresses rather than duplicating one of them.
    hdr.hdr1.data4 = hdr.hdr1.data5;
    hdr.hdr1.data5 = hdr.hdr1.data4;
    hdr.hdr2.ports = hdr.hdr2.ports[15:0] ++ hdr.hdr2.ports[31:16];
    hdr.hdr2.data4 = 8w0x50 ++ (hdr.hdr2.data4[7:0] | 8w0x12);
    hdr.hdr1.data0 = 8w0x45;
    hdr.hdr1.data1[15:0] = 16w0x0028;
    eg_md.redo_checksum = 1;
    eg_md.tcp_len = 20;
  }
  action age_calc() { eg_md.age = hdr.recirc_state.ctime - eg_md.cookie_val; }
  action tag_onward() {
    hdr.hdr2.data2 = hdr.hdr2.data2 - 1;
    hdr.hdr2.data4 = 8w0x50 ++ (hdr.hdr2.data4[7:0] | 8w0x40);
    hdr.hdr1.data0 = 8w0x45;
    hdr.hdr1.data1[15:0] = 16w0x0028;
    eg_md.redo_checksum = 1;
    eg_md.tcp_len = 20;
  }
  action drop() { ig_intr_dprs_md.drop_ctl = 1; }
  action stage_checksum_fields() {
    eg_md.proto  = hdr.hdr1.data3[23:16];
    eg_md.window = hdr.hdr2.data5[47:32];
    eg_md.urgent = hdr.hdr2.data5[15:0];
  }

  // A 32-bit inequality does not fit a gateway, so the accepted epochs are entries.
  table cookie_age {
    key = { eg_md.age: exact; }
    actions = { drop; nop; }
    default_action = drop();
    size = 8;
    const entries = { 0: nop(); 1: nop(); 2: nop(); }
  }

  action strip_state() {
    hdr.recirc.setInvalid();
    hdr.recirc_state.setInvalid();
    hdr.hdr0.data1 = 16w0x0800;
  }
  action keep_state() {}

  apply {
    eg_md.redo_checksum = 0;
    stage_checksum_fields();

    if (hdr.recirc_state.isValid()) {
      sip_message.apply();
      sip_1_odd();
      sip_1_a(); sip_1_b(); sip_2_a(); sip_3_a(); sip_3_b(); sip_4_a(); sip_4_b_odd();
      sip_1_a(); sip_1_b(); sip_2_a(); sip_3_a(); sip_3_b(); sip_4_a(); sip_4_b_even();
      sip_bump();

      sip_final_synack.apply();
      sip_final_tagack.apply();

      if (hdr.recirc_state.round >= SIP_ROUNDS) {
        if (hdr.recirc_state.cb == CB_SYNACK) {
          ack_from_seq();
          craft_synack();
        } else {
          age_calc();
          tag_onward();
          cookie_age.apply();
        }
        strip_state();
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
  Checksum() ipv4_checksum;
  Checksum() tcp_checksum;

  apply {
    if (eg_md.redo_checksum == 1) {
      hdr.hdr1.data3[15:0] = ipv4_checksum.update({
        hdr.hdr1.data0, hdr.hdr1.data1, hdr.hdr1.data2, eg_md.proto,
        hdr.hdr1.data4, hdr.hdr1.data5
      });
      hdr.hdr2.data5[31:16] = tcp_checksum.update({
        hdr.hdr1.data4, hdr.hdr1.data5, 8w0, eg_md.proto, eg_md.tcp_len,
        hdr.hdr2.ports, hdr.hdr2.data2, hdr.hdr2.data3, hdr.hdr2.data4,
        eg_md.window, eg_md.urgent
      });
    }
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
