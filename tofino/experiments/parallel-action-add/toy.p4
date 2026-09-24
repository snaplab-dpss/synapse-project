#include <core.p4>
#include <t2na.p4>

header eth_h { bit<48> dst; bit<48> src; bit<16> etype; }
header ip_h  { bit<8> a; bit<8> b; bit<16> c; bit<32> src; bit<32> dst; }
struct hdr_t { eth_h eth; ip_h ip; }
struct md_t  { bit<32> base; bit<32> in1; bit<32> idx1; bit<32> idx2; }

parser IPrs(packet_in pkt, out hdr_t h, out md_t m,
            out ingress_intrinsic_metadata_t ig_md) {
    state start { pkt.extract(ig_md); pkt.advance(PORT_METADATA_SIZE);
                  m.base = 0; m.in1 = 0; m.idx1 = 0; m.idx2 = 0;
                  pkt.extract(h.eth); pkt.extract(h.ip); transition accept; }
}
control IDpr(packet_out pkt, inout hdr_t h, in md_t m,
             in ingress_intrinsic_metadata_for_deparser_t d) {
    apply { pkt.emit(h); }
}
control Ing(inout hdr_t h, inout md_t m,
            in ingress_intrinsic_metadata_t ig_intr_md,
            in ingress_intrinsic_metadata_from_parser_t p,
            inout ingress_intrinsic_metadata_for_deparser_t d,
            inout ingress_intrinsic_metadata_for_tm_t tm) {
    Hash<bit<16>>(HashAlgorithm_t.CRC32) hsh;
    Hash<bit<16>>(HashAlgorithm_t.CRC32) hsh2;
    action nop() {}
    action mk_base()  { m.base = h.ip.src + h.ip.dst; }
    action mk_in1()   { m.in1  = m.base + 32w134140211; }
    action mk_both()  { m.in1  = m.base + 32w134140211; m.idx2 = m.base + 32w187182238; }
    action inhash()   { @in_hash { m.in1 = h.ip.src + h.ip.dst + 32w134140211; } }
    table t1 { actions = { mk_base; }  size = 1; default_action = mk_base; }
    table t2 { actions = { mk_in1; }   size = 1; default_action = mk_in1; }
    table t3 { actions = { mk_both; }  size = 1; default_action = mk_both; }
    table t4 { actions = { inhash; }   size = 1; default_action = inhash; }
    action mk_two()   { m.in1 = h.ip.src + h.ip.dst; m.idx2 = h.ip.src + h.ip.dst; }
    action salt_inplace() { m.in1 = m.in1 + 32w134140211; m.idx2 = m.idx2 + 32w187182238; }
    table sep { key = { h.ip.a : exact; } actions = { nop; } size = 2; default_action = nop; }
    table sep2 { key = { h.ip.b : exact; } actions = { nop; } size = 2; default_action = nop; }
    apply {
#if TEST == 1
        m.idx1 = (bit<32>) hsh.get(h.ip.src + h.ip.dst + 32w134140211);   // two adds in the call
#elif TEST == 2
        m.base = h.ip.src + h.ip.dst;
        m.idx1 = (bit<32>) hsh.get(m.base + 32w134140211);               // one add in the call
#elif TEST == 3
        m.base = h.ip.src + h.ip.dst;
        sep.apply();
        m.idx1 = (bit<32>) hsh.get(m.base + 32w134140211);               // one add, after a table
#elif TEST == 4
        m.base = h.ip.src + h.ip.dst;
        sep.apply();
        m.in1 = m.base + 32w134140211;
        m.idx1 = (bit<32>) hsh.get(m.in1);                               // no arithmetic in the call
#elif TEST == 5
        m.base = h.ip.src + h.ip.dst;                                    // just one add, no hash
#elif TEST == 6
        m.base = h.ip.src + 32w134140211;                                // add of field + const
#elif TEST == 7
        m.idx1 = (bit<32>) hsh.get(h.ip.src);                            // hash, no arithmetic
#elif TEST == 8
        m.base = h.ip.src + h.ip.dst;
        sep.apply();
        m.idx1 = (bit<32>) hsh.get(m.base);                              // add then hash, separated
#elif TEST == 9
        m.base = h.ip.src + h.ip.dst;
        sep.apply();
        m.in1 = m.base + 32w134140211;
        sep2.apply();
        m.idx1 = (bit<32>) hsh.get(m.in1);            // two adds, each with a boundary
#elif TEST == 10
        m.base = h.ip.src + h.ip.dst;
        m.in1 = m.base + 32w134140211;
        sep.apply();
        m.idx1 = (bit<32>) hsh.get(m.in1);            // two adds together, boundary before hash
#elif TEST == 11
        m.base = h.ip.src + h.ip.dst;
        m.in1  = m.base + 32w134140211;               // chained add, NO hash anywhere
        m.idx1 = m.in1;
#elif TEST == 12
        m.base = h.ip.src + h.ip.dst;
        sep.apply();
        m.in1  = m.base + 32w134140211;               // chained add over a boundary, no hash
        m.idx1 = m.in1;
#elif TEST == 13
        m.base = h.ip.src + 32w134140211;             // reassociated: (src+salt)+dst
        sep.apply();
        m.in1  = m.base + h.ip.dst;
        sep2.apply();
        m.idx1 = (bit<32>) hsh.get(m.in1);
#elif TEST == 14
        @in_hash { m.in1 = h.ip.src + h.ip.dst + 32w134140211; }
        m.idx1 = (bit<32>) hsh.get(m.in1);            // sum computed in the hash unit
#elif TEST == 15
        m.idx1 = (bit<32>) hsh.get({ h.ip.src + h.ip.dst + 32w134140211 });  // as a field list
#elif TEST == 16
        m.base = h.ip.src + h.ip.dst;
        sep.apply();
        m.in1  = m.base + 32w134140211;
        m.idx1 = (bit<32>) hsh.get(m.in1);
        m.idx2 = (bit<32>) hsh.get(m.base);           // same as 4 but both hashes used
#elif TEST == 17
        t1.apply(); t2.apply();                       // one add per table action
        m.idx1 = (bit<32>) hsh.get(m.in1);
#elif TEST == 18
        t4.apply();                                   // @in_hash inside a real action
        m.idx1 = (bit<32>) hsh.get(m.in1);
#elif TEST == 19
        t1.apply(); t3.apply();                       // base, then BOTH salted sums in one action
        m.idx1 = (bit<32>) hsh.get(m.in1);
        m.idx2 = (bit<32>) hsh2.get(m.idx2);
#elif TEST == 20
        mk_base(); mk_both();                         // direct action calls, no tables
        m.idx1 = (bit<32>) hsh.get(m.in1);
        m.idx2 = (bit<32>) hsh2.get(m.idx2);
#elif TEST == 21
        t1.apply();                                   // base in a table action, sums in apply
        m.in1  = m.base + 32w134140211;
        m.idx2 = m.base + 32w187182238;
        m.idx1 = (bit<32>) hsh.get(m.in1);
#elif TEST == 22
        mk_two(); salt_inplace();                     // salt in place: x = x + k, no base field
        m.idx1 = (bit<32>) hsh.get(m.in1);
        m.base = (bit<32>) hsh2.get(m.idx2);
#endif
        tm.ucast_egress_port = 1;
    }
}
control Eg(inout hdr_t h, inout md_t m, in egress_intrinsic_metadata_t e,
           in egress_intrinsic_metadata_from_parser_t p,
           inout egress_intrinsic_metadata_for_deparser_t d,
           inout egress_intrinsic_metadata_for_output_port_t o) { apply {} }
parser EPrs(packet_in pkt, out hdr_t h, out md_t m, out egress_intrinsic_metadata_t e) {
    state start { pkt.extract(e); transition accept; }
}
control EDpr(packet_out pkt, inout hdr_t h, in md_t m,
             in egress_intrinsic_metadata_for_deparser_t d) { apply {} }
Pipeline(IPrs(), Ing(), IDpr(), EPrs(), Eg(), EDpr()) pipe;
Switch(pipe) main;
