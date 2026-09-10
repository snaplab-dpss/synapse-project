// Question: can a parser copy an already-extracted header field into metadata, and does that
// copy escape the header's `deparsed exact_containers` byte grid when the chain operates on it?
#include <t2na.p4>

header eth_h { bit<48> dst; bit<48> src; bit<16> etype; }
header w_h   { bit<32> a; bit<32> b; }
struct hdrs_t { eth_h eth; w_h w; }
struct meta_t { bit<32> sa; bit<32> sb; bit<32> r; }

parser IngressParser(packet_in pkt, out hdrs_t hdr, out meta_t m,
                     out ingress_intrinsic_metadata_t ig_intr_md) {
  state start {
    pkt.extract(ig_intr_md);
    pkt.advance(PORT_METADATA_SIZE);
    pkt.extract(hdr.eth);
    pkt.extract(hdr.w);
    m.sa = hdr.w.a;          // <-- the idiom under test
    m.sb = hdr.w.b;
    m.r  = 0;
    transition accept;
  }
}

control Ingress(inout hdrs_t hdr, inout meta_t m,
                in ingress_intrinsic_metadata_t ig_intr_md,
                in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
                inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
                inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  // rotate by 13 on the staged copy, then xor: the pattern that fails on header operands
  action rot()  { @in_hash { m.r = m.sa[18:0] ++ m.sa[31:19]; } }
  action mix()  { @in_hash { hdr.w.b = m.r ^ m.sa; } }
  table t1 { actions = { rot; } default_action = rot(); size = 1; }
  table t2 { actions = { mix; } default_action = mix(); size = 1; }
  apply {
    t1.apply(); t2.apply();

    ig_tm_md.ucast_egress_port = 0;
  }
}

control IngressDeparser(packet_out pkt, inout hdrs_t hdr, in meta_t m,
                        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}

parser EgressParser(packet_in pkt, out hdrs_t hdr, out meta_t m,
                    out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); m.sa = 0; m.sb = 0; m.r = 0; transition accept; }
}
control Egress(inout hdrs_t hdr, inout meta_t m,
               in egress_intrinsic_metadata_t a, in egress_intrinsic_metadata_from_parser_t b,
               inout egress_intrinsic_metadata_for_deparser_t c,
               inout egress_intrinsic_metadata_for_output_port_t d) { apply {} }
control EgressDeparser(packet_out pkt, inout hdrs_t hdr, in meta_t m,
                       in egress_intrinsic_metadata_for_deparser_t c) { apply { pkt.emit(hdr); } }

Pipeline(IngressParser(), Ingress(), IngressDeparser(),
         EgressParser(), Egress(), EgressDeparser()) pipe;
Switch(pipe) main;
