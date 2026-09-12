// the same parameterized actions called with the same fields from both branches
#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<16> data3; bit<16> win; }
header out_h { bit<32> f0; bit<32> f1; bit<32> f2; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; tcp_h tcp; out_h o; }
struct meta_t { bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<32> a4; bit<32> a5; bit<32> a6; bit<32> a7; bit<32> a8; bit<32> a9; bit<32> a10; bit<32> a11; bit<32> a12; bit<32> b0; bit<32> b1; bit<32> b2; bit<32> b3; bit<32> b4; bit<32> b5; bit<32> b6; bit<32> b7; bit<32> b8; bit<32> b9; bit<32> b10; bit<32> b11; bit<32> b12; bit<32> c0; bit<32> c1; bit<32> c2; bit<32> c3; bit<32> c4; bit<32> c5; bit<32> c6; bit<32> c7; bit<32> c8; bit<32> c9; bit<32> c10; bit<32> c11; bit<32> c12; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action init() { meta.a0 = hdr.ipv4.src; meta.b0 = hdr.ipv4.dst; meta.c0 = hdr.tcp.seq; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = meta.a12; hdr.o.f1 = meta.b12; hdr.o.f2 = meta.c12; ig_tm_md.ucast_egress_port = 1; }
  action r5(in bit<32> x, out bit<32> y) { @in_hash { y = x[26:0] ++ x[31:27]; } }
  action r13(in bit<32> x, out bit<32> y) { @in_hash { y = x[18:0] ++ x[31:19]; } }
  apply { init(); if (hdr.tcp.dport == 80) { r5(meta.a0, meta.a1); r5(meta.b0, meta.b1); r5(meta.c0, meta.c1); r13(meta.a1, meta.a2); r13(meta.b1, meta.b2); r13(meta.c1, meta.c2); r5(meta.a2, meta.a3); r5(meta.b2, meta.b3); r5(meta.c2, meta.c3); r13(meta.a3, meta.a4); r13(meta.b3, meta.b4); r13(meta.c3, meta.c4); r5(meta.a4, meta.a5); r5(meta.b4, meta.b5); r5(meta.c4, meta.c5); r13(meta.a5, meta.a6); r13(meta.b5, meta.b6); r13(meta.c5, meta.c6); r5(meta.a6, meta.a7); r5(meta.b6, meta.b7); r5(meta.c6, meta.c7); r13(meta.a7, meta.a8); r13(meta.b7, meta.b8); r13(meta.c7, meta.c8); r5(meta.a8, meta.a9); r5(meta.b8, meta.b9); r5(meta.c8, meta.c9); r13(meta.a9, meta.a10); r13(meta.b9, meta.b10); r13(meta.c9, meta.c10); r5(meta.a10, meta.a11); r5(meta.b10, meta.b11); r5(meta.c10, meta.c11); r13(meta.a11, meta.a12); r13(meta.b11, meta.b12); r13(meta.c11, meta.c12); } else { r5(meta.a0, meta.a1); r5(meta.b0, meta.b1); r5(meta.c0, meta.c1); r13(meta.a1, meta.a2); r13(meta.b1, meta.b2); r13(meta.c1, meta.c2); r5(meta.a2, meta.a3); r5(meta.b2, meta.b3); r5(meta.c2, meta.c3); r13(meta.a3, meta.a4); r13(meta.b3, meta.b4); r13(meta.c3, meta.c4); r5(meta.a4, meta.a5); r5(meta.b4, meta.b5); r5(meta.c4, meta.c5); r13(meta.a5, meta.a6); r13(meta.b5, meta.b6); r13(meta.c5, meta.c6); r5(meta.a6, meta.a7); r5(meta.b6, meta.b7); r5(meta.c6, meta.c7); r13(meta.a7, meta.a8); r13(meta.b7, meta.b8); r13(meta.c7, meta.c8); r5(meta.a8, meta.a9); r5(meta.b8, meta.b9); r5(meta.c8, meta.c9); r13(meta.a9, meta.a10); r13(meta.b9, meta.b10); r13(meta.c9, meta.c10); r5(meta.a10, meta.a11); r5(meta.b10, meta.b11); r5(meta.c10, meta.c11); r13(meta.a11, meta.a12); r13(meta.b11, meta.b12); r13(meta.c11, meta.c12); } fin(); }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) { state start { pkt.extract(eg_intr_md); transition accept; } }
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
