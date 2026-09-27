#!/usr/bin/env python3
# Generates the two variants of the DNS name parse (see README.md): dns_per_byte.p4 and dns_per_len.p4.

import sys

LABELS = 4
PIECES = [1, 2, 4, 8]  # wire order of the pieces a label is extracted in, by the bits of its length
MAX_LEN = 15
DOMAINS = 2048

COMMON_HEAD = """#include <core.p4>
#include <t2na.p4>

header ethernet_t { bit<48> dst; bit<48> src; bit<16> type; }
header ipv4_t {
  bit<4> version; bit<4> ihl; bit<8> tos; bit<16> len; bit<16> id; bit<3> flags; bit<13> frag;
  bit<8> ttl; bit<8> proto; bit<16> csum; bit<32> src; bit<32> dst;
}
header udp_t { bit<16> sport; bit<16> dport; bit<16> len; bit<16> csum; }
header dns_t { bit<16> id; bit<16> flags; bit<16> qd; bit<16> an; bit<16> ns; bit<16> ar; }
header dns_len_t { bit<8> v; }
header dns_b_t { bit<8> v; }
"""


def gen(variant: str) -> str:
    out = [COMMON_HEAD]
    if variant == "per_len":
        for k in range(1, MAX_LEN + 1):
            out.append(f"header dns_h{k}_t {{ bit<{8 * k}> v; }}\n")
    if variant in ("per_piece", "per_piece_staged", "per_piece_reversed"):
        for p in PIECES:
            out.append(f"header dns_c{p}_t {{ bit<{8 * p}> v; }}\n")

    out.append("\nstruct headers_t {\n  ethernet_t eth; ipv4_t ipv4; udp_t udp; dns_t dns;\n")
    for i in range(LABELS):
        out.append(f"  dns_len_t l{i}_len;\n")
        if variant == "per_byte":
            for m in range(MAX_LEN):
                out.append(f"  dns_b_t l{i}_b{m};\n")
        elif variant == "per_len":
            for k in range(1, MAX_LEN + 1):
                out.append(f"  dns_h{k}_t l{i}_h{k};\n")
        else:
            for p in PIECES:
                out.append(f"  dns_c{p}_t l{i}_c{p};\n")
    out.append(f"  dns_len_t l{LABELS}_len;\n}}\n")

    out.append("struct metadata_t { bit<8> count; }\n")

    # ---------------- parser
    out.append("""
parser IngressParser(packet_in pkt, out headers_t hdr, out metadata_t meta, out ingress_intrinsic_metadata_t ig_intr_md) {
  state start {
    pkt.extract(ig_intr_md);
    pkt.advance(PORT_METADATA_SIZE);
    transition parse_eth;
  }
  state parse_eth { pkt.extract(hdr.eth); transition select(hdr.eth.type) { 0x800: parse_ipv4; default: accept; } }
  state parse_ipv4 { pkt.extract(hdr.ipv4); transition select(hdr.ipv4.proto) { 17: parse_udp; default: accept; } }
  state parse_udp { pkt.extract(hdr.udp); transition select(hdr.udp.sport) { 53: parse_dns; default: accept; } }
  state parse_dns {
    pkt.extract(hdr.dns);
    meta.count = 0;
""")
    if variant == "per_byte":
        # A label shorter than 15 bytes leaves its unextracted byte fields untouched; the key copies all 15, so they are zeroed here.
        for i in range(LABELS):
            for m in range(MAX_LEN):
                out.append(f"    hdr.l{i}_b{m}.v = 0;\n")
    out.append("    transition parse_l0;\n  }\n")

    for i in range(LABELS):
        nxt = f"parse_l{i + 1}" if i + 1 < LABELS else "parse_terminator"
        out.append(f"  state parse_l{i} {{\n    pkt.extract(hdr.l{i}_len);\n    transition select(hdr.l{i}_len.v) {{\n      0: end_{i};\n")
        for k in range(1, MAX_LEN + 1):
            out.append(f"      {k}: parse_l{i}_len{k};\n")
        out.append("      default: accept;\n    }\n  }\n")
        for k in range(1, MAX_LEN + 1):
            out.append(f"  state parse_l{i}_len{k} {{\n")
            if variant == "per_byte":
                for m in range(k):
                    out.append(f"    pkt.extract(hdr.l{i}_b{m});\n")
            elif variant == "per_len":
                out.append(f"    pkt.extract(hdr.l{i}_h{k});\n")
            else:
                for p in PIECES:
                    if k & p:
                        out.append(f"    pkt.extract(hdr.l{i}_c{p});\n")
            out.append(f"    transition {nxt};\n  }}\n")
    out.append(f"  state parse_terminator {{\n    pkt.extract(hdr.l{LABELS}_len);\n    transition select(hdr.l{LABELS}_len.v) {{ 0: end_{LABELS}; default: accept; }}\n  }}\n")
    for n in range(LABELS + 1):
        out.append(f"  state end_{n} {{ meta.count = {n}; transition accept; }}\n")
    out.append("}\n")

    # ---------------- ingress
    out.append("""
control Ingress(inout headers_t hdr, inout metadata_t meta, in ingress_intrinsic_metadata_t ig_intr_md,
                in ingress_intrinsic_metadata_from_parser_t ig_prsr_md, inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
                inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
""")
    # The C key: count, then per position j (top-level label first): len + 15 bytes.
    WORDS = [32, 32, 32, 24]
    for j in range(LABELS):
        out.append(f"  bit<8> k{j}_len = 0;\n")
        if variant == "per_piece_reversed":
            for p in PIECES:
                out.append(f"  bit<{8 * p}> k{j}_c{p} = 0;\n")
        elif variant == "per_piece_staged":
            for n, w in enumerate(WORDS):
                out.append(f"  bit<{w}> k{j}_w{n} = 0;\n")
        else:
            for m in range(MAX_LEN):
                out.append(f"  bit<8> k{j}_b{m} = 0;\n")

    if variant == "per_piece_reversed":
        # The key keeps the pieces as the parser laid them out; only the label order is the NF's.
        for n in range(1, LABELS + 1):
            out.append(f"  action build_{n}() {{\n")
            for i in range(n):
                j = n - 1 - i
                out.append(f"    k{j}_len = hdr.l{i}_len.v;\n")
                for p in PIECES:
                    out.append(f"    k{j}_c{p} = hdr.l{i}_c{p}.v;\n")
            out.append("  }\n")
        out.append("  table build_name {\n    key = { meta.count: exact; }\n    actions = {" + "".join(f" build_{n};" for n in range(1, LABELS + 1)) + " }\n    const entries = {\n")
        for n in range(1, LABELS + 1):
            out.append(f"      {n}: build_{n}();\n")
        out.append(f"    }}\n    size = {LABELS};\n  }}\n")

    if variant == "per_piece_staged":
        # The length byte of position j, by count.
        for n in range(1, LABELS + 1):
            out.append(f"  action lens_{n}() {{\n")
            for i in range(n):
                out.append(f"    k{n - 1 - i}_len = hdr.l{i}_len.v;\n")
            out.append("  }\n")
        out.append("  table lens {\n    key = { meta.count: exact; }\n    actions = {" + "".join(f" lens_{n};" for n in range(1, LABELS + 1)) + " }\n    const entries = {\n")
        for n in range(1, LABELS + 1):
            out.append(f"      {n}: lens_{n}();\n")
        out.append(f"    }}\n    size = {LABELS};\n  }}\n")

        # Piece p of label i lands at character offset o = len & (p - 1) of position j; the bytes o..o+p-1 as slices of the words.
        def word_slices(o, p):
            slices = []
            b = o
            while b < o + p:
                n = b // 4 if b < 12 else 3
                base = 4 * n
                width = 4 if n < 3 else 3
                first = b - base
                last = min(width - 1, o + p - 1 - base)
                slices.append((n, width, first, last, b - o))
                b = base + last + 1
            return slices

        for p in PIECES:
            for j in range(LABELS):
                for i in range(LABELS - j):
                    for o in range(p):
                        out.append(f"  action put{p}_{j}_from_{i}_at{o}() {{\n")
                        for n, width, first, last, src_b in word_slices(o, p):
                            dhi = 8 * width - 1 - 8 * first
                            dlo = 8 * width - 8 * (last + 1)
                            shi = 8 * p - 1 - 8 * src_b
                            slo = shi - (dhi - dlo)
                            src = f"hdr.l{i}_c{p}.v" if (p == 1) else f"hdr.l{i}_c{p}.v[{shi}:{slo}]"
                            dst = f"k{j}_w{n}" if (dhi - dlo + 1 == 8 * width) else f"k{j}_w{n}[{dhi}:{dlo}]"
                            out.append(f"    {dst} = {src};\n")
                        out.append("  }\n")
                out.append(f"  table put{p}_{j} {{\n    key = {{ meta.count: ternary;")
                for i in range(LABELS):
                    out.append(f" hdr.l{i}_len.v: ternary;")
                out.append(" }\n    actions = {")
                for i in range(LABELS - j):
                    for o in range(p):
                        out.append(f" put{p}_{j}_from_{i}_at{o};")
                out.append(" }\n    const entries = {\n")
                entries = 0
                for i in range(LABELS - j):
                    n = i + j + 1
                    for k in range(1, MAX_LEN + 1):
                        if not (k & p):
                            continue
                        o = k & (p - 1)
                        lens = ", ".join(f"8w{k} &&& 8w0xff" if ii == i else "8w0 &&& 8w0" for ii in range(LABELS))
                        out.append(f"      (8w{n} &&& 8w0xff, {lens}): put{p}_{j}_from_{i}_at{o}();\n")
                        entries += 1
                out.append(f"    }}\n    size = {entries};\n  }}\n")

    if variant == "per_byte":
        for n in range(1, LABELS + 1):
            out.append(f"  action build_{n}() {{\n")
            for i in range(n):
                j = n - 1 - i
                out.append(f"    k{j}_len = hdr.l{i}_len.v;\n")
                for m in range(MAX_LEN):
                    out.append(f"    k{j}_b{m} = hdr.l{i}_b{m}.v;\n")
            out.append("  }\n")
        out.append("  table build_name {\n    key = { meta.count: exact; }\n    actions = {")
        out.append("".join(f" build_{n};" for n in range(1, LABELS + 1)))
        out.append(" }\n    const entries = {\n")
        for n in range(1, LABELS + 1):
            out.append(f"      {n}: build_{n}();\n")
        out.append(f"    }}\n    size = {LABELS};\n  }}\n")
    elif variant in ("per_len", "per_piece"):
        # One table per key position j: which label i fills it depends on the count, and the header holding that label on its length.
        for j in range(LABELS):
            for i in range(LABELS - j):
                for k in range(1, MAX_LEN + 1):
                    out.append(f"  action fill_{j}_from_{i}_len{k}() {{\n    k{j}_len = hdr.l{i}_len.v;\n")
                    if variant == "per_len":
                        for m in range(k):
                            hi = 8 * k - 1 - 8 * m
                            out.append(f"    k{j}_b{m} = hdr.l{i}_h{k}.v[{hi}:{hi - 7}];\n")
                    else:
                        # The label's bytes, in order, are the bytes of the pieces its length selects, in wire order.
                        m = 0
                        for p in PIECES:
                            if k & p:
                                for q in range(p):
                                    hi = 8 * p - 1 - 8 * q
                                    src = f"hdr.l{i}_c{p}.v" if p == 1 else f"hdr.l{i}_c{p}.v[{hi}:{hi - 7}]"
                                    out.append(f"    k{j}_b{m} = {src};\n")
                                    m += 1
                    out.append("  }\n")
            out.append(f"  table fill_{j} {{\n    key = {{ meta.count: ternary;")
            for i in range(LABELS):
                out.append(f" hdr.l{i}_len.v: ternary;")
            out.append(" }\n    actions = {")
            for i in range(LABELS - j):
                for k in range(1, MAX_LEN + 1):
                    out.append(f" fill_{j}_from_{i}_len{k};")
            out.append(" }\n    const entries = {\n")
            for i in range(LABELS - j):
                n = i + j + 1
                for k in range(1, MAX_LEN + 1):
                    lens = ", ".join(f"8w{k} &&& 8w0xff" if ii == i else "8w0 &&& 8w0" for ii in range(LABELS))
                    out.append(f"      (8w{n} &&& 8w0xff, {lens}): fill_{j}_from_{i}_len{k}();\n")
            out.append(f"    }}\n    size = {(LABELS - j) * MAX_LEN};\n  }}\n")

    out.append("  action set_domain(bit<32> id) { hdr.dns.id = id[15:0]; }\n  table known_domains {\n    key = {\n      meta.count: ternary;\n")
    for j in range(LABELS):
        out.append(f"      k{j}_len: ternary;\n")
        if variant == "per_piece_staged":
            for n in range(len(WORDS)):
                out.append(f"      k{j}_w{n}: ternary;\n")
        elif variant == "per_piece_reversed":
            for p in PIECES:
                out.append(f"      k{j}_c{p}: ternary;\n")
        else:
            for m in range(MAX_LEN):
                out.append(f"      k{j}_b{m}: ternary;\n")
    out.append(f"    }}\n    actions = {{ set_domain; }}\n    size = {DOMAINS};\n  }}\n")
    out.append("  apply {\n    ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;\n    if (hdr.dns.isValid()) {\n")
    if variant in ("per_byte", "per_piece_reversed"):
        out.append("      build_name.apply();\n")
    elif variant == "per_piece_staged":
        out.append("      lens.apply();\n")
        for p in PIECES:
            for j in range(LABELS):
                out.append(f"      put{p}_{j}.apply();\n")
    else:
        for j in range(LABELS):
            out.append(f"      fill_{j}.apply();\n")
    out.append("      known_domains.apply();\n    }\n  }\n}\n")

    out.append("""
control IngressDeparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
  apply { pkt.emit(hdr); }
}
struct eg_headers_t {}
struct eg_metadata_t {}
parser EgressParser(packet_in pkt, out eg_headers_t hdr, out eg_metadata_t md, out egress_intrinsic_metadata_t eg_intr_md) {
  state start { pkt.extract(eg_intr_md); transition accept; }
}
control Egress(inout eg_headers_t hdr, inout eg_metadata_t md, in egress_intrinsic_metadata_t eg_intr_md,
               in egress_intrinsic_metadata_from_parser_t eg_prsr_md, inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
               inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) {
  apply {}
}
control EgressDeparser(packet_out pkt, inout eg_headers_t hdr, in eg_metadata_t md, in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {
  apply { pkt.emit(hdr); }
}
Pipeline(IngressParser(), Ingress(), IngressDeparser(), EgressParser(), Egress(), EgressDeparser()) pipe;
Switch(pipe) main;
""")
    return "".join(out)


for variant in ("per_byte", "per_len", "per_piece", "per_piece_staged", "per_piece_reversed"):
    with open(f"dns_{variant}.p4", "w") as f:
        f.write(gen(variant))
