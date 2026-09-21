// Can the policer's flow table carry a DirectMeter AND an idle timeout, and charge the FCS?
// The shape synapse would emit for pol: one exact table on the flow key, a DirectMeter(BYTES)
// attached to it, `idle_timeout = true` for tb_expire, and an action that executes the meter with
// adjust_byte_count = 4 (the FCS the ingress byte meter charges but DPDK's packet_length excludes).
// The colour is written into the IPv4 DSCP, which is what marking instead of dropping needs.
// See README.md.
#include <core.p4>
#include <t2na.p4>

header ethernet_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ipv4_h { bit<8> vihl; bit<8> tos; bit<16> len; bit<16> id; bit<16> frag; bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst; }

struct headers_t { ethernet_h eth; ipv4_h ipv4; }
struct meta_t { bit<8> color; }

parser IgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start { pkt.extract(ig_intr_md); pkt.advance(PORT_METADATA_SIZE); pkt.extract(hdr.eth); pkt.extract(hdr.ipv4); transition accept; }
}

control IgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  Checksum() ipv4_csum;
  apply {
    hdr.ipv4.csum = ipv4_csum.update({hdr.ipv4.vihl, hdr.ipv4.tos, hdr.ipv4.len, hdr.ipv4.id, hdr.ipv4.frag, hdr.ipv4.ttl, hdr.ipv4.proto, hdr.ipv4.src,
                                      hdr.ipv4.dst});
    pkt.emit(hdr);
  }
}

control Ig(inout headers_t hdr, inout meta_t meta, in ingress_intrinsic_metadata_t ig_intr_md, in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
           inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md, inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

  DirectMeter(MeterType_t.BYTES) tb_meter;

  // The 4 bytes are the FCS: an ingress byte meter charges the frame including it, the C's
  // packet_length excludes it.
  // Named argument, not positional: one bare argument is ambiguous between the colour-aware
  // execute(MeterColor_t) and this one, and bf-p4c rejects it.
  action policed() { meta.color = tb_meter.execute(adjust_byte_count = 32w4); }

  table tb {
    key     = { hdr.ipv4.dst : exact; }
    actions = { policed; @defaultonly NoAction; }
    default_action = NoAction();
    size           = 65536;
    meters         = tb_meter;
    idle_timeout   = true;
  }

  action fwd() { ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port; ig_tm_md.bypass_egress = 1; }

  apply {
    meta.color = 0;
    tb.apply();
    // Mark instead of drop: RED (3) lands in the DSCP, the packet still leaves.
    if (meta.color == 3) {
      hdr.ipv4.tos = 8w0xFC;
    }
    fwd();
  }
}

parser EgParser(packet_in pkt, out headers_t hdr, out meta_t meta, out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); transition accept; }
}
control Eg(inout headers_t hdr, inout meta_t meta, in egress_intrinsic_metadata_t eg_intr_md, in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
           inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md, inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) { apply {} }
control EgDeparser(packet_out pkt, inout headers_t hdr, in meta_t meta, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) { apply { pkt.emit(hdr); } }

Pipeline(IgParser(), Ig(), IgDeparser(), EgParser(), Eg(), EgDeparser()) pipe;
Switch(pipe) main;
