#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "includes/headers.p4"
#include "includes/util.p4"

#include "includes/controls/cached_table.p4"
#include "includes/controls/hashing.p4"

parser SwitchIngressParser(
    packet_in pkt,

    /* User */
    out header_t  hdr,
    out ig_meta_t ig_md,

    /* Intrinsic */
    out ingress_intrinsic_metadata_t ig_intr_md
) {
    TofinoIngressParser() tofino_parser;

    state start {
        tofino_parser.apply(pkt, ig_intr_md);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition parse_app;
    }

    state parse_app {
        pkt.extract(hdr.app);
        transition accept;
    }
}

control SwitchIngress(
    /* User */
    inout header_t  hdr,
    inout ig_meta_t ig_md,

    /* Intrinsic */ 
    in ingress_intrinsic_metadata_t                 ig_intr_md,
    in ingress_intrinsic_metadata_from_parser_t     ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t       ig_tm_md
) {
    Hashing() hashing;
    CachedTable() cached_table;

    bool hit;

    action forward(dev_port_t port_id) {
        ig_tm_md.ucast_egress_port = port_id;
    }

    action send_to_cpu(){
        forward(CPU_PCIE_PORT);
    }

    apply {
        hashing.apply(hdr.app.key, hdr.app.hash);

        cached_table.apply(
            ig_intr_md.ingress_mac_tstamp[47:16],
            hdr.app.op,
            hdr.app.hash,
            hdr.app.key,
            hdr.app.value,
            hit
        );

        if (hit) {
            hdr.app.hit = 1;
        } else {
            hdr.app.hit = 0;
        }

#if __TARGET_TOFINO__ == 2
        forward(9);
#else
        forward(1);
#endif
    }
}

control SwitchIngressDeparser(
    packet_out pkt,

    /* User */
    inout header_t  hdr,
    in    ig_meta_t ig_md,

    /* Intrinsic */
    in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
    apply {
        pkt.emit(hdr);
    }
}

parser SwitchEgressParser(
    packet_in pkt,

    /* User */
    out empty_header_t   hdr,
    out empty_metadata_t meta,

    /* Intrinsic */
    out egress_intrinsic_metadata_t eg_intr_md
) {
    TofinoEgressParser() tofino_parser;

    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        tofino_parser.apply(pkt, eg_intr_md);
        transition accept;
    }
}

control SwitchEgress(
    /* User */
    inout empty_header_t   hdr,
    inout empty_metadata_t meta,

    /* Intrinsic */ 
    in    egress_intrinsic_metadata_t                 eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t     eg_intr_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t    eg_intr_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
    apply {}
}

control SwitchEgressDeparser(
    packet_out pkt,

    /* User */
    inout empty_header_t   hdr,
    in    empty_metadata_t meta,

    /* Intrinsic */
    in egress_intrinsic_metadata_for_deparser_t eg_intr_dprsr_md
) {
    apply {
        pkt.emit(hdr);
    }
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         SwitchEgressParser(),
         SwitchEgress(),
         SwitchEgressDeparser()) pipe;

Switch(pipe) main;
