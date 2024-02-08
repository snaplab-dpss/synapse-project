#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

typedef bit<9>  port_t;
typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> ether_type_t;
typedef bit<8>  ip_protocol_t;

const port_t RECIRCULATING_PORT = 68;

// BFN-T10-032D
// BFN-T10-032D-024
// BFN-T10-032D-020
// BFN-T10-032D-018
// const PortId_t CPU = 192;

// BFN-T10-064Q
// BFN-T10-032Q
// PCIe port when using the tofino model
// const PortId_t CPU = 320;

const PortId_t LAN = 0;
const PortId_t WAN = 1;
const PortId_t CPU = 2;

const ether_type_t  ETHERTYPE_IPV4 = 0x0800;
const ether_type_t  ETHERTYPE_ARP  = 0x0806;
const ether_type_t  ETHERTYPE_IPV6 = 0x86dd;
const ether_type_t  ETHERTYPE_VLAN = 0x8100;

const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP  = 6;
const ip_protocol_t IP_PROTOCOLS_UDP  = 17;

header ethernet_h {
  mac_addr_t   dst_addr;
  mac_addr_t   src_addr;
  ether_type_t ether_type;
}

header ipv4_h {
  bit<4>        version;
  bit<4>        ihl;
  bit<8>        dscp;
  bit<16>       total_len;
  bit<16>       identification;
  bit<3>        flags;
  bit<13>       frag_offset;
  bit<8>        ttl;
  ip_protocol_t protocol;
  bit<16>       hdr_checksum;
  ipv4_addr_t   src_addr;
  ipv4_addr_t   dst_addr;
}

header tcpudp_h {
  bit<16> src_port;
  bit<16> dst_port;
}

struct my_ingress_metadata_t {
  bool checksum_update_ipv4;
}

struct my_ingress_headers_t {
  ethernet_h     ethernet;
  ipv4_h         ipv4;
  tcpudp_h       tcpudp;
}

struct my_egress_metadata_t {}

struct my_egress_headers_t {
  ethernet_h     ethernet;
  ipv4_h         ipv4;
  tcpudp_h       tcpudp;
}

parser TofinoIngressParser(
    packet_in pkt,
    out ingress_intrinsic_metadata_t ig_intr_md)
{
  state start {
    pkt.extract(ig_intr_md);
    transition select(ig_intr_md.resubmit_flag) {
      1 : parse_resubmit;
      0 : parse_port_metadata;
    }
  }

  state parse_resubmit {
    // Parse resubmitted packet here.
    transition reject;
  }

  state parse_port_metadata {
    pkt.advance(PORT_METADATA_SIZE);
    transition accept;
  }
}

parser IngressParser(
  packet_in pkt,

  /* User */    
  out my_ingress_headers_t  hdr,
  out my_ingress_metadata_t meta,

  /* Intrinsic */
  out ingress_intrinsic_metadata_t  ig_intr_md)
{
  TofinoIngressParser() tofino_parser;
  
  /* This is a mandatory state, required by Tofino Architecture */
  state start {
    meta.checksum_update_ipv4 = false;

    tofino_parser.apply(pkt, ig_intr_md);
    transition parse_ethernet;
  }

  state parse_ethernet {
    pkt.extract(hdr.ethernet);

    transition select(hdr.ethernet.ether_type) {
      ETHERTYPE_IPV4: parse_ipv4;
      default: reject;
    }
  }

  state parse_ipv4 {
    pkt.extract(hdr.ipv4);

    transition select (hdr.ipv4.protocol) {
        IP_PROTOCOLS_TCP: parse_tcpudp;
        default: accept;
    }
  }

  state parse_tcpudp {
    pkt.extract(hdr.tcpudp);
    transition accept;
  }
}

control Ingress(
  inout my_ingress_headers_t  hdr,
  inout my_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_t ig_intr_md,
  in    ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
  inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
  inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)
{
  action fwd(port_t port) {
    ig_tm_md.ucast_egress_port = port;
  }

  action drop() {
    ig_dprsr_md.drop_ctl = 1;
  }

  action send_to_backend(bit<32> backend_addr) { 
    hdr.ipv4.dst_addr = backend_addr;
    meta.checksum_update_ipv4 = true;
  }

  table fw_int_to_ext {
    key = { 
      hdr.ipv4.src_addr: exact;
      hdr.ipv4.dst_addr: exact;
      hdr.tcpudp.src_port: exact;
      hdr.tcpudp.dst_port: exact;
    }

    actions = {
      fwd;
    }

    size = 65536;
  }

  table fw_ext_to_int {
    key = { 
      hdr.ipv4.src_addr: exact;
      hdr.ipv4.dst_addr: exact;
      hdr.tcpudp.src_port: exact;
      hdr.tcpudp.dst_port: exact;
    }

    actions = {
      fwd;
    }

    size = 65536;
  }

  apply {
    if (ig_intr_md.ingress_port == LAN) {
      if (!fw_int_to_ext.apply().hit) {
        drop();
      }
    } else {
      if (!fw_ext_to_int.apply().hit) {
        drop();
      }
    }
  }
}

control IngressDeparser(
  packet_out pkt,
  inout my_ingress_headers_t hdr,
  in    my_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md)
{
  Checksum() ipv4_checksum;
  
  apply {
    if (meta.checksum_update_ipv4) {
      hdr.ipv4.hdr_checksum = ipv4_checksum.update({
        hdr.ipv4.version,
        hdr.ipv4.ihl,
        hdr.ipv4.dscp,
        hdr.ipv4.total_len,
        hdr.ipv4.identification,
        hdr.ipv4.flags,
        hdr.ipv4.frag_offset,
        hdr.ipv4.ttl,
        hdr.ipv4.protocol,
        hdr.ipv4.src_addr,
        hdr.ipv4.dst_addr
      });
    }

    pkt.emit(hdr);
  }
}

parser TofinoEgressParser(
    packet_in pkt,
    out egress_intrinsic_metadata_t eg_intr_md)
{
  state start {
    pkt.extract(eg_intr_md);
    transition accept;
  }
}

parser EgressParser(
    packet_in pkt,
    out my_egress_headers_t hdr,
    out my_egress_metadata_t eg_md,
    out egress_intrinsic_metadata_t eg_intr_md)
{
  TofinoEgressParser() tofino_parser;

  /* This is a mandatory state, required by Tofino Architecture */
  state start {
    tofino_parser.apply(pkt, eg_intr_md);
    transition accept;
  }
}

control Egress(
    inout my_egress_headers_t hdr,
    inout my_egress_metadata_t eg_md,
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
    inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md)
{
  apply {}
}

control EgressDeparser(
    packet_out pkt,
    inout my_egress_headers_t hdr,
    in my_egress_metadata_t eg_md,
    in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md)
{
  apply {
    pkt.emit(hdr);
  }
}

Pipeline(
  IngressParser(),
  Ingress(),
  IngressDeparser(),
  EgressParser(),
  Egress(),
  EgressDeparser()
) pipe;

Switch(pipe) main;
