#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<16> data3; bit<16> win; }
header out_h { bit<32> f0; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; tcp_h tcp; out_h o; }
struct meta_t { bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<32> a4; bit<32> a5; bit<32> a6; bit<32> a7; bit<32> a8; bit<32> a9; bit<32> a10; bit<32> a11; bit<32> a12; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) { apply { pkt.emit(hdr); } }
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action p_0() { meta.a1 = meta.a0 ^ 32w1; }
  action p_1() { meta.a2 = meta.a1 ^ 32w2; }
  action p_2() { meta.a3 = meta.a2 ^ 32w3; }
  action p_3() { meta.a4 = meta.a3 ^ 32w4; }
  action p_4() { meta.a5 = meta.a4 ^ 32w5; }
  action p_5() { meta.a6 = meta.a5 ^ 32w6; }
  action p_6() { meta.a7 = meta.a6 ^ 32w7; }
  action p_7() { meta.a8 = meta.a7 ^ 32w8; }
  action p_8() { meta.a9 = meta.a8 ^ 32w9; }
  action p_9() { meta.a10 = meta.a9 ^ 32w10; }
  action p_10() { meta.a11 = meta.a10 ^ 32w11; }
  action p_11() { meta.a12 = meta.a11 ^ 32w12; }
  action q_0() { meta.a1 = meta.a0 ^ 32w7; }
  action q_1() { meta.a2 = meta.a1 ^ 32w8; }
  action q_2() { meta.a3 = meta.a2 ^ 32w9; }
  action q_3() { meta.a4 = meta.a3 ^ 32w10; }
  action q_4() { meta.a5 = meta.a4 ^ 32w11; }
  action q_5() { meta.a6 = meta.a5 ^ 32w12; }
  action q_6() { meta.a7 = meta.a6 ^ 32w13; }
  action q_7() { meta.a8 = meta.a7 ^ 32w14; }
  action q_8() { meta.a9 = meta.a8 ^ 32w15; }
  action q_9() { meta.a10 = meta.a9 ^ 32w16; }
  action q_10() { meta.a11 = meta.a10 ^ 32w17; }
  action q_11() { meta.a12 = meta.a11 ^ 32w18; }
  action init_p() { meta.a0 = hdr.ipv4.src; }
  action init_q() { meta.a0 = hdr.ipv4.dst; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = meta.a12; ig_tm_md.ucast_egress_port = 1; }
  apply { if (hdr.tcp.dport == 80) { init_p(); p_0(); p_1(); p_2(); p_3(); p_4(); p_5(); p_6(); p_7(); p_8(); p_9(); p_10(); p_11(); } else if (hdr.tcp.dport == 81) { init_q(); q_0(); q_1(); q_2(); q_3(); q_4(); q_5(); q_6(); q_7(); q_8(); q_9(); q_10(); q_11(); } fin(); }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) { state start { pkt.extract(eg_intr_md); transition accept; } }
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
