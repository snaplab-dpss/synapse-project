// Can the CNAME's rdata be carried through intact as a varbit header (sized by rd_length), instead
// of being skipped with pkt.advance and lost at the deparser?
#include <core.p4>
#include <t2na.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> type; }
header ipv4_t {
  bit<4> version; bit<4> ihl; bit<8> tos; bit<16> len; bit<16> id; bit<3> flags; bit<13> frag;
  bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst;
}
header udp_t { bit<16> sport; bit<16> dport; bit<16> len; bit<16> csum; }
header dns_t { bit<16> id; bit<16> flags; bit<16> qd; bit<16> an; bit<16> ns; bit<16> ar; }
header dns_answer_t { bit<16> name; bit<16> rr_type; bit<16> rr_class; bit<32> ttl; bit<8> rd_length_hi; bit<8> rd_length_lo; }
header dns_rdata_t { varbit<2040> data; }
header dns_a_ip_t { bit<32> address; }

struct headers_t { ethernet_t eth; ipv4_t ipv4; udp_t udp; dns_t dns; dns_answer_t cname; dns_rdata_t cname_rdata; dns_answer_t answer; dns_a_ip_t a; }
struct metadata_t { bit<8> found; }

parser IngressParser(packet_in pkt, out headers_t hdr, out metadata_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); transition parse_eth; }
  state parse_eth { pkt.extract(hdr.eth); transition select(hdr.eth.type) { 0x800: parse_ipv4; default: accept; } }
  state parse_ipv4 { pkt.extract(hdr.ipv4); transition select(hdr.ipv4.proto) { 17: parse_udp; default: accept; } }
  state parse_udp { pkt.extract(hdr.udp); transition select(hdr.udp.sport) { 53: parse_dns; default: accept; } }
  state parse_dns { pkt.extract(hdr.dns); meta.found = 0; transition parse_answer; }
  state parse_answer {
    transition select(pkt.lookahead<bit<32>>()[15:0]) { 1: parse_a; 5: parse_cname; default: accept; }
  }
  state parse_cname {
    pkt.extract(hdr.cname);
    pkt.extract(hdr.cname_rdata, (bit<32>)hdr.cname.rd_length_lo * 8);
    transition parse_answer_after_cname;
  }
  state parse_answer_after_cname {
    transition select(pkt.lookahead<bit<32>>()[15:0]) { 1: parse_a; default: accept; }
  }
  state parse_a { pkt.extract(hdr.answer); pkt.extract(hdr.a); meta.found = 1; transition accept; }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in ingress_intrinsic_metadata_t ig_intr_md,
                in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
                inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
  action set_id(bit<32> id) { hdr.dns.id = id[15:0]; }
  table by_address { key = { hdr.a.address: exact; } actions = { set_id; } size = 1024; }
  apply {
    ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
    if (meta.found == 1) { by_address.apply(); }
  }
}
control IngressDeparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}
struct eg_headers_t {}
struct eg_metadata_t {}
parser EgressParser(packet_in pkt, out eg_headers_t hdr, out eg_metadata_t md, out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); transition accept; }
}
control Egress(inout eg_headers_t hdr, inout eg_metadata_t md, in egress_intrinsic_metadata_t eg_intr_md,
               in egress_intrinsic_metadata_from_parser_t eg_prsr_md, inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
               inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgressDeparser(packet_out pkt, inout eg_headers_t hdr, in eg_metadata_t md, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {
  apply { pkt.emit(hdr); }
}
Pipeline(IngressParser(), Ingress(), IngressDeparser(), EgressParser(), Egress(), EgressDeparser()) pipe;
Switch(pipe) main;
