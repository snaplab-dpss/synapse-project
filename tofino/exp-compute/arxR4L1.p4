
#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header out_h { bit<32> f0; bit<32> f1; bit<32> f2; bit<32> f3; bit<32> f4; bit<32> f5; bit<32> f6; bit<32> f7; bit<32> f8; bit<32> f9; bit<32> f10; bit<32> f11; bit<32> f12; bit<32> f13; bit<32> f14; bit<32> f15; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; out_h o; }
struct meta_t { bit<32> s0; bit<32> s1; bit<32> s2; bit<32> s3; bit<32> s4; bit<32> s5; bit<32> s6; bit<32> s7; bit<32> s8; bit<32> s9; bit<32> s10; bit<32> s11;  bit<32> v0; bit<32> v1; bit<32> v2; bit<32> v3; bit<32> a0; bit<32> a1; bit<32> a2; bit<32> a3; bit<16> w0; bit<16> w1; bit<8> b0; bit<8> b1; bit<12> t0; bit<12> t1; bit<32> o0; bit<32> o1; bit<32> o2; bit<32> o3; bit<32> o4; bit<32> o5; bit<32> o6; bit<32> o7; bit<32> o8; bit<32> o9; bit<32> o10; bit<32> o11; bit<32> o12; bit<32> o13; bit<32> o14; bit<32> o15; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action chain_init() {
    meta.s0 = hdr.ipv4.src;
    meta.s1 = hdr.ipv4.dst;
    meta.s2 = hdr.eth.dst[31:0];
    meta.s3 = hdr.eth.src[31:0];
  }

  action rnd0_x() {
    meta.s4 = meta.s0 + meta.s1;
    meta.s5 = meta.s2 + meta.s3;
  }

  action rnd0_shl() {
    meta.s0 = meta.s4 << 5;
    meta.s2 = meta.s4 >> 27;
    meta.s6 = meta.s5 << 16;
    meta.s7 = meta.s5 >> 16;
  }

  action rnd0_or() {
    meta.s8 = meta.s0 | meta.s2;
    meta.s9 = meta.s6 | meta.s7;
    meta.s10 = meta.s4 ^ meta.s1;
    meta.s11 = meta.s5 ^ meta.s3;
  }

  action rnd1_x() {
    meta.s0 = meta.s8 + meta.s10;
    meta.s1 = meta.s9 + meta.s11;
  }

  action rnd1_shl() {
    meta.s2 = meta.s0 << 8;
    meta.s3 = meta.s0 >> 24;
    meta.s4 = meta.s1 << 7;
    meta.s5 = meta.s1 >> 25;
  }

  action rnd1_or() {
    meta.s6 = meta.s2 | meta.s3;
    meta.s7 = meta.s4 | meta.s5;
    meta.s8 = meta.s0 ^ meta.s10;
    meta.s9 = meta.s1 ^ meta.s11;
  }

  action rnd2_x() {
    meta.s0 = meta.s6 + meta.s8;
    meta.s1 = meta.s7 + meta.s9;
  }

  action rnd2_shl() {
    meta.s2 = meta.s0 << 13;
    meta.s3 = meta.s0 >> 19;
    meta.s4 = meta.s1 << 12;
    meta.s5 = meta.s1 >> 20;
  }

  action rnd2_or() {
    meta.s6 = meta.s2 | meta.s3;
    meta.s7 = meta.s4 | meta.s5;
    meta.s10 = meta.s0 ^ meta.s8;
    meta.s11 = meta.s1 ^ meta.s9;
  }

  action rnd3_x() {
    meta.s0 = meta.s6 + meta.s10;
    meta.s1 = meta.s7 + meta.s11;
  }

  action rnd3_shl() {
    meta.s2 = meta.s0 << 16;
    meta.s3 = meta.s0 >> 16;
    meta.s4 = meta.s1 << 3;
    meta.s5 = meta.s1 >> 29;
  }

  action rnd3_or() {
    meta.s6 = meta.s2 | meta.s3;
    meta.s7 = meta.s4 | meta.s5;
    meta.s8 = meta.s0 ^ meta.s10;
    meta.s9 = meta.s1 ^ meta.s11;
  }

  action chain_fin() {
    hdr.o.setValid();
    hdr.o.f0 = meta.s6;
    hdr.o.f1 = meta.s8;
    hdr.o.f2 = meta.s7;
    hdr.o.f3 = meta.s9;
  }

  action init() { meta.v0 = hdr.ipv4.src; meta.v1 = hdr.ipv4.dst; meta.v2 = hdr.eth.dst[31:0]; meta.v3 = hdr.eth.src[31:0]; meta.w0 = hdr.ipv4.id; meta.w1 = hdr.ipv4.len; meta.b0 = hdr.ipv4.ttl; meta.b1 = hdr.ipv4.tos; meta.t0 = hdr.ipv4.id[11:0]; meta.t1 = hdr.ipv4.len[11:0]; }
  action fin2() { hdr.o.setValid(); hdr.o.f0 = 7; ig_tm_md.ucast_egress_port = 2; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = 1; ig_tm_md.ucast_egress_port = 1; }
  apply { init(); chain_init(); rnd0_x(); rnd0_shl(); rnd0_or(); rnd1_x(); rnd1_shl(); rnd1_or(); rnd2_x(); rnd2_shl(); rnd2_or(); rnd3_x(); rnd3_shl(); rnd3_or(); chain_fin(); if (meta.t0 <= 12w100) { fin(); } else { fin2(); } }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) { state start { pkt.extract(eg_intr_md); transition accept; } }
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
