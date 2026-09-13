// stD4 with a second set of round actions on the other branch of an if: same slots, same stages, but the hash unit writes the slot the other branch's ALU writes in that stage (t0<->t1, t2<->t3 as destinations)
#include <core.p4>
#include <t2na.p4>
header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }
header tcp_h { bit<16> sport; bit<16> dport; bit<32> seq; bit<16> data3; bit<16> win; }
header out_h { bit<32> f0; bit<32> f1; }
header state_h { bit<32> v0; bit<32> v1; bit<32> v2; bit<32> v3; bit<32> t0; bit<32> t1; bit<32> t2; bit<32> t3; }
struct headers_t { ethernet_h eth; ipv4_h ipv4; tcp_h tcp; state_h st; out_h o; }
struct meta_t { bit<32> o0; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) { apply { pkt.emit(hdr); } }
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action init() { hdr.st.setValid(); hdr.st.v0 = hdr.ipv4.src; hdr.st.v1 = hdr.ipv4.dst; hdr.st.v2 = hdr.tcp.seq; hdr.st.v3 = hdr.ipv4.src ^ hdr.ipv4.dst; }
  action r0_1() { hdr.st.t0 = hdr.st.v0 + hdr.st.v1; hdr.st.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r0_1b() { hdr.st.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r0_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action r0_3() { hdr.st.t2 = hdr.st.v2 + hdr.st.v1; hdr.st.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r0_3b() { @in_hash { hdr.st.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r0_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action r0_4b() { hdr.st.v0 = hdr.st.t0; }
  action r1_1() { hdr.st.t0 = hdr.st.v0 + hdr.st.v1; hdr.st.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r1_1b() { hdr.st.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r1_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action r1_3() { hdr.st.t2 = hdr.st.v2 + hdr.st.v1; hdr.st.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r1_3b() { @in_hash { hdr.st.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r1_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action r1_4b() { hdr.st.v0 = hdr.st.t0; }
  action r2_1() { hdr.st.t0 = hdr.st.v0 + hdr.st.v1; hdr.st.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r2_1b() { hdr.st.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r2_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action r2_3() { hdr.st.t2 = hdr.st.v2 + hdr.st.v1; hdr.st.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r2_3b() { @in_hash { hdr.st.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r2_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action r2_4b() { hdr.st.v0 = hdr.st.t0; }
  action r3_1() { hdr.st.t0 = hdr.st.v0 + hdr.st.v1; hdr.st.t2 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action r3_1b() { hdr.st.t3 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action r3_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action r3_3() { hdr.st.t2 = hdr.st.v2 + hdr.st.v1; hdr.st.t0 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t1 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action r3_3b() { @in_hash { hdr.st.t3 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action r3_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action r3_4b() { hdr.st.v0 = hdr.st.t0; }
  action q0_1() { hdr.st.t1 = hdr.st.v0 + hdr.st.v1; hdr.st.t3 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action q0_1b() { hdr.st.t2 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action q0_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action q0_3() { hdr.st.t3 = hdr.st.v2 + hdr.st.v1; hdr.st.t1 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action q0_3b() { @in_hash { hdr.st.t2 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action q0_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action q0_4b() { hdr.st.v0 = hdr.st.t0; }
  action q1_1() { hdr.st.t1 = hdr.st.v0 + hdr.st.v1; hdr.st.t3 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action q1_1b() { hdr.st.t2 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action q1_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action q1_3() { hdr.st.t3 = hdr.st.v2 + hdr.st.v1; hdr.st.t1 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action q1_3b() { @in_hash { hdr.st.t2 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action q1_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action q1_4b() { hdr.st.v0 = hdr.st.t0; }
  action q2_1() { hdr.st.t1 = hdr.st.v0 + hdr.st.v1; hdr.st.t3 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action q2_1b() { hdr.st.t2 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action q2_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action q2_3() { hdr.st.t3 = hdr.st.v2 + hdr.st.v1; hdr.st.t1 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action q2_3b() { @in_hash { hdr.st.t2 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action q2_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action q2_4b() { hdr.st.v0 = hdr.st.t0; }
  action q3_1() { hdr.st.t1 = hdr.st.v0 + hdr.st.v1; hdr.st.t3 = hdr.st.v2 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[26:0] ++ hdr.st.v1[31:27]; } }
  action q3_1b() { hdr.st.t2 = hdr.st.v3[23:0] ++ hdr.st.v3[31:24]; }
  action q3_2() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t0; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t2; hdr.st.v0 = hdr.st.t0[15:0] ++ hdr.st.t0[31:16]; hdr.st.v2 = hdr.st.t2; }
  action q3_3() { hdr.st.t3 = hdr.st.v2 + hdr.st.v1; hdr.st.t1 = hdr.st.v0 + hdr.st.v3; @in_hash { hdr.st.t0 = hdr.st.v1[18:0] ++ hdr.st.v1[31:19]; } }
  action q3_3b() { @in_hash { hdr.st.t2 = hdr.st.v3[24:0] ++ hdr.st.v3[31:25]; } }
  action q3_4() { hdr.st.v1 = hdr.st.t1 ^ hdr.st.t2; hdr.st.v3 = hdr.st.t3 ^ hdr.st.t0; hdr.st.v2 = hdr.st.t2[15:0] ++ hdr.st.t2[31:16]; }
  action q3_4b() { hdr.st.v0 = hdr.st.t0; }
  action fin() { hdr.o.setValid(); hdr.o.f0 = hdr.st.v0 ^ hdr.st.v1; hdr.o.f1 = hdr.st.v2 ^ hdr.st.v3; hdr.st.setInvalid(); ig_tm_md.ucast_egress_port = 1; }
  apply { if (hdr.tcp.sport == 80) { init(); r0_1(); r0_1b(); r0_2(); r0_3(); r0_3b(); r0_4(); r0_4b(); r1_1(); r1_1b(); r1_2(); r1_3(); r1_3b(); r1_4(); r1_4b(); r2_1(); r2_1b(); r2_2(); r2_3(); r2_3b(); r2_4(); r2_4b(); r3_1(); r3_1b(); r3_2(); r3_3(); r3_3b(); r3_4(); r3_4b(); } else { init(); q0_1(); q0_1b(); q0_2(); q0_3(); q0_3b(); q0_4(); q0_4b(); q1_1(); q1_1b(); q1_2(); q1_3(); q1_3b(); q1_4(); q1_4b(); q2_1(); q2_1b(); q2_2(); q2_3(); q2_3b(); q2_4(); q2_4b(); q3_1(); q3_1b(); q3_2(); q3_3(); q3_3b(); q3_4(); q3_4b(); } fin(); }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); pkt.extract(hdr.tcp); pkt.extract(hdr.st); transition accept; }
}
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
