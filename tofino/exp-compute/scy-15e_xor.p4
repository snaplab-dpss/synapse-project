// scy-15c_add with the add turned into an xor of the same operands
#include <core.p4>
#include <t2na.p4>
header hdr0_h {
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<16> data3;
}
header hdr1_h {
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<32> data3;
  bit<32> data4;
}
header hdr2_h {
  bit<32> data0;
  bit<32> data1;
  bit<32> data2;
  bit<16> data3;
  bit<16> data4;
  bit<32> data5;
}
header recirc_h { bit<32> f32_0; bit<32> f32_1; bit<32> f32_2; }
header state_h {
  bit<32> s32_0;
  bit<32> s32_1;
  bit<32> s32_2;
  bit<32> s32_3;
  bit<32> s32_4;
  bit<32> s32_5;
  bit<32> s32_6;
  bit<32> s32_7;
  bit<32> s32_8;
  bit<32> s32_9;
  bit<32> s32_10;
}
struct headers_t { hdr0_h hdr0; hdr1_h hdr1; hdr2_h hdr2; recirc_h recirc; state_h st; }
struct meta_t { bit<32> cond_operand_90_0_out; bit<32> op_xor_345_out; }
parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.hdr0); pkt.extract(hdr.hdr1); pkt.extract(hdr.hdr2); pkt.extract(hdr.recirc); transition accept; }
}
control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) { apply { pkt.emit(hdr); } }
control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action init() { hdr.st.setValid(); }
  action compute_op_add_281() {
    hdr.st.s32_1 = hdr.st.s32_8 + hdr.st.s32_6;
    hdr.st.s32_3 = hdr.st.s32_2 ^ hdr.st.s32_8;
  }
  action compute_op_add_285_b() {
    @in_hash { hdr.st.s32_8 = hdr.hdr2.data1; }
  }
  action compute_op_add_285() {
    hdr.st.s32_1 = 32w0xffffffff + hdr.st.s32_8;
  }
  action compute_rotate_left_53_x() {
    hdr.st.s32_6 = (hdr.st.s32_0) ^ (hdr.st.s32_1);
  }
  action compute_op_xor_345() {
    @in_hash { meta.op_xor_345_out = hdr.recirc.f32_1 ^ hdr.recirc.f32_2; }
  }
  action compute_cond_operand_90_0() {
    meta.cond_operand_90_0_out = (hdr.recirc.f32_0) - (meta.op_xor_345_out);
  }
  action compute_rotate_left_107() {
    hdr.st.s32_0 = 32w2225785509;
  }
  action compute_rotate_left_108_x() {
    @in_hash { hdr.st.s32_1 = (32w0x3b355c4b) ^ (hdr.hdr1.data3); }
    hdr.st.s32_2 = 32w3399118710;
  }
  action compute_rotate_left_108() {
    hdr.st.s32_3 = hdr.st.s32_1[23:0] ++ hdr.st.s32_1[31:24];
    hdr.st.s32_4 = (32w0x6f76ca9a) ^ (hdr.st.s32_0);
    hdr.st.s32_5 = 32w0x5d476351 + hdr.st.s32_1;
  }
  action compute_rotate_left_110() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
    hdr.st.s32_6 = (hdr.st.s32_3) ^ (hdr.st.s32_5);
    hdr.st.s32_7 = hdr.st.s32_1 + hdr.st.s32_4;
  }
  action compute_rotate_left_111() {
    @in_hash { hdr.st.s32_1 = hdr.st.s32_6[24:0] ++ hdr.st.s32_6[31:25]; }
    hdr.st.s32_3 = (32w0x5d476351) + (hdr.st.s32_7);
    hdr.st.s32_4 = hdr.st.s32_2 + hdr.st.s32_6;
  }
  action compute_rotate_left_112() {
    hdr.st.s32_2 = hdr.st.s32_3[15:0] ++ hdr.st.s32_3[31:16];
    hdr.st.s32_5 = (hdr.st.s32_0) ^ (hdr.st.s32_3);
    hdr.st.s32_6 = (hdr.st.s32_1) ^ (hdr.st.s32_4);
  }
  action compute_rotate_left_113() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_5[26:0] ++ hdr.st.s32_5[31:27]; }
    hdr.st.s32_1 = hdr.st.s32_6[23:0] ++ hdr.st.s32_6[31:24];
    hdr.st.s32_3 = (hdr.st.s32_4) + (hdr.st.s32_5);
    hdr.st.s32_7 = hdr.st.s32_2 + hdr.st.s32_6;
  }
  action compute_rotate_left_115() {
    hdr.st.s32_2 = hdr.st.s32_3[15:0] ++ hdr.st.s32_3[31:16];
    hdr.st.s32_4 = (hdr.st.s32_0) ^ (hdr.st.s32_3);
    hdr.st.s32_5 = (hdr.st.s32_1) ^ (hdr.st.s32_7);
  }
  action compute_rotate_left_116() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
  }
  action compute_rotate_left_117() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_5[24:0] ++ hdr.st.s32_5[31:25]; }
    hdr.st.s32_6 = (hdr.st.s32_7) + (hdr.st.s32_4);
  }
  action compute_rotate_left_118() {
    hdr.st.s32_5 = hdr.st.s32_6[15:0] ++ hdr.st.s32_6[31:16];
  }
  action compute_rotate_left_143_x() {
    hdr.st.s32_2 = (hdr.st.s32_0) ^ (hdr.st.s32_3);
    hdr.st.s32_6 = hdr.st.s32_4 + hdr.st.s32_1;
  }
  action compute_op_xor_399() {
    hdr.st.s32_0 = hdr.st.s32_5 ^ hdr.st.s32_6;
    hdr.st.s32_3 = hdr.st.s32_6 ^ hdr.st.s32_7;
  }
  action compute_rotate_left_143() {
    @in_hash { hdr.st.s32_4 = hdr.st.s32_2[26:0] ++ hdr.st.s32_2[31:27]; }
    hdr.st.s32_5 = (hdr.st.s32_3) + (hdr.st.s32_2);
  }
  action select_unrolled__54() {
    @in_hash { hdr.st.s32_1 = hdr.hdr2.data1; }
  }
  action compute_rotate_left_144_x() {
    @in_hash { hdr.st.s32_2 = (hdr.st.s32_0) ^ (hdr.st.s32_1); }
    hdr.st.s32_3 = hdr.st.s32_5[15:0] ++ hdr.st.s32_5[31:16];
    hdr.st.s32_7 = (hdr.st.s32_4) ^ (hdr.st.s32_5);
  }
  action compute_rotate_left_144() {
    hdr.st.s32_0 = hdr.st.s32_2[23:0] ++ hdr.st.s32_2[31:24];
    @in_hash { hdr.st.s32_4 = hdr.st.s32_7[18:0] ++ hdr.st.s32_7[31:19]; }
    hdr.st.s32_5 = hdr.st.s32_9 + hdr.st.s32_2;
  }
  action compute_rotate_left_147_x() {
    hdr.st.s32_2 = (hdr.st.s32_0) ^ (hdr.st.s32_5);
    hdr.st.s32_6 = (hdr.st.s32_5) + (hdr.st.s32_7);
  }
  action compute_rotate_left_148() {
    hdr.st.s32_0 = hdr.st.s32_6[15:0] ++ hdr.st.s32_6[31:16];
    hdr.st.s32_5 = (hdr.st.s32_4) ^ (hdr.st.s32_6);
    hdr.st.s32_7 = hdr.st.s32_3 + hdr.st.s32_2;
  }
  action compute_rotate_left_147() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_2[24:0] ++ hdr.st.s32_2[31:25]; }
  }
  action compute_rotate_left_149() {
    @in_hash { hdr.st.s32_4 = hdr.st.s32_5[26:0] ++ hdr.st.s32_5[31:27]; }
    hdr.st.s32_6 = (hdr.st.s32_7) + (hdr.st.s32_5);
  }
  action compute_rotate_left_150_x() {
    hdr.st.s32_2 = (hdr.st.s32_3) ^ (hdr.st.s32_7);
    hdr.st.s32_5 = hdr.st.s32_6[15:0] ++ hdr.st.s32_6[31:16];
    hdr.st.s32_8 = (hdr.st.s32_4) ^ (hdr.st.s32_6);
  }
  action compute_rotate_left_150() {
    hdr.st.s32_6 = hdr.st.s32_0 ^ hdr.st.s32_2;
  }
  action compute_rotate_left_153_x() {
    hdr.st.s32_0 = (hdr.st.s32_3) ^ (hdr.st.s32_6);
    hdr.st.s32_2 = (hdr.st.s32_6) + (hdr.st.s32_8);
  }
  action compute_rotate_left_153() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_0[24:0] ++ hdr.st.s32_0[31:25]; }
    hdr.st.s32_6 = hdr.st.s32_2[15:0] ++ hdr.st.s32_2[31:16];
    hdr.st.s32_7 = (hdr.st.s32_4) ^ (hdr.st.s32_2);
    hdr.st.s32_8 = hdr.st.s32_5 + hdr.st.s32_0;
  }
  action compute_rotate_left_155() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_7[26:0] ++ hdr.st.s32_7[31:27]; }
    hdr.st.s32_2 = (hdr.st.s32_3) ^ (hdr.st.s32_8);
  }
  action compute_op_xor_417() {
    @in_hash { hdr.st.s32_4 = hdr.st.s32_8 ^ hdr.st.s32_1; }
  }
  action compute_rotate_left_156() {
    hdr.st.s32_1 = hdr.st.s32_2[23:0] ++ hdr.st.s32_2[31:24];
    hdr.st.s32_3 = (hdr.st.s32_4) + (hdr.st.s32_7);
    hdr.st.s32_5 = hdr.st.s32_6 + hdr.st.s32_2;
  }
  action compute_rotate_left_157() {
    hdr.st.s32_2 = hdr.st.s32_3[15:0] ++ hdr.st.s32_3[31:16];
    hdr.st.s32_4 = (hdr.st.s32_0) ^ (hdr.st.s32_3);
    hdr.st.s32_6 = (hdr.st.s32_1) ^ (hdr.st.s32_5);
  }
  action compute_rotate_left_158() {
    @in_hash { hdr.st.s32_0 = hdr.st.s32_4[18:0] ++ hdr.st.s32_4[31:19]; }
  }
  action compute_rotate_left_159() {
    @in_hash { hdr.st.s32_3 = hdr.st.s32_6[24:0] ++ hdr.st.s32_6[31:25]; }
    hdr.st.s32_7 = (hdr.st.s32_5) + (hdr.st.s32_4);
  }
  action compute_rotate_left_160() {
    hdr.st.s32_6 = hdr.st.s32_7[15:0] ++ hdr.st.s32_7[31:16];
  }
  action fin() { hdr.st.setInvalid(); ig_tm_md.ucast_egress_port = 1; }
  apply { init(); if (ig_intr_md.ingress_port == 2) { compute_op_add_281(); compute_op_add_285_b(); compute_rotate_left_143_x(); compute_op_add_285(); compute_op_xor_399(); compute_rotate_left_53_x(); compute_rotate_left_143(); compute_rotate_left_144_x(); compute_rotate_left_144(); compute_rotate_left_147_x(); compute_rotate_left_148(); compute_rotate_left_147(); compute_rotate_left_149(); compute_rotate_left_150_x(); compute_rotate_left_150(); } fin(); }
}
parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) { state start { pkt.extract(eg_intr_md); transition accept; } }
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
