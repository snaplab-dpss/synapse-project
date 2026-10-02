# Multi-value register actions for a HashPipe stage (Tofino 2)

## Question

A HashPipe stage (dpdk-nfs/hhh, `vector_inc_or_swap`) reads a {key, count} cell and either adds
the carried count to it (same key), swaps the carried pair in (lighter cell), or keeps it; what is
NOT in the cell afterwards is carried to the next stage. Can one Tofino 2 register action do this,
and what can it return? bf-p4c 9.13.4, `--target tofino2 --arch t2na`,
`bf-p4c -DVARIANT=<n> salu-multi-return.p4`.

## Variants and results

| variant | predicates | returns | result |
|---|---|---|---|
| 1 | hit, lighter | old pair + a constant outcome code per branch | **rejected**: "cannot output constant from Register ... because the predicate is output in another control flow" |
| 2 | hit, lighter | the carried-out pair, sourced from constants / memory / PHV per branch | **rejected**: "Incompatible outputs in RegisterAction: mem_lo and meta.carried_key" |
| 3 | hit only (first stage, always insert) | old pair + constant outcome | compiles |
| 4 | hit, lighter | old pair + `this.predicate<bit<16>>(hit, lighter)` | compiles |
| 6 | hit, lighter | old pair only | compiles |

## Takeaways

- The extern names are `RegisterAction2<T, H, U1, U2>` / `RegisterAction3<T, H, U1, U2, U3>` with
  `U1 execute(in H index, out U2 rv2[, out U3 rv3])`; `RegisterAction<...>` takes one return value.
- Every return value must have ONE source across all branches: memory (the old cell), or a
  constant, or the predicate -- a value that is a constant in one branch and a memory word in
  another is rejected, and so is a constant output when the predicate differs per branch (v1, v2).
  So the action cannot hand back "what is not in the cell" directly.
- What it CAN return: the old pair (memory, both halves) and `this.predicate<bit<16>>(c0, c1)`,
  a one-hot over the predicate combinations (bit index = c1 << 1 | c0: 1 -> none, 2 -> hit only,
  4 -> lighter only, 8 -> both). With the old pair alone the control can also recompute the
  outcome (old.key == carried.key, old.count < carried.count) in gateways (v6).
- Updating the carried pair after the action takes a gateway on the outcome and two assignments
  (zeros after a hit, the old pair after a swap, unchanged otherwise) -- outside the SALU.
- A struct used as a register value type must be declared at top level, not inside a control.
