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
			ether_type_t.IPV4	: parse_ipv4;
			ether_type_t.CUCKOO	: parse_cuckoo_op;
			default				: accept;
		}
	}

	state parse_cuckoo_op {
		pkt.extract(hdr.cuckoo_op);
		transition select(hdr.cuckoo_op.op) {
			cuckoo_ops_t.INSERT	: parse_cuckoo_cntr;
			cuckoo_ops_t.SWAP	: parse_swap_entry;
			cuckoo_ops_t.LOOKUP	: parse_cuckoo_cntr;
			default				: reject;
		}
	}

	state parse_cuckoo_cntr {
		pkt.extract(hdr.cuckoo_cntr);
		transition parse_ipv4;
	}

	state parse_swap_entry {
		pkt.extract(hdr.swap_entry);
		transition parse_ipv4;
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
		transition accept;
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		ig_md.l4_lookup = {hdr.udp.src_port, hdr.udp.dst_port};
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

		transition select(eg_intr_md.egress_port) {
			RECIRCULATE_PORT_SWAP_TO_CUCKOO	: parse_swap_mirror;
			default							: reject;
		}
	}

	state parse_swap_mirror {
		pkt.extract(eg_md.swap_mirror);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.ether_type) {
			ether_type_t.CUCKOO	: parse_cuckoo_op;
			default				: reject;
		}
	}

	state parse_cuckoo_op {
		pkt.extract(hdr.cuckoo_op);
		transition select(hdr.cuckoo_op.op) {
			cuckoo_ops_t.WAIT		: parse_cuckoo_cntr;
			cuckoo_ops_t.SWAPPED	: parse_swap_entry;
			default					: reject;
		}
	}

	state parse_cuckoo_cntr {
		pkt.extract(hdr.cuckoo_cntr);
		transition select(hdr.cuckoo_cntr.has_swap) {
			1		: parse_carry_swap_entry;
			default	: reject;
		}
	}

	state parse_swap_entry {
		pkt.extract(hdr.swap_entry);
		transition select(hdr.swap_entry.has_swap) {
			1		: parse_carry_swap_entry;
			default	: reject;
		}
	}

	state parse_carry_swap_entry {
		pkt.extract(hdr.carry_swap_entry);
		transition accept;
	}
}

#endif
