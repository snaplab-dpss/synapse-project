
#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header out_h { bit<32> f0; }
header st_h { bit<32> v0; bit<32> v1; bit<32> v2; bit<32> v3; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; st_h st; out_h o; }
struct meta_t { bit<32> sa0; bit<32> sa1; bit<32> sa2; bit<32> sa3; bit<32> msg;  bit<32> v0; bit<32> v1; bit<32> v2; bit<32> v3; bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<16> w0; bit<16> w1; bit<8> b0; bit<8> b1; bit<12> t0; bit<12> t1; bit<32> o0; bit<32> o1; bit<32> o2; bit<32> o3; bit<32> o4; bit<32> o5; bit<32> o6; bit<32> o7; bit<32> o8; bit<32> o9; bit<32> o10; bit<32> o11; bit<32> o12; bit<32> o13; bit<32> o14; bit<32> o15; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

  action sip_init() {
    hdr.st.setValid();
    hdr.st.v0 = 32w0x33323130 ^ 32w0x70736575;
    hdr.st.v1 = 32w0x42413938 ^ 32w0x6e646f6d;
    hdr.st.v2 = 32w0x33323130 ^ 32w0x6e657261;
    hdr.st.v3 = 32w0x42413938 ^ 32w0x79746573;
    meta.msg = hdr.ipv4.src;
  }
  action sip_1_odd() { hdr.st.v3 = hdr.st.v3 ^ meta.msg; }
  action sip_1_a() {
    meta.sa0 = hdr.st.v0 + hdr.st.v1;
    meta.sa2 = hdr.st.v2 + hdr.st.v3;
    @in_hash { meta.sa1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; }
  }
  action sip_1_b() { meta.sa3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action sip_2_a() {
    hdr.st.v1 = meta.sa1 ^ meta.sa0;
    hdr.st.v3 = meta.sa3 ^ meta.sa2;
    hdr.st.v0 = meta.sa0[15:0] ++ meta.sa0[31:16];
    hdr.st.v2 = meta.sa2;
  }
  action sip_3_a() {
    meta.sa2 = hdr.st.v2 + hdr.st.v1;
    meta.sa0 = hdr.st.v0 + hdr.st.v3;
    @in_hash { meta.sa1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; }
  }
  action sip_3_b() { @in_hash { meta.sa3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action sip_4_a() {
    hdr.st.v1 = meta.sa1 ^ meta.sa2;
    hdr.st.v3 = meta.sa3 ^ meta.sa0;
    hdr.st.v2 = meta.sa2[15:0] ++ meta.sa2[31:16];
  }
  action sip_4_b_odd()  { hdr.st.v0 = meta.sa0; }
  action sip_4_b_even() { hdr.st.v0 = meta.sa0 ^ meta.msg; }
  action sip_out() { hdr.o.setValid(); @in_hash { hdr.o.f0 = hdr.st.v0 ^ hdr.st.v1 ^ hdr.st.v2 ^ hdr.st.v3; } }

  action init() { meta.v0 = hdr.ipv4.src; meta.v1 = hdr.ipv4.dst; meta.v2 = hdr.eth.dst[31:0]; meta.v3 = hdr.eth.src[31:0]; meta.w0 = hdr.ipv4.id; meta.w1 = hdr.ipv4.len; meta.b0 = hdr.ipv4.ttl; meta.b1 = hdr.ipv4.tos; meta.t0 = hdr.ipv4.id[11:0]; meta.t1 = hdr.ipv4.len[11:0]; }
  action fin2() { hdr.o.setValid(); hdr.o.f0 = 7; ig_tm_md.ucast_egress_port = 2; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = 1; ig_tm_md.ucast_egress_port = 1; }
  apply { init(); sip_init(); sip_1_odd(); sip_1_a(); sip_1_b(); sip_2_a(); sip_3_a(); sip_3_b(); sip_4_a(); sip_4_b_odd(); sip_1_a(); sip_1_b(); sip_2_a(); sip_3_a(); sip_3_b(); sip_4_a(); sip_4_b_even(); sip_out();  if (meta.t0 <= 12w100) { fin(); } else { fin2(); } }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) { state start { pkt.extract(eg_intr_md); transition accept; } }
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
