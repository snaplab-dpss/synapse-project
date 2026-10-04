#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "headers.p4"

#ifndef _DEPARSER_
#define _DEPARSER_

// ---------------------------------------------------------------------------
// Ingress Deparser
// ---------------------------------------------------------------------------

control SwitchIngressDeparser(
	packet_out pkt,
	inout header_t hdr,
	in ingress_metadata_t ig_md,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
	Mirror() mirror;
	Digest<hh_digest_t>() hh_digest;

	apply {
		if (ig_dprsr_md.mirror_type == MIRROR_TYPE_I2E) {
			mirror.emit<mirror_h>(ig_md.mirror_session, {ig_md.pkt_type});
		}

		if (ig_dprsr_md.digest_type == 1) {
			hh_digest.pack({hdr.netcache.key, ig_md.cm_result});
		}

		pkt.emit(hdr);
	}
}

// ---------------------------------------------------------------------------
// Egress Deparser
// ---------------------------------------------------------------------------

control SwitchEgressDeparser(
	packet_out pkt,
	inout header_t hdr,
	in egress_metadata_t eg_md,
	in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md
) {
	apply {
		pkt.emit(hdr);
	}
}

#endif
