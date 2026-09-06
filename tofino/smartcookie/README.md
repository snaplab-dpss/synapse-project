# SmartCookie (expert baseline)

Tofino implementation of the **SmartCookie switch agent**: cryptographically
secure SYN cookies (HalfSipHash-2-4) computed and verified entirely in the
switch data plane, from Princeton's
[p4-projects](https://github.com/Princeton-Cabernet/p4-projects/tree/master/SmartCookie).
We use it as an **expert baseline** to compare against synapse-synthesized
solutions, alongside `tofino/netcache`, `tofino/switcharoo` and
`tofino/hyperloglog`.

Upstream docs are preserved in [`README.upstream.md`](README.upstream.md).

## Credit

**All credit for the SmartCookie design and implementation belongs to its
original authors.** This directory is an adaptation of existing work, not our
own; our contribution is limited to the small portability edits and testbed
adaptations described below.

- **Original authors:** Sophia Yoo, Xiaoqi Chen and Jennifer Rexford,
  Princeton University.
- **Paper:** *SmartCookie: Blocking Large-Scale SYN Floods with a Split-Proxy
  Defense on Programmable Data Planes*, USENIX Security 2024
  ([usenix.org](https://www.usenix.org/conference/usenixsecurity24/presentation/yoo)).
- **Source:** [Princeton-Cabernet/p4-projects](https://github.com/Princeton-Cabernet/p4-projects),
  directory `SmartCookie/`, file `p4src/SmartCookie-HalfSipHash.p4`. If you use
  this code, please cite:

  ```bibtex
  @inproceedings{yoo2024smartcookie,
      title={SmartCookie: Blocking Large-Scale SYN Floods with a Split-Proxy Defense on Programmable Data Planes},
      author={Yoo, Sophia and Chen, Xiaoqi and Rexford, Jennifer},
      booktitle={33rd USENIX Security Symposium (USENIX Security 24)},
      year={2024},
      publisher={USENIX Association}
  }
  ```

- **License:** Copyright 2023 Sophia Yoo & Xiaoqi Chen, Princeton University.
  Released under the **GNU Affero General Public License v3**
  ([`LICENSE`](LICENSE)). The AGPLv3 is preserved unchanged. Per its terms, our
  modifications to the P4 data plane (see [Changes vs. upstream](#changes-vs-upstream))
  are made available here under the same license.

## What it is

SmartCookie is a *split-proxy* SYN-flooding defense: a **switch agent** (P4,
this directory) blocks SYN floods in the data plane, and a **server agent**
(eBPF, upstream `ebpf/`, not ported) runs on the end host in front of an
unmodified TCP stack. Only the switch agent is an NF; the server agent is part
of the environment, like the KVS server behind `netcache`/`switcharoo`.

Per packet, the switch agent:

| From   | Packet                          | Action                                                                 |
|--------|---------------------------------|------------------------------------------------------------------------|
| server | UDP to port 5555 (time sync)    | store `switch_time - server_time` in a register; drop                   |
| server | TCP with ECE set (confirmation) | add the 4-tuple to the bloom filter; drop                               |
| server | TCP, ECE clear                  | forward to the client (`naive_routing`)                                 |
| client | TCP SYN                         | cookie = `time ^ HalfSipHash(4-tuple, seq)`; reply SYN-ACK (2 recirculations) |
| client | TCP non-SYN, bloom hit          | forward to the server                                                   |
| client | TCP non-SYN, bloom miss         | recompute and verify the cookie; forward tagged (ECE, `seq-1`) or drop (3 recirculations) |
| any    | non-TCP IPv4                    | `naive_routing`                                                         |

HalfSipHash-2-4 needs 12 rounds of 14 add/rotate/xor operations; each pipeline
pass (ingress or egress) computes two rounds, carrying the hash state in a
custom `sip_meta` header between passes. All match-action tables are populated
with `const entries`; nothing is installed at runtime and no packet ever
reaches the CPU.

## Layout

```
smartcookie.py            controller (brings up the front-panel ports, then exits)
p4/smartcookie.p4         data plane (upstream SmartCookie-HalfSipHash.p4 + the edits below)
p4/smartcookie-resources.txt  bf-p4c resource usage on Tofino 2
p4/Makefile               APP := smartcookie; includes tofino/tools/Makefile
```

## Changes vs. upstream

The **cookie computation, verification, bloom filter and packet-crafting logic
are unchanged.** The edits are portability fixes for our Tofino 2 DUT and
bf-p4c 9.13.4, plus the testbed port plan:

1. **Tofino 2 target.** `#include <tna.p4>` →
   `#if __TARGET_TOFINO__ == 2 #include <t2na.p4> #else #include <tna.p4> #endif`.
2. **Portable port metadata.** Both `pkt.advance(64) // tofino 1` (resubmit and
   normal paths) → `pkt.advance(PORT_METADATA_SIZE)`.
3. **Explicit `RegisterAction` index types.** bf-p4c 9.13.4 cannot infer the `_`
   index placeholder: the two `reg_timedelta` actions (1 cell, indexed by a
   constant) got `bit<32>`, the four bloom-filter actions got `bit<12>`
   (their `Hash<bit<12>>` index). Pure type annotations.
4. **One recirculation port per pipe.** Upstream picked one of Tofino 1's two
   recirculation ports (68, 196) with a 1-bit random number. Tofino 2 has four
   pipes; `select_recirc_port` sends a packet to the recirculation port of the
   pipe it arrived on (6, 128, 256, 384, keyed by `ingress_port[8:7]`), exactly
   as `tofino/switcharoo` does. Recirculated packets re-enter on the same pipe,
   so this holds on every pass. Same policy, four pipes instead of two; the
   `Random` extern is gone.
5. **`SERVER_PORT = 136`** (was 12): dev port of front-panel port 1, the port
   our KVS NFs use for their server (`KVS_SERVER_PORT` in
   `tofino/switcharoo`). Still a global constant, as upstream.

Everything else is upstream's, including things an expert *could* improve on
Tofino 2 but that would change the design rather than port it:

- The `@pragma stage 0` / `@pragma stage 11` pins size each pass for Tofino 1's
  12 stages. On Tofino 2 (20 stages) stages 12-19 go unused; four rounds per
  pass would halve the recirculations. We keep two rounds per pass.
- `naive_routing` (server→client and non-TCP traffic): egress port = first
  octet of the destination IP. The switch keeps no per-flow state, so the
  client's port has to be derivable from the packet; this is upstream's
  convention and the testbed must assign client IPs accordingly. The SYN-flood
  benchmark never takes this path (SYN-ACKs are reflected to the ingress port).

### Paper vs. prototype

The protocol matches the paper; several parameters and hardening measures in
the paper are not in the released prototype, and we port it as released:

- Standard HalfSipHash XORs `0xff` into `v2` before finalization (paper Fig. 3,
  and the upstream eBPF server agent does it); the P4 does not. The switch and
  server therefore compute different hashes, which only affects the server's
  last-line cookie check on bloom-filter false positives.
- Cookie epochs are 2^28 ns ≈ 268 ms with the current + 2 previous accepted
  (paper: 1 s, current + previous).
- Bloom filter: k = 2 arrays of 4096 bits, never cleaned (paper: 3 × 2^20 bits,
  rotating, cleaned every 15 s).
- Hash key is hardcoded (`DEFAULT_SIP_KEY_*`); the control-plane `sip_init`
  action is commented out upstream (paper: keys rotated every 5-30 s).
- No MSS in the cookie, no TTL filtering, no switch-ID in the setup tag.

### Bloom filter state is per pipe

Tofino registers are private to each pipe. The confirmation from the server
sets the bloom filter in the *server port's* pipe, so the "verified connection"
fast path only applies to client traffic entering on that same pipe; clients on
other pipes have every non-SYN packet cookie-checked instead (still correct,
just slower: 3 recirculations instead of 0). This is a property of the design
on any multi-pipe switch, not of our port. It does not affect the SYN-flood
benchmark.

## Testbed port plan

- Clients: front-panel ports 3-32 (all four pipes). Server: front-panel port 1
  (dev port 136, pipe 1).
- SYN-flood benchmark (upstream's README methodology: send TCP packets with
  flags `0x02`, measure the SYN-ACK response rate until loss): every SYN-ACK
  is reflected to its ingress port, i.e. our **echo methodology**
  (`broadcast = dut_ports`, no symmetric ports, no routes, **no server**).
- On the Tofino 2 **model** (`tofino/tools/ports_tof2.json`) dev ports are
  grouped per pipe (8-64, 136-192, 264-320, 392-448), so dev 136 is the 9th
  entry (veth16/veth17), not front-panel 17.

## Bring-up validation (Tofino 2 model)

Compiled with bf-p4c 9.13.4 (`--arch t2na`, 0 warnings) and exercised on the
SDE 9.13.4 model with hand-crafted packets:

1. SYN on a client port → SYN-ACK back on the same port, `ack = seq+1`,
   `ip.len = 40`, IP and TCP checksums valid (2 recirculations, ~0.1 s on the model).
2. ACK with `ack = cookie+1` sent within the cookie's lifetime → tagged ACK
   (`ECE = 1`, `seq−1`, `ack` preserved) out of `SERVER_PORT`, checksums valid
   (3 recirculations).
3. ACK with a forged cookie → dropped.
4. Server confirmation (ECE, from `SERVER_PORT`) → dropped after setting the
   bloom filter; a data packet on that flow from a client port **on the same
   pipe** → forwarded to the server untouched; from another pipe → cookie-checked
   (per-pipe registers, above).
5. Plain server→client packet → `naive_routing` to the port named by the
   destination IP's first octet.

## Running

Compile and install (Tofino 2):

```
cd tofino/smartcookie/p4 && make install-tofino2
```

Bring up the ports once `bf_switchd` is running (the script exits; the port
configuration persists):

```
cd tofino/smartcookie && PYTHONPATH=$SDE_INSTALL/lib/python3.10/site-packages/tofino:$SDE_INSTALL/lib/python3.10/site-packages ./smartcookie.py
```

Resource report:

```
cd tofino/smartcookie/p4 && ../../tools/get_resources_tofino2.sh smartcookie.p4
```

Throughput evaluation: pending. The traffic generator currently emits UDP only;
the SYN-flood benchmark needs TCP SYN packets (see `eval/`).
