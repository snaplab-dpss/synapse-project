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

	Register<bit<NC_VALUE_WIDTH>, bit<16>>(NC_ENTRIES) reg_vtable;

	RegisterAction<_, bit<16>, bit<NC_VALUE_WIDTH>>(reg_vtable) ract_vtable_read = {
		void apply(inout bit<NC_VALUE_WIDTH> value, out bit<NC_VALUE_WIDTH> res) {
            res = value;
		}
	};

	RegisterAction<_, bit<16>, bit<NC_VALUE_WIDTH>>(reg_vtable) ract_vtable_update = {
		void apply(inout bit<NC_VALUE_WIDTH> value) {
            if (hdr.netcache.op == WRITE_QUERY) {
                value = hdr.netcache.value;
            } else {
                value = 0;
            }
		}
	};

    action set_lookup_metadata(vtableIdx_t vt_idx, keyIdx_t key_idx) {
        hdr.meta.vt_idx = vt_idx;
        hdr.meta.key_idx = key_idx;
	}

	action vtable_read() {
       hdr.netcache.value = ract_vtable_read.execute(hdr.meta.vt_idx);
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
			hdr.ethernet.dst_addr : exact;
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
                    // Assume no WRITE/UPDATE validity check, immediately access the cached value.
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
                // Cache hit
                if (hdr.meta.cache_hit == 1) {
                    // Update/delete the cached value with the received value.
                    vtable_update();
                }
            }
        }
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

    apply {
        if (hdr.netcache.isValid()) {
            if (hdr.netcache.op == READ_QUERY) {
                // Cache hit
                if (hdr.meta.cache_hit == 1) {
                    // Update cache counter.
                } else {
                    // Update cm sketch.
                    // If cm cntr > HH_THRESHOLD:
                    //     Check against bloom filter.
                    //     If confirmed HH:
                    //         Update OP to HOT_READ, to inform the server.
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
