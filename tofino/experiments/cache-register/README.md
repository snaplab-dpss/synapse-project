# Timeouts

## Takeaways

- `idle_timeout` is a boolean table property which enables the table to expire its entries.
- The controller configures the table with one of two timeout modes: `POLL` and `NOTIFY` (via `attribute_idle_time_set` in python, or `tableAttributesSet`+`idleTableNotifyModeSet`/`idleTablePollModeSet` in C++).
- `POLL` mode allows the controller to query the table for its idle time state, via the `$ENTRY_HIT_STATE` table entry.
- `NOTIFY` mode allows the controller to provide a callback to be called whenever an entry is expired.
- The controller configures the idle timeout value for each entry in **millisecond** units.
- Timeout values are set by setting the entry data `$ENTRY_TTL`, but they can only be set to _as low as the value given for the table timeout_, otherwise we get the following error:

```
2023-06-01 11:55:45.610183 BF_PIPE ERROR - pipe_mgr_mat_ent_add:2851 Cannot add entry with ttl 2000 to match table 0x1000002 device 0 in notify mode, TTL must be at least 3000
```
- The `$ENTRY_TTL` data does not change with time. It sets the value for the timeout, but it does not act as a timeout counter itself. Therefore, we cannot check the current timeout counter using this data.

## Questions

**Can we configure a table in NOTIFY mode but still poll its hit state?**

No. Tables in NOTIFY mode do not have a hit state variable.

Content of a table configured in NOTIFY mode:

```
In [6]: bfrt.timeouts.pipe.Ingress.table_with_timeout.dump
------> bfrt.timeouts.pipe.Ingress.table_with_timeout.dump()
----- table_with_timeout Dump Start -----
Default Entry:
Entry data (action : NoAction):

Entry 0:
Entry key:
    hdr.data.data[111:96]          : 0xC9C0
Entry data (action : Ingress.fwd):
    port                           : 0x01
    $ENTRY_TTL                     : 0x00000000

----- table_with_timeout Dump End -----
```
- Poll mode tables don't receive timeout values. They only change their `$ENTRY_HIT_STATE`. The controller can then reset this value.