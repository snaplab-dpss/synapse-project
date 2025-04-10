#ifndef _DEPARSER_
#define _DEPARSER_

// ---------------------------------------------------------------------------
// Ingress Deparser
// ---------------------------------------------------------------------------

control IngressDeparser(packet_out pkt,
						inout header_t hdr,
						in ingress_metadata_t ig_md,
						in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
	apply {
		pkt.emit(hdr);
	}
}

// ---------------------------------------------------------------------------
// Egress Deparser
// ---------------------------------------------------------------------------

control EgressDeparser(packet_out pkt,
					   inout header_t hdr,
					   in egress_metadata_t eg_md,
					   in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {
	apply {
		pkt.emit(hdr);
	}
}

#endif
