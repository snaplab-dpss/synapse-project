// Does work guarded by a condition on a table's result have to wait a stage for that table?
#include <core.p4>
#include <t2na.p4>

header eth_h { bit<48> dst; bit<48> src; bit<16> type; }
struct hdrs_t { eth_h eth; }
struct meta_t { bit<32> dev; bit<32> a; bit<32> b; }

parser IgParser(packet_in pkt, out hdrs_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig) {
  state start { pkt.extract(ig); pkt.advance(PORT_METADATA_SIZE); meta.dev = 0; meta.a = 0; meta.b = 0;
                pkt.extract(hdr.eth); transition accept; }
}

control Ig(inout hdrs_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig,
           in ingress_intrinsic_metadata_from_parser_t igp,
           inout ingress_intrinsic_metadata_for_deparser_t igd,
           inout ingress_intrinsic_metadata_for_tm_t igt) {
  action set_dev(bit<32> d) { meta.dev = d; }
  table port_to_dev { key = { ig.ingress_port: exact; } actions = { set_dev; } size = 64; }

  action step_a() { meta.a = hdr.eth.dst[31:0] ^ 32w0x9e3779b9; }
  action step_b() { meta.b = meta.a + 32w0x85ebca6b; }

  apply {
    port_to_dev.apply();
    if (meta.dev != 0) {          // guarded by the table's result
      step_a();
      step_b();
    }
    igt.ucast_egress_port = ig.ingress_port;
  }
}

control IgDep(packet_out pkt, inout hdrs_t hdr, in meta_t meta,
              in ingress_intrinsic_metadata_for_deparser_t igd) { apply { pkt.emit(hdr); } }
parser EgParser(packet_in pkt, out hdrs_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg) {
  state start { pkt.extract(eg); transition accept; } }
control Eg(inout hdrs_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg,
           in egress_intrinsic_metadata_from_parser_t egp,
           inout egress_intrinsic_metadata_for_deparser_t egd,
           inout egress_intrinsic_metadata_for_output_port_t ego) { apply {} }
control EgDep(packet_out pkt, inout hdrs_t hdr, in meta_t meta,
              in egress_intrinsic_metadata_for_deparser_t egd) { apply { pkt.emit(hdr); } }
Pipeline(IgParser(), Ig(), IgDep(), EgParser(), Eg(), EgDep()) pipe;
Switch(pipe) main;
