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
	Register<bit<32>, _>(NC_ENTRIES) reg_v;

	RegisterAction<bit<32>, bit<16>, bit<32>>(reg_v) read_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<16>, bit<8>>(reg_v) update_v = {
		void apply(inout bit<32> val) {
			if (hdr.netcache.op == WRITE_QUERY) {
				val = hdr.netcache.val;
			} else {
				val = 0;
			}
		}
	};

	action set_key_idx(bit<16> key_idx) {
		ig_md.key_idx = key_idx;
		hdr.netcache.status = NC_STATUS_HIT;
	}

	table keys {
		key = {
			hdr.netcache.key : exact;
		}
		actions = {
			set_key_idx;
		}

		size = NC_ENTRIES;
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

	action set_out_port(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action set_client_packet() {
		ig_md.is_client_packet = 1;
	}

	action set_not_client_packet() {
		ig_md.is_client_packet = 0;
	}

	table is_client_packet {
		key = {
			ig_intr_md.ingress_port: exact;
		}
		actions = {
			set_client_packet;
			set_not_client_packet;
		}

		const default_action = set_client_packet;
		size = 2;
	}

	table fwd {
		key = {
			ig_intr_md.ingress_port: exact;
			ig_md.cache_hit: exact;
			ig_md.original_nc_port: ternary;
		}

		actions = {
			set_out_port;
		}

		size = 1024;
	}

	c_cm()      cm;
	c_bloom()   bloom;

	Register<bit<8>, bit<1>>(1) reg_sampl;
	Register<bit<32>, bit<NC_KEY_WIDTH>>(NC_ENTRIES) reg_key_count;

	bit<8>  sampl_cur;
	bit<16> cm_result;
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
		ract_key_count_incr.execute(ig_md.key_idx);
	}

	action set_normal_pkt() {
		hdr.bridged_md.setValid();
		hdr.bridged_md.pkt_type = PKT_TYPE_NORMAL;
	}

	action set_mirror_pkt() {
		ig_dprsr_md.mirror_type = MIRROR_TYPE_I2E;
		ig_md.pkt_type = PKT_TYPE_MIRROR;
		ig_md.mirror_session = 128;
	}

	apply {
		ig_md.original_nc_port = hdr.netcache.port;
		hdr.netcache.port = (bit<16>)ig_intr_md.ingress_port;

		is_client_packet.apply();
		bool cache_hit = keys.apply().hit;

		if (cache_hit) {
			ig_md.cache_hit = 1;
		} else {
			ig_md.cache_hit = 0;
		}

		// Check if packet is not a HH report going from/to controller<->server.
		if (ig_md.is_client_packet == 1 && ig_md.cache_hit == 1) {
			if (hdr.netcache.op == READ_QUERY) {
				// Read the cached value and update the packet header.
				hdr.netcache.val = read_v.execute(ig_md.key_idx);
			} else {
				update_v.execute(ig_md.key_idx);
			}

			// Swap the IP src/dst and port src/dst.
			update_pkt_udp();
		}

		// Packet is a HH report going from/to controller<->server
		if (ig_md.is_client_packet == 1) {
			sampl_check();
			if (sampl_cur == 1) {
				if (ig_md.cache_hit == 1) {
					// Update cache counter.
					key_count_incr();
				} else {
					// Update cm sketch.
					cm.apply(hdr, cm_result);
					// Check cm result against threshold (HH_THRES).
					if (cm_result > HH_THRES) {
						// Check against bloom filter.
						bloom.apply(hdr, bloom_result);
						// If confirmed HH, inform the controller through mirroring.
						if (bloom_result == 0) {
							// FIXME: we should get these values from the control plane.
							// Otherwise, we lose the PUT value.
							hdr.netcache.val = (bit<NC_VAL_WIDTH>)cm_result;
							set_mirror_pkt();
						}
					}
				}
			}
		}

		fwd.apply();
		set_normal_pkt();
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
	
	apply {
		if (hdr.bridged_md.isValid()) {
			hdr.bridged_md.setInvalid();
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
