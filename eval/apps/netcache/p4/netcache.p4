#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "includes/constants.p4"
#include "includes/parser.p4"
#include "includes/deparser.p4"
#include "includes/stats/cm.p4"
#include "includes/stats/bloom.p4"

// ---------------------------------------------------------------------------
// Pipeline - Ingress
// ---------------------------------------------------------------------------

control SwitchIngress(
		inout header_t hdr,
		inout ingress_metadata_t ig_md,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

	Register<bit<NC_VAL_WIDTH>, bit<16>>(NC_ENTRIES) reg_vtable;

	RegisterAction<_, bit<16>, bit<NC_VAL_WIDTH>>(reg_vtable) ract_vtable_read = {
		void apply(inout bit<NC_VAL_WIDTH> val, out bit<NC_VAL_WIDTH> res) {
			res = val;
		}
	};

	RegisterAction<_, bit<16>, bit<NC_VAL_WIDTH>>(reg_vtable) ract_vtable_update = {
		void apply(inout bit<NC_VAL_WIDTH> val) {
			if (hdr.netcache.op == WRITE_QUERY) {
				val = hdr.netcache.val;
			} else {
				val = 0;
			}
		}
	};

	action set_lookup_metadata(vtableIdx_t vt_idx, keyIdx_t key_idx) {
		hdr.meta.vt_idx = vt_idx;
		hdr.meta.key_idx = key_idx;
	}

	action vtable_read() {
		hdr.netcache.val = ract_vtable_read.execute(hdr.meta.vt_idx);
	}

	action vtable_update() {
		ract_vtable_update.execute(hdr.meta.vt_idx);
	}

	action update_pkt_udp() {
		bit<48> mac_src_tmp = hdr.ethernet.src_addr;
		hdr.ethernet.src_addr = hdr.ethernet.dst_addr;
		hdr.ethernet.dst_addr = mac_src_tmp;

		bit<32> ip_src_tmp = hdr.ipv4.src_addr;
		hdr.ipv4.src_addr = hdr.ipv4.dst_addr;
		hdr.ipv4.dst_addr = ip_src_tmp;

		bit<16> port_src_tmp = hdr.udp.src_port;
		hdr.udp.src_port = hdr.udp.dst_port;
		hdr.udp.dst_port = port_src_tmp;
	}

	action update_pkt_tcp() {
		bit<48> mac_src_tmp = hdr.ethernet.src_addr;
		hdr.ethernet.src_addr = hdr.ethernet.dst_addr;
		hdr.ethernet.dst_addr = mac_src_tmp;

		bit<32> ip_src_tmp = hdr.ipv4.src_addr;
		hdr.ipv4.src_addr = hdr.ipv4.dst_addr;
		hdr.ipv4.dst_addr = ip_src_tmp;

		bit<16> port_src_tmp = hdr.tcp.src_port;
		hdr.tcp.src_port = hdr.tcp.dst_port;
		hdr.tcp.dst_port = port_src_tmp;
	}

	action set_normal_pkt() {
		hdr.bridged_md.setValid();
		hdr.bridged_md.pkt_type = PKT_TYPE_NORMAL;
	}

	action set_out_port(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action miss() {}

	table cache_lookup {
		key = {
			hdr.netcache.key : exact;
		}
		actions = {
			set_lookup_metadata;
			miss;
		}
		size = NC_ENTRIES;
		const default_action = miss();
	}

	table fwd {
		key = {
			ig_intr_md.ingress_port : exact;
			hdr.netcache.op         : exact;
		}
		actions = {
			set_out_port;
			miss;
		}
		const default_action = miss;
		size = 1024;
	}

	apply {
		if (hdr.netcache.isValid()) {
			cache_lookup.apply();

			if (hdr.netcache.op == READ_QUERY) {
				// Cache hit
				if (hdr.meta.cache_hit == 1) {
					// Update the packet header with the cached value.
					vtable_read();
					// Swap the IP src/dst and port src/dst.
					if (hdr.udp.isValid()) {
						update_pkt_udp();
					} else if (hdr.tcp.isValid()) {
						update_pkt_tcp();
					}
				}
			}

			else if (hdr.netcache.op == WRITE_QUERY || hdr.netcache.op == DELETE_QUERY) {
				// Update/delete the cached value with the received value.
				hdr.meta.vt_idx = hdr.netcache.key;
				vtable_update();
			}
		}
		set_normal_pkt();
		fwd.apply();
	}
}

// ---------------------------------------------------------------------------
// Pipeline - Egress
// ---------------------------------------------------------------------------

control SwitchEgress(
		inout header_t hdr,
		inout egress_metadata_t eg_md,
		in egress_intrinsic_metadata_t eg_intr_md,
		in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
		inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
		inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

	c_cm()      cm;
	c_bloom()   bloom;

	Register<bit<8>, bit<1>>(1) reg_sampl;
	Register<bit<32>, bit<NC_KEY_WIDTH>>(NC_ENTRIES) reg_key_count;

	bit<8>  sampl_cur;
	bit<32> cm_result;
	bit<1>  bloom_result;

	RegisterAction<bit<8>, bit<8>, bit<8>>(reg_sampl) ract_sampl = {
		void apply(inout bit<8> val, out bit<8> res) {
			res = 0;
			if (val < 3) {
				val = val + 1;
			} else {
				val = 0;
				res = 1;
			}
		}
	};

	RegisterAction<_, bit<16>, bit<32>>(reg_key_count) ract_key_count_incr = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	action sampl_check() {
		sampl_cur = ract_sampl.execute(0);
	}

	action key_count_incr() {
		ract_key_count_incr.execute(hdr.netcache.key);
	}

	action set_mirror() {
		// eg_md.egr_mir_ses = hdr.bridged_md.egr_mir_ses;
		eg_md.egr_mir_ses = 128;
		eg_md.pkt_type = PKT_TYPE_MIRROR;
		eg_dprsr_md.mirror_type = MIRROR_TYPE_E2E;
		eg_dprsr_md.mirror_io_select = 1;
	}

	apply {
		if (hdr.netcache.isValid()) {
			eg_dprsr_md.drop_ctl = 1;
			if (hdr.netcache.op == READ_QUERY) {
				eg_dprsr_md.drop_ctl = 0;
				sampl_check();
				if (sampl_cur == 1) {
					if (hdr.meta.cache_hit == 1) {
						// Update cache counter.
						key_count_incr();
					} else {
						// Update cm sketch.
						cm.apply(hdr, cm_result);
						// Check cm result against threshold (HH_THRES).
						if (cm_result[31:7] != 0) {
							// Check against bloom filter.
							bloom.apply(hdr, bloom_result);
							// If confirmed HH, inform the controller through mirroring.
							if (bloom_result == 0) {
								hdr.netcache.val = cm_result;
								set_mirror();
							}
						}
					}
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Instantiation
// ---------------------------------------------------------------------------

Pipeline(SwitchIngressParser(),
		SwitchIngress(),
		SwitchIngressDeparser(),
		SwitchEgressParser(),
		SwitchEgress(),
		SwitchEgressDeparser()) pipe;

Switch(pipe) main;
