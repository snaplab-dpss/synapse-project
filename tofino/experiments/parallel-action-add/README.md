# Why `hash.get(a + b + salt)` does not compile, and what does

**Question.** Porting Meta4 (`tofino/meta4`) to bf-p4c 9.13.4, all four of its hash calls are
rejected:

```p4
hash_1.get(headers.ipv4.src + headers.ipv4.dst + 32w134140211)
```

> `error: add: action spanning multiple stages. Operations on operand 2 ($tmp4[0..31]) in action`
> `toy30 require multiple stages for a single action.`

The sum must be preserved bit-for-bit (the salts are load-bearing — see `tofino/meta4/README.md`),
so the question is the minimal rewrite that keeps the hashed value identical.

**Method.** `toy.p4` is one small t2na program with `-DTEST=n` selecting each candidate shape.
Compile with:

```
bf-p4c --target tofino2 --arch t2na -DTEST=n -o /tmp/toy$n toy.p4
```

## Results

| # | Shape | |
|---|-------|---|
| 1 | `hash.get(a + b + salt)` — as upstream | FAILS |
| 2 | `base = a+b;` then `hash.get(base + salt)` | FAILS |
| 3 | as 2, with a table applied between | FAILS |
| 4 | `base = a+b;` table; `in1 = base+salt;` `hash.get(in1)` | FAILS |
| 5 | `base = a + b` alone, no hash | compiles |
| 6 | `base = a + const` alone | compiles |
| 7 | `hash.get(a)` — hash, no arithmetic | compiles |
| 8 | `base = a+b;` table; `hash.get(base)` | compiles |
| 9 | two adds, a table after each, then hash | FAILS |
| 10 | two adds together, table, then hash | FAILS |
| 11 | **two chained adds, no hash anywhere** | **FAILS** |
| 12 | as 11 with a table between the adds | FAILS |
| 13 | reassociated `(a+salt)+b`, tables between | FAILS |
| 14 | `@in_hash { in1 = a+b+salt; }` then hash | FAILS |
| 15 | the sum inside a hash field list | FAILS |
| 16 | as 4, both hashes used | FAILS |
| 17 | **one add per named table action, then hash** | **compiles** |
| 18 | `@in_hash` inside a real table action | FAILS |
| 19 | base action + both salted sums in one action | compiles |
| 20 | **base action + salt action called directly, no tables** | **compiles** |
| 21 | base in a table action, sums in the apply block | compiles |
| 22 | **salt in place (`x = x + k`), no base field** | **compiles** |

## Takeaways

**It is not about the hash.** Test 11 has no hash at all and fails identically. The rule is about
arithmetic, and the hash call was only where the arithmetic happened to be written.

**One action gets one ALU operation per destination, because a Tofino action is parallel.** From
`VerifyParallelWritesAndReads` in p4c's `mau/instruction_selection.cpp`:

> The purpose of this pass is to verify that an action is able to be performed in parallel.
> Because the semantics of P4 are that instructions are sequential, and the actions in Tofino are
> parallel, this guarantees that an action is possible as a single action.

So `a + b + salt` is two `add`s where the second reads what the first wrote — meaningless in a
parallel action. Note the check's exemption, `if (instruction->name != "set")`: a plain **copy**
may read a value written in the same action (`BackendCopyPropagation` handles it), which is why
bare assignments fold silently but arithmetic does not.

**Self-modification is fine** (test 22). `x = x + k` is one instruction reading the old `x`, which
is exactly the parallel semantics. This saves a metadata field: build the base sum into the
destination, then salt it in place.

**A table boundary does not separate two adds** (tests 3, 9, 12). Copy propagation folds the
first add's destination into the second statement across an intervening `apply()`, recreating the
two-dependent-adds action. Only a **named action** stops it (tests 17, 19, 20, 22) — and it need
not be a table's action, a direct action call from the apply block is enough (test 20).

**The hash unit cannot do the add** (tests 14, 15, 18). `@in_hash` routes an expression through
the IXBar, gated on `CanBeIXBarExpr` (`mau/ixbar_expr.h`), which accepts `Slice`, `Concat`,
`Cast`, `SignExtend`, `ReinterpretCast`, `BXor`, and `BAnd`/`BOr` against a constant — then:

```cpp
// any other expression cannot be an ixbar expression
bool preorder(const IR::Expression *) { return rv = false; }
```

`IR::Add` hits that catch-all. The unit is a GF(2) matrix: XOR is free there, a carrying `+`
never is.

**Not a Tofino 1 → Tofino 2 difference.** Test 1 fails and test 20 compiles on `--target tofino`
too. This is bf-p4c 9.13.4 enforcing what 9.2.0 (which Meta4 was written against) did not.

## Applied to Meta4

`tofino/meta4/p4/meta4.p4` uses the test-20 + test-22 shape: `set_hash_base_{dns,ip}` writes the
address sum into both `ig_md.hash_in_1` and `ig_md.hash_in_2`, `salt_hash_inputs` adds each salt
in place, and both are called directly from the apply block. The hashes then take a plain field,
holding exactly the sum that used to be written inline. It costs no resources — with the adds
removed entirely the program places identically.
