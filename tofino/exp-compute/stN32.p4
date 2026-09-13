// stD4 with every write to a fresh slot, round-robin over 32 header slots: does the count of sliced fields alone break PHV allocation?
#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<16> data3; bit<16> win; }
header out_h { bit<32> f0; bit<32> f1; }
header state_h { bit<32> s0; bit<32> s1; bit<32> s2; bit<32> s3; bit<32> s4; bit<32> s5; bit<32> s6; bit<32> s7; bit<32> s8; bit<32> s9; bit<32> s10; bit<32> s11; bit<32> s12; bit<32> s13; bit<32> s14; bit<32> s15; bit<32> s16; bit<32> s17; bit<32> s18; bit<32> s19; bit<32> s20; bit<32> s21; bit<32> s22; bit<32> s23; bit<32> s24; bit<32> s25; bit<32> s26; bit<32> s27; bit<32> s28; bit<32> s29; bit<32> s30; bit<32> s31; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; tcp_h tcp; state_h st; out_h o; }
struct meta_t { bit<32> o0; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) { apply { pkt.emit(hdr); } }
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action init() { hdr.st.setValid(); hdr.st.s0 = hdr.ipv4.src; hdr.st.s1 = hdr.ipv4.dst; hdr.st.s2 = hdr.tcp.seq; hdr.st.s3 = hdr.ipv4.src ^ hdr.ipv4.dst; }
  action r0_1() { hdr.st.s4 = hdr.st.s0 + hdr.st.s1; hdr.st.s5 = hdr.st.s2 + hdr.st.s3; @in_hash { hdr.st.s6 = hdr.st.s1[26:0] ++ hdr.st.s1[31:27]; } }
  action r0_1b() { hdr.st.s7 = hdr.st.s3[23:0] ++ hdr.st.s3[31:24]; }
  action r0_2() { hdr.st.s8 = hdr.st.s6 ^ hdr.st.s4; hdr.st.s9 = hdr.st.s7 ^ hdr.st.s5; hdr.st.s10 = hdr.st.s4[15:0] ++ hdr.st.s4[31:16]; hdr.st.s11 = hdr.st.s5; }
  action r0_3() { hdr.st.s12 = hdr.st.s11 + hdr.st.s8; hdr.st.s13 = hdr.st.s10 + hdr.st.s9; @in_hash { hdr.st.s14 = hdr.st.s8[18:0] ++ hdr.st.s8[31:19]; } }
  action r0_3b() { @in_hash { hdr.st.s15 = hdr.st.s9[24:0] ++ hdr.st.s9[31:25]; } }
  action r0_4() { hdr.st.s16 = hdr.st.s14 ^ hdr.st.s12; hdr.st.s17 = hdr.st.s15 ^ hdr.st.s13; hdr.st.s18 = hdr.st.s12[15:0] ++ hdr.st.s12[31:16]; }
  action r0_4b() { hdr.st.s19 = hdr.st.s13; }
  action r1_1() { hdr.st.s20 = hdr.st.s19 + hdr.st.s16; hdr.st.s21 = hdr.st.s18 + hdr.st.s17; @in_hash { hdr.st.s22 = hdr.st.s16[26:0] ++ hdr.st.s16[31:27]; } }
  action r1_1b() { hdr.st.s23 = hdr.st.s17[23:0] ++ hdr.st.s17[31:24]; }
  action r1_2() { hdr.st.s24 = hdr.st.s22 ^ hdr.st.s20; hdr.st.s25 = hdr.st.s23 ^ hdr.st.s21; hdr.st.s26 = hdr.st.s20[15:0] ++ hdr.st.s20[31:16]; hdr.st.s27 = hdr.st.s21; }
  action r1_3() { hdr.st.s28 = hdr.st.s27 + hdr.st.s24; hdr.st.s29 = hdr.st.s26 + hdr.st.s25; @in_hash { hdr.st.s30 = hdr.st.s24[18:0] ++ hdr.st.s24[31:19]; } }
  action r1_3b() { @in_hash { hdr.st.s31 = hdr.st.s25[24:0] ++ hdr.st.s25[31:25]; } }
  action r1_4() { hdr.st.s1 = hdr.st.s30 ^ hdr.st.s28; hdr.st.s3 = hdr.st.s31 ^ hdr.st.s29; hdr.st.s0 = hdr.st.s28[15:0] ++ hdr.st.s28[31:16]; }
  action r1_4b() { hdr.st.s2 = hdr.st.s29; }
  action r2_1() { hdr.st.s5 = hdr.st.s2 + hdr.st.s1; hdr.st.s4 = hdr.st.s0 + hdr.st.s3; @in_hash { hdr.st.s6 = hdr.st.s1[26:0] ++ hdr.st.s1[31:27]; } }
  action r2_1b() { hdr.st.s7 = hdr.st.s3[23:0] ++ hdr.st.s3[31:24]; }
  action r2_2() { hdr.st.s8 = hdr.st.s6 ^ hdr.st.s5; hdr.st.s9 = hdr.st.s7 ^ hdr.st.s4; hdr.st.s11 = hdr.st.s5[15:0] ++ hdr.st.s5[31:16]; hdr.st.s10 = hdr.st.s4; }
  action r2_3() { hdr.st.s13 = hdr.st.s10 + hdr.st.s8; hdr.st.s12 = hdr.st.s11 + hdr.st.s9; @in_hash { hdr.st.s14 = hdr.st.s8[18:0] ++ hdr.st.s8[31:19]; } }
  action r2_3b() { @in_hash { hdr.st.s15 = hdr.st.s9[24:0] ++ hdr.st.s9[31:25]; } }
  action r2_4() { hdr.st.s16 = hdr.st.s14 ^ hdr.st.s13; hdr.st.s17 = hdr.st.s15 ^ hdr.st.s12; hdr.st.s19 = hdr.st.s13[15:0] ++ hdr.st.s13[31:16]; }
  action r2_4b() { hdr.st.s18 = hdr.st.s12; }
  action r3_1() { hdr.st.s21 = hdr.st.s18 + hdr.st.s16; hdr.st.s20 = hdr.st.s19 + hdr.st.s17; @in_hash { hdr.st.s22 = hdr.st.s16[26:0] ++ hdr.st.s16[31:27]; } }
  action r3_1b() { hdr.st.s23 = hdr.st.s17[23:0] ++ hdr.st.s17[31:24]; }
  action r3_2() { hdr.st.s24 = hdr.st.s22 ^ hdr.st.s21; hdr.st.s25 = hdr.st.s23 ^ hdr.st.s20; hdr.st.s27 = hdr.st.s21[15:0] ++ hdr.st.s21[31:16]; hdr.st.s26 = hdr.st.s20; }
  action r3_3() { hdr.st.s29 = hdr.st.s26 + hdr.st.s24; hdr.st.s28 = hdr.st.s27 + hdr.st.s25; @in_hash { hdr.st.s30 = hdr.st.s24[18:0] ++ hdr.st.s24[31:19]; } }
  action r3_3b() { @in_hash { hdr.st.s31 = hdr.st.s25[24:0] ++ hdr.st.s25[31:25]; } }
  action r3_4() { hdr.st.s1 = hdr.st.s30 ^ hdr.st.s29; hdr.st.s3 = hdr.st.s31 ^ hdr.st.s28; hdr.st.s2 = hdr.st.s29[15:0] ++ hdr.st.s29[31:16]; }
  action r3_4b() { hdr.st.s0 = hdr.st.s28; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = hdr.st.s0 ^ hdr.st.s1; hdr.o.f1 = hdr.st.s2 ^ hdr.st.s3; hdr.st.setInvalid(); ig_tm_md.ucast_egress_port = 1; }
  apply { init(); r0_1(); r0_1b(); r0_2(); r0_3(); r0_3b(); r0_4(); r0_4b(); r1_1(); r1_1b(); r1_2(); r1_3(); r1_3b(); r1_4(); r1_4b(); r2_1(); r2_1b(); r2_2(); r2_3(); r2_3b(); r2_4(); r2_4b(); r3_1(); r3_1b(); r3_2(); r3_3(); r3_3b(); r3_4(); r3_4b(); fin(); }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); pkt.extract(hdr.st); transition accept; }
}
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
