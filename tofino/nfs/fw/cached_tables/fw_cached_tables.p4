#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "constants.p4"
#include "headers.p4"
#include "cached_table.p4"
#include "hashing.p4"

parser TofinoIngressParser(
		packet_in pkt,
		out ingress_intrinsic_metadata_t ig_intr_md) {
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
		transition select(hdr.ethernet.etherType) {
			TYPE_IPV4: parse_ipv4;
			default: accept;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select(hdr.ipv4.protocol) {
			IP_PROTOCOLS_TCP: parse_tcpudp;
			IP_PROTOCOLS_UDP: parse_tcpudp;
			default: accept;
		}
	}

	state parse_tcpudp {
		pkt.extract(hdr.tcpudp);
		transition accept;
	}
}

control Ingress(
	/* User */
	inout my_ingress_headers_t  hdr,
	inout my_ingress_metadata_t meta,

	/* Intrinsic */
	in    ingress_intrinsic_metadata_t               ig_intr_md,
	in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
	inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
	inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
	Hashing() hashing;
	CachedTable() cached_table;

	bit<16> port;
	hash_t hash;
	bool hit;

	action forward(port_t p) {
		ig_tm_md.ucast_egress_port = p;
	}

	action send_to_controller() {
		hdr.cpu.setValid();
		hdr.cpu.code_path = 1234;
		hdr.cpu.in_port = ig_intr_md.ingress_port;
		forward(CPU_PCIE_PORT);
	}

	action populate(bit<16> out_port) {
		port = out_port;
	}

	table forwarder {
		key = {}

		actions = {
			populate;
		}

		size = 1;
	}

	apply {
		if (hdr.cpu.isValid()) {
			forward(hdr.cpu.out_port);
			hdr.cpu.setInvalid();
		} else {
			if (ig_intr_md.ingress_port == LAN) {
				forwarder.apply();

				key_t k = {
					hdr.ipv4.src_addr,
					hdr.ipv4.dst_addr,
					hdr.tcpudp.src_port,
					hdr.tcpudp.dst_port
				};

				hashing.apply(k, hash);

				cached_table.apply(
					ig_intr_md.ingress_mac_tstamp[47:16],
					ct_op_t.COND_WRITE,
					hash,
					k,
					port,
					hit
				);

				if (!hit) {
					send_to_controller();
				} else {
					forward(port[8:0]);
				}
			} else {
				key_t k = {
					hdr.ipv4.dst_addr,
					hdr.ipv4.src_addr,
					hdr.tcpudp.dst_port,
					hdr.tcpudp.src_port
				};

				hashing.apply(k, hash);
				
				cached_table.apply(
					ig_intr_md.ingress_mac_tstamp[47:16],
					ct_op_t.READ,
					hash,
					k,
					port,
					hit
				);

				if (!hit) {
					send_to_controller();
				} else {
					forward(port[8:0]);
				}
			}
		}

		ig_tm_md.bypass_egress = 1;
	}
}

control IngressDeparser(
	packet_out pkt,

	/* User */
	inout my_ingress_headers_t  hdr,
	in    my_ingress_metadata_t meta,

	/* Intrinsic */
	in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
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
	out empty_header_t hdr,
	out empty_metadata_t eg_md,
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
	inout empty_header_t hdr,
	inout empty_metadata_t eg_md,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
	inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md)
{
  apply {}
}

control EgressDeparser(
	packet_out pkt,
	inout empty_header_t hdr,
	in empty_metadata_t eg_md,
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
