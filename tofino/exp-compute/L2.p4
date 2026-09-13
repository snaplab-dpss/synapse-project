// stC4 with the rounds called from both branches of an if
// four rounds in the ground truth's exact discipline: adds and hash rotates write metadata temporaries, xors and byte-aligned (bare) rotates write the header state
#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<16> data3; bit<16> win; }
header out_h { bit<32> f0; bit<32> f1; }
header state_h { bit<32> v0; bit<32> v1; bit<32> v2; bit<32> v3; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; tcp_h tcp; state_h st; out_h o; }
struct meta_t { bit<32> o0; bit<32> t0; bit<32> t1; bit<32> t2; bit<32> t3; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) { apply { pkt.emit(hdr); } }
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action init() { hdr.st.setValid(); hdr.st.v0 = hdr.ipv4.src; hdr.st.v1 = hdr.ipv4.dst; hdr.st.v2 = hdr.tcp.seq; hdr.st.v3 = hdr.ipv4.src ^ hdr.ipv4.dst; }
  action r0_1() { meta.t0 = hdr.st.v0 + hdr.st.v1; meta.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r0_1b() { meta.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r0_2() { hdr.st.v1 = meta.t1 ^ meta.t0; hdr.st.v3 = meta.t3 ^ meta.t2; hdr.st.v0 = meta.t0[15:0] ++ meta.t0[31:16]; hdr.st.v2 = meta.t2; }
  action r0_3() { meta.t2 = hdr.st.v2 + hdr.st.v1; meta.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r0_3b() { @in_hash { meta.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r0_4() { hdr.st.v1 = meta.t1 ^ meta.t2; hdr.st.v3 = meta.t3 ^ meta.t0; hdr.st.v2 = meta.t2[15:0] ++ meta.t2[31:16]; }
  action r0_4b() { hdr.st.v0 = meta.t0; }
  action r1_1() { meta.t0 = hdr.st.v0 + hdr.st.v1; meta.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r1_1b() { meta.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r1_2() { hdr.st.v1 = meta.t1 ^ meta.t0; hdr.st.v3 = meta.t3 ^ meta.t2; hdr.st.v0 = meta.t0[15:0] ++ meta.t0[31:16]; hdr.st.v2 = meta.t2; }
  action r1_3() { meta.t2 = hdr.st.v2 + hdr.st.v1; meta.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r1_3b() { @in_hash { meta.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r1_4() { hdr.st.v1 = meta.t1 ^ meta.t2; hdr.st.v3 = meta.t3 ^ meta.t0; hdr.st.v2 = meta.t2[15:0] ++ meta.t2[31:16]; }
  action r1_4b() { hdr.st.v0 = meta.t0; }
  action r2_1() { meta.t0 = hdr.st.v0 + hdr.st.v1; meta.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r2_1b() { meta.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r2_2() { hdr.st.v1 = meta.t1 ^ meta.t0; hdr.st.v3 = meta.t3 ^ meta.t2; hdr.st.v0 = meta.t0[15:0] ++ meta.t0[31:16]; hdr.st.v2 = meta.t2; }
  action r2_3() { meta.t2 = hdr.st.v2 + hdr.st.v1; meta.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r2_3b() { @in_hash { meta.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r2_4() { hdr.st.v1 = meta.t1 ^ meta.t2; hdr.st.v3 = meta.t3 ^ meta.t0; hdr.st.v2 = meta.t2[15:0] ++ meta.t2[31:16]; }
  action r2_4b() { hdr.st.v0 = meta.t0; }
  action r3_1() { meta.t0 = hdr.st.v0 + hdr.st.v1; meta.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r3_1b() { meta.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r3_2() { hdr.st.v1 = meta.t1 ^ meta.t0; hdr.st.v3 = meta.t3 ^ meta.t2; hdr.st.v0 = meta.t0[15:0] ++ meta.t0[31:16]; hdr.st.v2 = meta.t2; }
  action r3_3() { meta.t2 = hdr.st.v2 + hdr.st.v1; meta.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { meta.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r3_3b() { @in_hash { meta.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r3_4() { hdr.st.v1 = meta.t1 ^ meta.t2; hdr.st.v3 = meta.t3 ^ meta.t0; hdr.st.v2 = meta.t2[15:0] ++ meta.t2[31:16]; }
  action r3_4b() { hdr.st.v0 = meta.t0; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = hdr.st.v0 ^ hdr.st.v1; hdr.o.f1 = hdr.st.v2 ^ hdr.st.v3; hdr.st.setInvalid(); ig_tm_md.ucast_egress_port = 1; }
  apply { init(); if (hdr.tcp.dport == 80) { r0_1(); r0_1b(); r0_2(); r0_3(); r0_3b(); r0_4(); r0_4b(); r1_1(); r1_1b(); r1_2(); r1_3(); r1_3b(); r1_4(); r1_4b(); r2_1(); r2_1b(); r2_2(); r2_3(); r2_3b(); r2_4(); r2_4b(); r3_1(); r3_1b(); r3_2(); r3_3(); r3_3b(); r3_4(); r3_4b(); } else { r0_1(); r0_1b(); r0_2(); r0_3(); r0_3b(); r0_4(); r0_4b(); r1_1(); r1_1b(); r1_2(); r1_3(); r1_3b(); r1_4(); r1_4b(); r2_1(); r2_1b(); r2_2(); r2_3(); r2_3b(); r2_4(); r2_4b(); r3_1(); r3_1b(); r3_2(); r3_3(); r3_3b(); r3_4(); r3_4b(); } fin(); }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); pkt.extract(hdr.st); transition accept; }
}
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
