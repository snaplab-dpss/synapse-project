#include <core.p4>
#include <t2na.p4>

const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<16> ETHERTYPE_ST   = 0xff00;

header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header st_h { bit<32> v0; bit<32> v1; bit<32> v2; bit<32> v3; bit<8> round; @padding bit<24> pad; bit<32> ctime; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<32> ack; bit<16> off_flags; bit<16> win; bit<16> csum; bit<16> urg; }
struct headers_t { ethernet_h eth; st_h st; ipv4_h ipv4; tcp_h tcp; }
struct ig_md_t { bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<32> msg; bit<32> ts; bit<32> delta; bit<32> ctime; bit<1> b1; bit<1> b2; }
struct eg_md_t { bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<32> msg; }

parser IgParser(packet_in pkt, out headers_t hdr, out ig_md_t ig_md, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); transition parse_eth; }
  state parse_eth { pkt.extract(hdr.eth); transition select(hdr.eth.etype) { ETHERTYPE_ST: parse_st; ETHERTYPE_IPV4: parse_ipv4; default: accept; } }
  state parse_st { pkt.extract(hdr.st); transition parse_ipv4; }
  state parse_ipv4 { pkt.extract(hdr.ipv4); transition parse_tcp; }
  state parse_tcp { pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in ig_md_t ig_md, in ingress_intrinsic_metadata_for_deparser_t d) { apply { pkt.emit(hdr); } }

@pa_container_size("ingress","ig_md.a0",32)
@pa_container_size("ingress","ig_md.a1",32)
@pa_container_size("ingress","ig_md.a2",32)
@pa_container_size("ingress","ig_md.a3",32)
@pa_container_size("ingress","hdr.st.v0",32)
@pa_container_size("ingress","hdr.st.v1",32)
@pa_container_size("ingress","hdr.st.v2",32)
@pa_container_size("ingress","hdr.st.v3",32)

control Ig(inout headers_t hdr, inout ig_md_t ig_md, in ingress_intrinsic_metadata_t ig_intr_md,
           in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
           inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

  action sip_1_odd() { hdr.st.v3 = hdr.st.v3 ^ ig_md.msg; }
  action sip_1_a() {
    ig_md.a0 = hdr.st.v0 + hdr.st.v1;
    ig_md.a2 = hdr.st.v2 + hdr.st.v3;
    @in_hash { ig_md.a1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; }
  }
  action sip_1_b() { ig_md.a3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action sip_2_a() {
    hdr.st.v1 = ig_md.a1 ^ ig_md.a0;
    hdr.st.v3 = ig_md.a3 ^ ig_md.a2;
    hdr.st.v0 = ig_md.a0[15:0] ++ ig_md.a0[31:16];
    hdr.st.v2 = ig_md.a2;
  }
  action sip_3_a() {
    ig_md.a2 = hdr.st.v2 + hdr.st.v1;
    ig_md.a0 = hdr.st.v0 + hdr.st.v3;
    @in_hash { ig_md.a1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; }
  }
  action sip_3_b() { @in_hash { ig_md.a3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action sip_4_a() {
    hdr.st.v1 = ig_md.a1 ^ ig_md.a2;
    hdr.st.v3 = ig_md.a3 ^ ig_md.a0;
    hdr.st.v2 = ig_md.a2[15:0] ++ ig_md.a2[31:16];
  }
  action sip_4_b_odd()  { hdr.st.v0 = ig_md.a0; }
  action sip_4_b_even() { hdr.st.v0 = ig_md.a0 ^ ig_md.msg; }


  // cookie_time = (ticks(now) - stored_delta) >> 12
  Register<bit<32>,_>(1) reg_timedelta;
  RegisterAction<bit<32>, bit<32>, bit<32>>(reg_timedelta) regact_delta_read = {
    void apply(inout bit<32> value, out bit<32> ret) { ret = value; }
  };
  action ts_now()    { @in_hash { ig_md.ts = (bit<32>) ig_intr_md.ingress_mac_tstamp[47:16]; } }
  action delta_read(){ ig_md.delta = regact_delta_read.execute(0); }
  action ctime_sub() { ig_md.ctime = ig_md.ts - ig_md.delta; }
  action ctime_shr() { hdr.st.ctime = ig_md.ctime >> 12; }

  // per-flow bloom filter, two rows
  Register<bit<1>,_>(32w1048576) reg_bloom_1;
  Register<bit<1>,_>(32w1048576) reg_bloom_2;
  RegisterAction<bit<1>, bit<20>, bit<1>>(reg_bloom_1) regact_b1_get = {
    void apply(inout bit<1> value, out bit<1> ret) { ret = value; }
  };
  RegisterAction<bit<1>, bit<20>, bit<1>>(reg_bloom_2) regact_b2_get = {
    void apply(inout bit<1> value, out bit<1> ret) { ret = value; }
  };
  Hash<bit<20>>(HashAlgorithm_t.CRC16) hash_1;
  Hash<bit<20>>(HashAlgorithm_t.CRC32) hash_2;
  action bloom_get_1() { ig_md.b1 = regact_b1_get.execute(hash_1.get({ hdr.ipv4.src, hdr.ipv4.dst, hdr.tcp.sport, hdr.tcp.dport })); }
  action bloom_get_2() { ig_md.b2 = regact_b2_get.execute(hash_2.get({ 3w1, hdr.ipv4.src, 3w1, hdr.ipv4.dst, 3w1, hdr.tcp.sport, 3w1, hdr.tcp.dport })); }

  action sip_init() {
    hdr.st.setValid(); hdr.eth.etype = ETHERTYPE_ST; hdr.st.round = 0; hdr.st.ctime = 0;
    hdr.st.v0 = 32w0x33323130 ^ 32w0x70736575;
    hdr.st.v1 = 32w0x42413938 ^ 32w0x6e646f6d;
    hdr.st.v2 = 32w0x33323130 ^ 32w0x6e657261;
    hdr.st.v3 = 32w0x42413938 ^ 32w0x79746573;
  }
  action msg_src()   { ig_md.msg = hdr.ipv4.src; }
  action msg_dst()   { ig_md.msg = hdr.ipv4.dst; }
  action msg_ports() { ig_md.msg = hdr.tcp.sport ++ hdr.tcp.dport; }
  action msg_seq()   { ig_md.msg = hdr.tcp.seq; }
  action msg_zero()  { ig_md.msg = 0; }
  action bump()      { hdr.st.round = hdr.st.round + 2; }
  action recirc()    { ig_tm_md.ucast_egress_port = 6; }
  action fwd()       { ig_tm_md.ucast_egress_port = 1; }
  table pick_msg {
    key = { hdr.st.round: exact; }
    actions = { msg_src; msg_dst; msg_ports; msg_seq; msg_zero; }
    default_action = msg_zero;
    size = 16;
    const entries = { 0: msg_src(); 4: msg_dst(); 8: msg_ports(); 12: msg_seq(); }
  }
  apply {
    if (!hdr.st.isValid()) {
      sip_init();
      ts_now();
      delta_read();
      ctime_sub();
      ctime_shr();
      bloom_get_1();
      bloom_get_2();
    }
    pick_msg.apply();
    sip_1_odd();
    sip_1_a();
    sip_1_b();
    sip_2_a();
    sip_3_a();
    sip_3_b();
    sip_4_a();
    sip_4_b_odd();
    sip_1_a();
    sip_1_b();
    sip_2_a();
    sip_3_a();
    sip_3_b();
    sip_4_a();
    sip_4_b_even();
    bump();
    if (hdr.st.round >= 24) { fwd(); } else { recirc(); }
  }
}

parser EgParser(packet_in pkt, out headers_t hdr, out eg_md_t eg_md, out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); transition parse_eth; }
  state parse_eth { pkt.extract(hdr.eth); transition select(hdr.eth.etype) { ETHERTYPE_ST: parse_st; ETHERTYPE_IPV4: parse_ipv4; default: accept; } }
  state parse_st { pkt.extract(hdr.st); transition parse_ipv4; }
  state parse_ipv4 { pkt.extract(hdr.ipv4); transition parse_tcp; }
  state parse_tcp { pkt.extract(hdr.tcp); transition accept; }
}
control EgDeparser(packet_out pkt, inout headers_t hdr, in eg_md_t eg_md, in egress_intrinsic_metadata_for_deparser_t d) { apply { pkt.emit(hdr); } }

@pa_container_size("egress","eg_md.a0",32)
@pa_container_size("egress","eg_md.a1",32)
@pa_container_size("egress","eg_md.a2",32)
@pa_container_size("egress","eg_md.a3",32)
@pa_container_size("egress","hdr.st.v0",32)
@pa_container_size("egress","hdr.st.v1",32)
@pa_container_size("egress","hdr.st.v2",32)
@pa_container_size("egress","hdr.st.v3",32)

control Eg(inout headers_t hdr, inout eg_md_t eg_md, in egress_intrinsic_metadata_t eg_intr_md,
           in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
           inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) {

  action sip_1_odd() { hdr.st.v3 = hdr.st.v3 ^ eg_md.msg; }
  action sip_1_a() {
    eg_md.a0 = hdr.st.v0 + hdr.st.v1;
    eg_md.a2 = hdr.st.v2 + hdr.st.v3;
    @in_hash { eg_md.a1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; }
  }
  action sip_1_b() { eg_md.a3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action sip_2_a() {
    hdr.st.v1 = eg_md.a1 ^ eg_md.a0;
    hdr.st.v3 = eg_md.a3 ^ eg_md.a2;
    hdr.st.v0 = eg_md.a0[15:0] ++ eg_md.a0[31:16];
    hdr.st.v2 = eg_md.a2;
  }
  action sip_3_a() {
    eg_md.a2 = hdr.st.v2 + hdr.st.v1;
    eg_md.a0 = hdr.st.v0 + hdr.st.v3;
    @in_hash { eg_md.a1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; }
  }
  action sip_3_b() { @in_hash { eg_md.a3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action sip_4_a() {
    hdr.st.v1 = eg_md.a1 ^ eg_md.a2;
    hdr.st.v3 = eg_md.a3 ^ eg_md.a0;
    hdr.st.v2 = eg_md.a2[15:0] ++ eg_md.a2[31:16];
  }
  action sip_4_b_odd()  { hdr.st.v0 = eg_md.a0; }
  action sip_4_b_even() { hdr.st.v0 = eg_md.a0 ^ eg_md.msg; }

  action msg_src()   { eg_md.msg = hdr.ipv4.src; }
  action msg_dst()   { eg_md.msg = hdr.ipv4.dst; }
  action msg_ports() { eg_md.msg = hdr.tcp.sport ++ hdr.tcp.dport; }
  action msg_seq()   { eg_md.msg = hdr.tcp.seq; }
  action msg_zero()  { eg_md.msg = 0; }
  action bump()      { hdr.st.round = hdr.st.round + 2; }
  action finish_hash() { @in_hash { hdr.tcp.seq = hdr.st.ctime ^ hdr.st.v0 ^ hdr.st.v1 ^ hdr.st.v2 ^ hdr.st.v3; } }
  action nop() {}
  action cleanup() { hdr.st.setInvalid(); hdr.eth.etype = ETHERTYPE_IPV4; }
  table tb_finish {
    key = { hdr.st.round: exact; }
    actions = { finish_hash; nop; }
    default_action = nop;
    size = 16;
    const entries = { 24: finish_hash(); }
  }
  table pick_msg {
    key = { hdr.st.round: exact; }
    actions = { msg_src; msg_dst; msg_ports; msg_seq; msg_zero; }
    default_action = msg_zero;
    size = 16;
    const entries = { 2: msg_src(); 6: msg_dst(); 10: msg_ports(); 14: msg_seq(); }
  }
  apply {
    if (hdr.st.isValid()) {
      pick_msg.apply();
      sip_1_odd();
      sip_1_a();
      sip_1_b();
      sip_2_a();
      sip_3_a();
      sip_3_b();
      sip_4_a();
      sip_4_b_odd();
      sip_1_a();
      sip_1_b();
      sip_2_a();
      sip_3_a();
      sip_3_b();
      sip_4_a();
      sip_4_b_even();
      bump();
      tb_finish.apply();
      if (hdr.st.round >= 24) { cleanup(); }
    }
  }
}
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
