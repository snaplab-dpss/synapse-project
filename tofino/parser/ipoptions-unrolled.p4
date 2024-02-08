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
// const PortId_t CPU_PCIE_PORT = 192;

// BFN-T10-064Q
// BFN-T10-032Q
// PCIe port when using the tofino model
const PortId_t CPU_PCIE_PORT = 320;

const ether_type_t  ETHERTYPE_IPV4 = 0x0800;
const ether_type_t  ETHERTYPE_ARP  = 0x0806;
const ether_type_t  ETHERTYPE_IPV6 = 0x86dd;
const ether_type_t  ETHERTYPE_VLAN = 0x8100;

const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP  = 6;
const ip_protocol_t IP_PROTOCOLS_UDP  = 17;

header cpu_h {
  bit<16> code_path;
  bit<7> pad0;
  port_t in_port;
  bit<7> pad1;
  port_t out_port;
}

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

header ipv4_options_h {
  bit<32> data;
}

typedef ipv4_options_h[10] ipv4_options_t;

header tcpudp_h {
  bit<16> src_port;
  bit<16> dst_port;
}

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
  cpu_h          cpu;
  ethernet_h     ethernet;
  ipv4_h         ipv4;
  ipv4_options_t ipv4_options;
  tcpudp_h       tcpudp;
}

struct my_egress_metadata_t {}

struct my_egress_headers_t {
  cpu_h          cpu;
  ethernet_h     ethernet;
  ipv4_h         ipv4;
  ipv4_options_h ipv4_options;
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
    tofino_parser.apply(pkt, ig_intr_md);

    transition select(ig_intr_md.ingress_port) {
      CPU_PCIE_PORT: parse_cpu;
      default: parse_ethernet;
    }
  }

  state parse_cpu {
    pkt.extract(hdr.cpu);
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

    transition select (hdr.ipv4.ihl) {
      0x5: parse_ipv4_no_options; 
      0x6: parse_ipv4_options_1;
      0x7: parse_ipv4_options_2;
      0x8: parse_ipv4_options_3;
      0x9: parse_ipv4_options_4;
      0xa: parse_ipv4_options_5;
      0xb: parse_ipv4_options_6;
      0xc: parse_ipv4_options_7;
      0xd: parse_ipv4_options_8;
      0xe: parse_ipv4_options_9;
      0xf: parse_ipv4_options_10;
      /* Omit the default case to drop packets 0..4 */
    }
  }

  state parse_ipv4_options_1 {
    pkt.extract(hdr.ipv4_options[0]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_2 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_3 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_4 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    pkt.extract(hdr.ipv4_options[3]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_5 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    pkt.extract(hdr.ipv4_options[3]);
    pkt.extract(hdr.ipv4_options[4]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_6 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    pkt.extract(hdr.ipv4_options[3]);
    pkt.extract(hdr.ipv4_options[4]);
    pkt.extract(hdr.ipv4_options[5]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_7 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    pkt.extract(hdr.ipv4_options[3]);
    pkt.extract(hdr.ipv4_options[4]);
    pkt.extract(hdr.ipv4_options[5]);
    pkt.extract(hdr.ipv4_options[6]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_8 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    pkt.extract(hdr.ipv4_options[3]);
    pkt.extract(hdr.ipv4_options[4]);
    pkt.extract(hdr.ipv4_options[5]);
    pkt.extract(hdr.ipv4_options[6]);
    pkt.extract(hdr.ipv4_options[7]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_9 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    pkt.extract(hdr.ipv4_options[3]);
    pkt.extract(hdr.ipv4_options[4]);
    pkt.extract(hdr.ipv4_options[5]);
    pkt.extract(hdr.ipv4_options[6]);
    pkt.extract(hdr.ipv4_options[7]);
    pkt.extract(hdr.ipv4_options[8]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_options_10 {
    pkt.extract(hdr.ipv4_options[0]);
    pkt.extract(hdr.ipv4_options[1]);
    pkt.extract(hdr.ipv4_options[2]);
    pkt.extract(hdr.ipv4_options[3]);
    pkt.extract(hdr.ipv4_options[4]);
    pkt.extract(hdr.ipv4_options[5]);
    pkt.extract(hdr.ipv4_options[6]);
    pkt.extract(hdr.ipv4_options[7]);
    pkt.extract(hdr.ipv4_options[8]);
    pkt.extract(hdr.ipv4_options[9]);
    transition parse_ipv4_no_options;
  }

  state parse_ipv4_no_options {
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
  action flood() {
    ig_tm_md.mcast_grp_a = 1;
  }

  action fwd(port_t port) {
    ig_tm_md.ucast_egress_port = port;
  }

  action drop() {
    ig_dprsr_md.drop_ctl = 1;
  }

  action send_to_cpu(bit<16> code_path) {
    hdr.cpu.setValid();
    hdr.cpu.code_path = code_path;
    hdr.cpu.in_port = ig_intr_md.ingress_port;
    fwd(CPU_PCIE_PORT);
  }

  apply {
    if (hdr.cpu.isValid()) {
      fwd(hdr.cpu.out_port);
      hdr.cpu.setInvalid();
    } else {
      if (hdr.ipv4_options[0].isValid() /* HACK */) {
        fwd(1);
      } else {
        fwd(2);
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
  apply {
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
