
#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<16> data3; bit<16> win; }
header out_h { bit<32> f0; bit<32> f1; bit<32> f2; bit<32> f3; bit<32> f4; bit<32> f5; bit<32> f6; bit<32> f7; bit<32> f8; bit<32> f9; bit<32> f10; bit<32> f11; bit<32> f12; bit<32> f13; bit<32> f14; bit<32> f15; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; tcp_h tcp; out_h o; }
struct meta_t { bit<32> v0; bit<32> v1; bit<32> v2; bit<32> v3; bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<16> w0; bit<16> w1; bit<8> b0; bit<8> b1; bit<12> t0; bit<12> t1; bit<32> o0; bit<32> o1; bit<32> o2; bit<32> o3; bit<32> o4; bit<32> o5; bit<32> o6; bit<32> o7; bit<32> o8; bit<32> o9; bit<32> o10; bit<32> o11; bit<32> o12; bit<32> o13; bit<32> o14; bit<32> o15; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action init() { meta.v0 = hdr.ipv4.src; meta.v1 = hdr.ipv4.dst; }
  action rot() { @in_hash { meta.v2 = meta.v0[18:0] ++ meta.v0[31:19]; } }
  action x() { meta.v3 = meta.v2 ^ meta.v1; meta.a0 = meta.v2 + meta.v1; }
  action rot2() { @in_hash { meta.a1 = meta.v3[24:0] ++ meta.v3[31:25]; } }
  action x2() { meta.a2 = meta.a1 ^ meta.a0; meta.a3 = meta.a0 >> 8; }
  action val() { meta.o0 = meta.a3 ^ meta.a2; }
  action wr() { hdr.tcp.seq = meta.o0; }
  action fin2() { hdr.o.setValid(); hdr.o.f0 = 7; ig_tm_md.ucast_egress_port = 2; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = 1; ig_tm_md.ucast_egress_port = 1; }
  apply { init(); rot(); x(); rot2(); x2(); val(); wr(); if (meta.o0 == 7) { fin(); } else { fin2(); } }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) { state start { pkt.extract(eg_intr_md); transition accept; } }
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
