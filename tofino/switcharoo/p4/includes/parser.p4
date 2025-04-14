#ifndef _PARSER_
#define _PARSER_

#include "constants.p4"
#include "headers.p4"

// ---------------------------------------------------------------------------
// Ingress Parser
// ---------------------------------------------------------------------------

parser IngressParser(packet_in pkt,
					 out header_t hdr,
					 out ingress_metadata_t ig_md,
					 out ingress_intrinsic_metadata_t ig_intr_md) {
	state start {
		pkt.extract(ig_intr_md);
		pkt.advance(PORT_METADATA_SIZE);
		transition meta_init;
	}

	state meta_init {
		ig_md.l4_lookup = {0, 0};
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.ether_type) {
			ETHERTYPE_IPV4	: parse_ipv4;
			default			: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select(hdr.ipv4.protocol) {
			(ip_proto_t.TCP)	: parse_tcp;
			(ip_proto_t.UDP)	: parse_udp;
			default				: reject;
		}
	}

	state parse_tcp {
		pkt.extract(hdr.tcp);
		ig_md.l4_lookup = {hdr.tcp.src_port, hdr.tcp.dst_port};
		transition select(hdr.tcp.src_port, hdr.tcp.dst_port) {
			(CUCKOO_PORT, _)	: parse_kv;
			(_, CUCKOO_PORT)	: parse_kv;
			default				: reject;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		ig_md.l4_lookup = {hdr.udp.src_port, hdr.udp.dst_port};
		transition select(hdr.udp.src_port, hdr.udp.dst_port) {
			(CUCKOO_PORT, _)	: parse_kv;
			(_, CUCKOO_PORT)	: parse_kv;
			default				: reject;
		}
	}

	state parse_kv {
		pkt.extract(hdr.kv);
		transition select(hdr.kv.status) {
			2		: parse_cuckoo;
			default	: accept;
		}
	}

	state parse_cuckoo {
		pkt.extract(hdr.cuckoo);
		transition select(hdr.cuckoo.op) {
			cuckoo_ops_t.SWAP	: parse_cur_swap;
			default				: accept;
		}
	}

	state parse_cur_swap {
		pkt.extract(hdr.cur_swap);
		transition accept;
	}
}

// ---------------------------------------------------------------------------
// Egress Parser
// ---------------------------------------------------------------------------

parser EgressParser(packet_in pkt,
					out header_t hdr,
					out egress_metadata_t eg_md,
					out egress_intrinsic_metadata_t eg_intr_md) {
	state start {
		pkt.extract(eg_intr_md);
		transition accept;
	}
}

#endif
