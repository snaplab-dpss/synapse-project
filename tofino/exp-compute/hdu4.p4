#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<16> data3; bit<16> win; }
header out_h { bit<32> f0; bit<32> f1; bit<32> f2; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; tcp_h tcp; out_h o; }
struct meta_t { bit<32> d0; bit<32> d1; bit<32> d2; bit<32> d3; bit<32> d4; bit<32> d5; bit<32> d6; bit<32> d7; bit<32> d8; bit<32> d9; bit<32> d10; bit<32> d11; bit<32> d12; bit<32> e0; bit<32> e1; bit<32> e2; bit<32> e3; bit<32> e4; bit<32> e5; bit<32> e6; bit<32> e7; bit<32> e8; bit<32> e9; bit<32> e10; bit<32> e11; bit<32> e12; bit<32> f0; bit<32> f1; bit<32> f2; bit<32> f3; bit<32> f4; bit<32> f5; bit<32> f6; bit<32> f7; bit<32> f8; bit<32> f9; bit<32> f10; bit<32> f11; bit<32> f12; bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<32> a4; bit<32> a5; bit<32> a6; bit<32> a7; bit<32> a8; bit<32> a9; bit<32> a10; bit<32> a11; bit<32> a12; bit<32> b0; bit<32> b1; bit<32> b2; bit<32> b3; bit<32> b4; bit<32> b5; bit<32> b6; bit<32> b7; bit<32> b8; bit<32> b9; bit<32> b10; bit<32> b11; bit<32> b12; bit<32> c0; bit<32> c1; bit<32> c2; bit<32> c3; bit<32> c4; bit<32> c5; bit<32> c6; bit<32> c7; bit<32> c8; bit<32> c9; bit<32> c10; bit<32> c11; bit<32> c12; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action init() { meta.a0 = hdr.ipv4.src; meta.b0 = hdr.ipv4.dst; meta.c0 = hdr.tcp.seq; meta.d0 = hdr.ipv4.src; meta.e0 = hdr.ipv4.dst; meta.f0 = hdr.tcp.seq; }
  action fin2() { hdr.o.setValid(); hdr.o.f0 = meta.d12; hdr.o.f1 = meta.e12; hdr.o.f2 = meta.f12; ig_tm_md.ucast_egress_port = 1; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = meta.a12; hdr.o.f1 = meta.b12; hdr.o.f2 = meta.c12; ig_tm_md.ucast_egress_port = 1; }
// two mutually exclusive copies of the same ops on the same inputs, each into its own fields (a/b/c vs d/e/f)
  action p_a0() { @in_hash { meta.a1 = meta.a0[26:0] ++ meta.a0[31:27]; } }
  action p_b0() { @in_hash { meta.b1 = meta.b0[26:0] ++ meta.b0[31:27]; } }
  action p_c0() { @in_hash { meta.c1 = meta.c0[26:0] ++ meta.c0[31:27]; } }
  action p_a1() { @in_hash { meta.a2 = meta.a1[18:0] ++ meta.a1[31:19]; } }
  action p_b1() { @in_hash { meta.b2 = meta.b1[18:0] ++ meta.b1[31:19]; } }
  action p_c1() { @in_hash { meta.c2 = meta.c1[18:0] ++ meta.c1[31:19]; } }
  action p_a2() { @in_hash { meta.a3 = meta.a2[26:0] ++ meta.a2[31:27]; } }
  action p_b2() { @in_hash { meta.b3 = meta.b2[26:0] ++ meta.b2[31:27]; } }
  action p_c2() { @in_hash { meta.c3 = meta.c2[26:0] ++ meta.c2[31:27]; } }
  action p_a3() { @in_hash { meta.a4 = meta.a3[18:0] ++ meta.a3[31:19]; } }
  action p_b3() { @in_hash { meta.b4 = meta.b3[18:0] ++ meta.b3[31:19]; } }
  action p_c3() { @in_hash { meta.c4 = meta.c3[18:0] ++ meta.c3[31:19]; } }
  action p_a4() { @in_hash { meta.a5 = meta.a4[26:0] ++ meta.a4[31:27]; } }
  action p_b4() { @in_hash { meta.b5 = meta.b4[26:0] ++ meta.b4[31:27]; } }
  action p_c4() { @in_hash { meta.c5 = meta.c4[26:0] ++ meta.c4[31:27]; } }
  action p_a5() { @in_hash { meta.a6 = meta.a5[18:0] ++ meta.a5[31:19]; } }
  action p_b5() { @in_hash { meta.b6 = meta.b5[18:0] ++ meta.b5[31:19]; } }
  action p_c5() { @in_hash { meta.c6 = meta.c5[18:0] ++ meta.c5[31:19]; } }
  action p_a6() { @in_hash { meta.a7 = meta.a6[26:0] ++ meta.a6[31:27]; } }
  action p_b6() { @in_hash { meta.b7 = meta.b6[26:0] ++ meta.b6[31:27]; } }
  action p_c6() { @in_hash { meta.c7 = meta.c6[26:0] ++ meta.c6[31:27]; } }
  action p_a7() { @in_hash { meta.a8 = meta.a7[18:0] ++ meta.a7[31:19]; } }
  action p_b7() { @in_hash { meta.b8 = meta.b7[18:0] ++ meta.b7[31:19]; } }
  action p_c7() { @in_hash { meta.c8 = meta.c7[18:0] ++ meta.c7[31:19]; } }
  action p_a8() { @in_hash { meta.a9 = meta.a8[26:0] ++ meta.a8[31:27]; } }
  action p_b8() { @in_hash { meta.b9 = meta.b8[26:0] ++ meta.b8[31:27]; } }
  action p_c8() { @in_hash { meta.c9 = meta.c8[26:0] ++ meta.c8[31:27]; } }
  action p_a9() { @in_hash { meta.a10 = meta.a9[18:0] ++ meta.a9[31:19]; } }
  action p_b9() { @in_hash { meta.b10 = meta.b9[18:0] ++ meta.b9[31:19]; } }
  action p_c9() { @in_hash { meta.c10 = meta.c9[18:0] ++ meta.c9[31:19]; } }
  action p_a10() { @in_hash { meta.a11 = meta.a10[26:0] ++ meta.a10[31:27]; } }
  action p_b10() { @in_hash { meta.b11 = meta.b10[26:0] ++ meta.b10[31:27]; } }
  action p_c10() { @in_hash { meta.c11 = meta.c10[26:0] ++ meta.c10[31:27]; } }
  action p_a11() { @in_hash { meta.a12 = meta.a11[18:0] ++ meta.a11[31:19]; } }
  action p_b11() { @in_hash { meta.b12 = meta.b11[18:0] ++ meta.b11[31:19]; } }
  action p_c11() { @in_hash { meta.c12 = meta.c11[18:0] ++ meta.c11[31:19]; } }
  action q_a0() { @in_hash { meta.d1 = meta.d0[26:0] ++ meta.d0[31:27]; } }
  action q_b0() { @in_hash { meta.e1 = meta.e0[26:0] ++ meta.e0[31:27]; } }
  action q_c0() { @in_hash { meta.f1 = meta.f0[26:0] ++ meta.f0[31:27]; } }
  action q_a1() { @in_hash { meta.d2 = meta.d1[18:0] ++ meta.d1[31:19]; } }
  action q_b1() { @in_hash { meta.e2 = meta.e1[18:0] ++ meta.e1[31:19]; } }
  action q_c1() { @in_hash { meta.f2 = meta.f1[18:0] ++ meta.f1[31:19]; } }
  action q_a2() { @in_hash { meta.d3 = meta.d2[26:0] ++ meta.d2[31:27]; } }
  action q_b2() { @in_hash { meta.e3 = meta.e2[26:0] ++ meta.e2[31:27]; } }
  action q_c2() { @in_hash { meta.f3 = meta.f2[26:0] ++ meta.f2[31:27]; } }
  action q_a3() { @in_hash { meta.d4 = meta.d3[18:0] ++ meta.d3[31:19]; } }
  action q_b3() { @in_hash { meta.e4 = meta.e3[18:0] ++ meta.e3[31:19]; } }
  action q_c3() { @in_hash { meta.f4 = meta.f3[18:0] ++ meta.f3[31:19]; } }
  action q_a4() { @in_hash { meta.d5 = meta.d4[26:0] ++ meta.d4[31:27]; } }
  action q_b4() { @in_hash { meta.e5 = meta.e4[26:0] ++ meta.e4[31:27]; } }
  action q_c4() { @in_hash { meta.f5 = meta.f4[26:0] ++ meta.f4[31:27]; } }
  action q_a5() { @in_hash { meta.d6 = meta.d5[18:0] ++ meta.d5[31:19]; } }
  action q_b5() { @in_hash { meta.e6 = meta.e5[18:0] ++ meta.e5[31:19]; } }
  action q_c5() { @in_hash { meta.f6 = meta.f5[18:0] ++ meta.f5[31:19]; } }
  action q_a6() { @in_hash { meta.d7 = meta.d6[26:0] ++ meta.d6[31:27]; } }
  action q_b6() { @in_hash { meta.e7 = meta.e6[26:0] ++ meta.e6[31:27]; } }
  action q_c6() { @in_hash { meta.f7 = meta.f6[26:0] ++ meta.f6[31:27]; } }
  action q_a7() { @in_hash { meta.d8 = meta.d7[18:0] ++ meta.d7[31:19]; } }
  action q_b7() { @in_hash { meta.e8 = meta.e7[18:0] ++ meta.e7[31:19]; } }
  action q_c7() { @in_hash { meta.f8 = meta.f7[18:0] ++ meta.f7[31:19]; } }
  action q_a8() { @in_hash { meta.d9 = meta.d8[26:0] ++ meta.d8[31:27]; } }
  action q_b8() { @in_hash { meta.e9 = meta.e8[26:0] ++ meta.e8[31:27]; } }
  action q_c8() { @in_hash { meta.f9 = meta.f8[26:0] ++ meta.f8[31:27]; } }
  action q_a9() { @in_hash { meta.d10 = meta.d9[18:0] ++ meta.d9[31:19]; } }
  action q_b9() { @in_hash { meta.e10 = meta.e9[18:0] ++ meta.e9[31:19]; } }
  action q_c9() { @in_hash { meta.f10 = meta.f9[18:0] ++ meta.f9[31:19]; } }
  action q_a10() { @in_hash { meta.d11 = meta.d10[26:0] ++ meta.d10[31:27]; } }
  action q_b10() { @in_hash { meta.e11 = meta.e10[26:0] ++ meta.e10[31:27]; } }
  action q_c10() { @in_hash { meta.f11 = meta.f10[26:0] ++ meta.f10[31:27]; } }
  action q_a11() { @in_hash { meta.d12 = meta.d11[18:0] ++ meta.d11[31:19]; } }
  action q_b11() { @in_hash { meta.e12 = meta.e11[18:0] ++ meta.e11[31:19]; } }
  action q_c11() { @in_hash { meta.f12 = meta.f11[18:0] ++ meta.f11[31:19]; } }
  apply { init(); if (hdr.tcp.dport == 80) { p_a0(); p_b0(); p_c0(); p_a1(); p_b1(); p_c1(); p_a2(); p_b2(); p_c2(); p_a3(); p_b3(); p_c3(); p_a4(); p_b4(); p_c4(); p_a5(); p_b5(); p_c5(); p_a6(); p_b6(); p_c6(); p_a7(); p_b7(); p_c7(); p_a8(); p_b8(); p_c8(); p_a9(); p_b9(); p_c9(); p_a10(); p_b10(); p_c10(); p_a11(); p_b11(); p_c11(); fin(); } else { q_a0(); q_b0(); q_c0(); q_a1(); q_b1(); q_c1(); q_a2(); q_b2(); q_c2(); q_a3(); q_b3(); q_c3(); q_a4(); q_b4(); q_c4(); q_a5(); q_b5(); q_c5(); q_a6(); q_b6(); q_c6(); q_a7(); q_b7(); q_c7(); q_a8(); q_b8(); q_c8(); q_a9(); q_b9(); q_c9(); q_a10(); q_b10(); q_c10(); q_a11(); q_b11(); q_c11(); fin2(); } }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) { state start { pkt.extract(eg_intr_md); transition accept; } }
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
