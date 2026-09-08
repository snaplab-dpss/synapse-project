# Toy P4 experiments for bf-p4c (Tofino 2, SDE 9.13.4)

Small programs compiled to learn how bf-p4c reacts to a code shape before synapse relies on
it. Each file is one question; the answer is what the compiler did. Compile with the SDE's
compiler:

```
bf-p4c --target tofino2 --arch t2na -o out_X X.p4
```

Build directories (`out_*`) and logs are not kept.

## Skeleton

`cmp12.p4`: ethernet + ipv4 parser, a few metadata fields, an `init` action and a forwarding
decision. Every experiment is this file with the actions replaced.

## Header-field concatenation (`concat*.p4`)

Question: synapse's header guess splits the IPv4 source address into 24 + 8 bits, while the
NF hashes it as one 32-bit network-order word. Can the P4 rebuild it with `++`?

- `concatA`: `meta.v = hdr.src_hi ++ hdr.src_lo` as a bare assignment: compiles.
- `concatB`: the same concat used directly as an xor operand: PHV allocation fails.
- `concatC`: byte-reversed concat of slices (the bswap32 macro spelled out): compiles.
- `concatD`: slicing a parenthesised concat, `(a ++ b)[7:0] ++ ...`: compiles.

Takeaway: a field concat must be materialised by its own assignment before any arithmetic
uses it; the existing `bswap32` macro works over a concat.

## Hash rotates, ALU chains and headers (`phvT*.p4`, `phvW*.p4`)

Question: the synthesized SmartCookie P4 fails PHV allocation ("field slices must be packed
together", "unable to slice group") around its `@in_hash` rotates. Which shape triggers it?

- `phvT1..T4`: odd-bit `@in_hash` rotates on metadata chained through xor/add, results and
  inputs copied into a recirculation-style header (T4 also parses that header back and feeds
  its fields to more rotates): all compile.
- `phvW1..W3`: the chain's final value written byte-wise, shifted by one byte, into a 40-bit
  header field (the synthesized cookie write): fail. `phvW5`: the same writes from a plain
  value: compiles. `phvW4`: the chain's value written whole into a 32-bit `hdr.tcp.seq`: fails.
- `phvW6..W8`: W4 without the `>> 12`, or with byte shifts instead: still fail. `phvW9`: the
  shifted value xored with a plain value instead of the chain: compiles.
- `phvW10`: hash inputs read from a dedicated header (expert style): fails. `phvW11`: the
  rotate input computed into its own variable instead of being shared: fails.
- `phvW12`: both rotates in shift form (`x << n`, `x >> (32-n)`, then `|`): compiles.

Takeaway (from bf-p4c's own message on W4): the allocator slices the whole ALU group at the
hash-rotate input's odd bit (`a0[24:0]`, `a0[31:25]`), and a 32-bit add on operands sliced
like that needs four PHV sources where Tofino 2 allows two. Whether that happens depends on
the allocator's choices (T4 passes, W4 fails), so the hash form is fragile for values that
reach a packet header; the shift form avoids odd slicing altogether.

## SmartCookie bisection (`sc/`)

`sc/sc.p4` is the synthesized SmartCookie solution (c0-unif profile) that fails as above;
`sc_churn.p4` the high-churn one (same error). `sc_nohash.p4` drops the `@in_hash`
annotations, `sc_e2.p4` removes the second recirculation path, `sc_e6.p4` moves the
recirculation-header traffic onto metadata: the first two fail identically, the third moves
the error to the cookie's header write. Takeaway: the recirculation header was a symptom; the
odd-slice propagation above is the cause.
