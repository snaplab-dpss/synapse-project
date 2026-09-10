# Capping compute per gress to force the chain across more passes

**Question.** SmartCookie's plan puts far more of the HalfSipHash chain in one ingress pass than the
ground truth does, and the values live at the pass boundary are what the PHV allocator runs out of.
`max_compute_ops_per_gress` already exists in the target config, left effectively off at 1000000.
Turning it on should force the search to split the chain across more passes.

`configs/tofino2-smartcookie.toml` records what a previous attempt found:

> Measured on SmartCookie by cutting the chain down until it built: half of it fits in the ingress,
> two thirds does not, so the real figure is near 120.
>
> Left effectively off, because enforcing it does not currently pay: at 120 the search meets it by
> offloading to the controller (1 Kpps against 286 Mpps), and at 200 the crossing is rarely legal
> where the budget bites, so it recirculates instead.

That objection is about *throughput*. This experiment asked a different question -- does the capped
plan **compile** -- for which a throughput regression would have been an acceptable price.

**Method.** `tofino2-sc-cap{120,200,300}.toml` are `configs/tofino2-smartcookie.toml` with only
`max_compute_ops_per_gress` changed.

    synapse --in bdds/smartcookie.bdd --config tofino/experiments/compute-cap/tofino2-sc-cap200.toml \
            --heuristic max-tput --profile profiles/smartcookie-f40000-c0-unif.json

## Result: the search does not survive the cap

At 200, killed after 11:41 against a ~5 minute baseline:

| | baseline | capped at 200 |
|---|---|---|
| search space estimate | ~5.6e12 | **8.60e+71** |
| unfinished EPs | -- | 7'765, still climbing |
| finished EPs | 1 | **0** |

The cap turns every compute step into a branch point -- continue in this pass, or recirculate --
so the space multiplies by roughly two per op over a chain that is a hundred ops long. It is the
same failure the parent study records for offering the egress crossing everywhere, which took the
estimate to 2.3e61 and made the winner *worse*.

**So the cap is not the lever.** A budget that is only ever enforced by adding a branch cannot
shrink the plan; it has to be something the score prefers, not something the placer refuses. That
is the live-state cost term `tofino/exp-compute/GROUND-TRUTH.md` ends on:

> The plan should prefer pass boundaries where few values are live, and nothing in the score
> expresses that today.

See `../rebalance` for that idea applied by hand to a finished plan, which is the cheaper way to
check it before building it into the search.

The three configs are kept so the measurement can be repeated; none of them is meant to be used.
