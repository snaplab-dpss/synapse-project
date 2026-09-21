# Can a DirectMeter implement the policer's token bucket?

`dpdk-nfs/pol` keeps, per destination IP, a token bucket of `burst` bytes refilled at `rate` B/s
(`tb_trace` / `tb_update_and_check` / `tb_expire`). The TNA `DirectMeter` is a hardware token bucket,
so the plan is to map the bucket onto a meter attached to the flow table. Three things the Intel
documentation claims had never been compiled, and all three decide the design:

1. can one table carry a `DirectMeter` **and** `idle_timeout = true` (the timeout is how `tb_expire`
   is implemented for every other flow table)?
2. can the meter charge the 4-byte FCS, which an ingress byte meter counts but the C's
   `packet_length` does not?
3. can that same table also carry a `Register`?

## Files

- `meter.p4` — the shape synapse would emit: one exact table on the flow key, `DirectMeter(BYTES)`
  attached, `idle_timeout = true`, an action executing the meter with `adjust_byte_count = 4`, and
  the colour written into the IPv4 DSCP (marking instead of dropping).
- `meter_and_register.p4` — the same table with a `Register` added. Negative test.

Compile (from the repo, which the container mounts at `/home/user/workspace`):

```
bf-p4c --target tofino2 --arch t2na --verbose 2 -o out_meter meter.p4
```

## Takeaways (bf-p4c 9.13.4, Tofino 2)

- **A DirectMeter and an idle timeout DO coexist on one table.** `meter.p4` compiles with 0 errors;
  `context.json` shows `meter_table_refs: [{"how_referenced": "direct", "name": "Ig.tb_meter"}]` and
  an `idletime_stage_table` in every stage the table occupies (`precision: 3`,
  `two_way_notification: true`, in map_ram). Both the meter's colour maprams and the idle timeout
  consume map RAM, so they compete for it.
- **`execute(32w4)` does NOT compile: the call is ambiguous.** The extern declares both
  `execute(in MeterColor_t color, @optional in bit<32> adjust_byte_count)` and
  `execute(@optional in bit<32> adjust_byte_count)` (`tofino2_base.p4:691-692`), so one bare
  argument matches both. bf-p4c: "Ambiguous method execute". The named form
  `execute(adjust_byte_count = 32w4)` compiles. **The emitter must use the named argument.**
- **A table CANNOT carry both a DirectMeter and a Register.** `meter_and_register.p4` fails with
  "table Ig.tb: There are issues with the following indirect externs: DirectMeter Ig.tb_meter,
  Register Ig.tb_reg", followed by an internal compiler error. So the policer's metered table must
  hold no register, which constrains what else synapse may place on it.
- The SDE's own example (`pkgsrc/p4-examples/p4_16_programs/tna_meter_bytecount_adjust`) writes the
  colour into `hdr.ipv4.diffserv`, i.e. DSCP marking is the canonical use of a meter's result.
