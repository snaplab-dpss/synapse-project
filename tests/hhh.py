#!/usr/bin/env python3
"""
hhh: hierarchical heavy hitters over the source prefixes of WAN traffic, counted per interval.
Reference: dpdk-nfs/hhh/hhh_main.c, built with the arguments in dpdk-nfs/hhh/Makefile:
  - internal (LAN) devices are the even ones, so LAN = odd front panel ports (dev = port - 1);
  - forwarding rules pair device 2k with 2k+1, i.e. port p with p+1 (odd p);
  - the /24 HashPipe has 6 stages of 1024 slots; every table is cleared every second.
Semantics:
  - not IPv4, or IPv4 but not TCP/UDP: dropped (from either side);
  - from LAN: forwarded to the paired WAN port, not counted;
  - from WAN: forwarded to the paired LAN port intact, and counted at every level: the exact
    counters of its /8 and /16 (one cell per prefix) and the HashPipe of its /24 (a {key, count}
    pair in whichever stage holds it);
  - every interval starts afresh: the controller clears every table.
The NF forwards without marking, so the tables are what is checked, read over bfrt: those the
solution keeps on the switch (gallium leaves all but the /8 counter to the controller, where only
the forwarding is observable). The prefixes are keyed as the C reads them: the address's bytes in
packet order as a little-endian number, so the /8 index is the first octet, the /16 index the
first two, the /24 key the first three.
"""

import os
import re
import sys
from dataclasses import dataclass
from glob import glob
from pathlib import Path
from socket import inet_aton
from time import sleep, time

from util import *

INTERVAL_SEC = 1.0
STAGES = 6
# The clear thread fires every second whatever the test is doing: a burst and its reads must fit
# in one interval, and are retried when a clear fell in the middle.
BURST_RETRIES = 3


def prefix_index(ip: str, octets: int) -> int:
    return int.from_bytes(inet_aton(ip)[:octets], "little")


# --- the switch's registers, over bfrt


@dataclass
class Crc32:
    """A CRC-32 as libnf's crc32_hasher computes it (dpdk-nfs/lib/util/math.h): the Rocksoft model
    with refin == refout."""

    coeff: int
    reversed: bool
    init: int
    xor_out: int

    def hash(self, data: bytes) -> int:
        c = self.init
        if self.reversed:
            poly = int(f"{self.coeff:032b}"[::-1], 2)
            for byte in data:
                c ^= byte
                for _ in range(8):
                    c = (c >> 1) ^ poly if c & 1 else c >> 1
        else:
            for byte in data:
                c ^= byte << 24
                for _ in range(8):
                    c = ((c << 1) ^ self.coeff) & 0xFFFFFFFF if c & 0x80000000 else (c << 1) & 0xFFFFFFFF
        return c ^ self.xor_out


IEEE_CRC32 = Crc32(0x04C11DB7, True, 0xFFFFFFFF, 0xFFFFFFFF)


@dataclass
class Stage:
    register: str
    width: int
    crc: Crc32

    def slot(self, key: int) -> int:
        """The C hashes the key's bytes in memory: a little-endian uint32_t."""
        return self.crc.hash(key.to_bytes(4, "little")) & (self.width - 1)

    @staticmethod
    def cell_key(key: int) -> int:
        """The register holds the key as the C has it in memory (bytes the cell is compared with),
        read as one big-endian word."""
        return int.from_bytes(key.to_bytes(4, "little"), "big")


def hhh_registers(p4: Path) -> tuple[dict[str, str], list[Stage]]:
    """The /8 and /16 counters (by register name) and the /24 stages that the switch holds, read
    from `p4`: the NF allocates its vectors in that order (dpdk-nfs/hhh/hhh_state.c) and synapse
    names a register by the address of the vector it implements; a stage's hash is the one its
    inc_or_swap action indexes with, a custom CRC-32 polynomial or the IEEE one. A level the
    solution keeps on the controller is absent (gallium's /16 and HashPipe)."""
    text = p4.read_text()
    counters = {int(size): int(addr) for size, addr in re.findall(r"Register<bit<32>,_>\((\d+), 0\) vector_register_(\d+)_0;", text)}
    pairs = {int(addr): int(width) for addr, width in re.findall(r"Register<vector_register_(\d+)_0_pair_t,_>\((\d+)\) vector_register_\1_0;", text)}
    hash_of = {int(addr): int(h) for addr, h in re.findall(r"vector_register_(\d+)_0_inc_or_swap_\d+\.execute\(meta\.hash_(\d+)_value", text)}
    crcs = {int(h): IEEE_CRC32 for h in re.findall(r"Hash<bit<\d+>>\(HashAlgorithm_t\.CRC32\) hash_(\d+);", text)}
    for coeff, reversed_, init, xor_out, h in re.findall(
        r"CRCPolynomial<bit<32>>\(32w0x([0-9a-f]+), (true|false), false, false, 32w0x([0-9a-f]+), 32w0x([0-9a-f]+)\) hash_(\d+)_poly;", text
    ):
        crcs[int(h)] = Crc32(int(coeff, 16), reversed_ == "true", int(init, 16), int(xor_out, 16))
    if len(pairs) not in (0, STAGES):
        raise TestFailure(f"{p4.name}: expected {STAGES} pair registers or none, found {pairs}")
    stages = []
    for addr in sorted(pairs):
        if addr not in hash_of or hash_of[addr] not in crcs:
            raise TestFailure(f"{p4.name}: cannot tell which CRC-32 indexes vector_register_{addr}_0")
        stages.append(Stage(f"Ingress.vector_register_{addr}_0", pairs[addr], crcs[hash_of[addr]]))
    counters = {f"/{8 * octets}": f"Ingress.vector_register_{counters[size]}_0" for octets, size in ((1, 256), (2, 65536)) if size in counters}
    return counters, stages


class Tables:
    def __init__(self):
        sde_install = os.environ.get("SDE_INSTALL")
        if not sde_install:
            raise TestFailure("SDE_INSTALL is not set; the registers are read over bfrt")
        for site_packages in glob(f"{sde_install}/lib/python*/site-packages"):
            if Path(site_packages, "tofino").is_dir():
                sys.path[:0] = [f"{site_packages}/tofino", site_packages]
                break
        import bfrt_grpc.client as gc

        self.gc = gc
        # The controller is client 0; a fresh client id avoids stepping on it.
        self.client = gc.ClientInterface("127.0.0.1:50052", 1, 0)
        self.client.bind_pipeline_config(program())
        info = self.client.bfrt_info_get(program())
        counters, self.stages = hhh_registers(p4_file())
        self.tables = {level: info.table_get(name) for level, name in counters.items()}
        for i, stage in enumerate(self.stages):
            self.tables[f"/24 stage {i}"] = info.table_get(stage.register)
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        on_switch = [level for level in ("/8", "/16", "/24") if self.on_switch(level)]
        print(f"    levels the switch holds: {', '.join(on_switch) or 'none'} (the others are counted on the controller, unseen here)")

    def on_switch(self, level: str) -> bool:
        return level in self.tables if level != "/24" else bool(self.stages)

    def read(self, level: str, index: int) -> dict:
        """The cell's fields (`f1` for a counter, `lo`/`hi` for a pair), each the largest over the
        pipes: the traffic's pipe holds the live value, the others what the controller last wrote."""
        t = self.tables[level]
        key = t.make_key([self.gc.KeyTuple("$REGISTER_INDEX", index)])
        # The controller clears the tables inside a transaction, which holds them against reads
        # for as long as the clear takes: most of a second on the model, the 65536-entry /16 counter.
        deadline = time() + 3.0
        while True:
            try:
                data = next(t.entry_get(self.target, [key], {"from_hw": True}))[0].to_dict()
                break
            except self.gc.BfruntimeReadWriteRpcException as e:
                if "held by another session" not in str(e) or time() > deadline:
                    raise
                sleep(0.02)
        fields = {}
        for name, values in data.items():
            if name.startswith("$") or name in ("action_name", "is_default_entry"):
                continue
            fields[name.split(".")[-1]] = max(values) if isinstance(values, list) else values
        return fields

    def count(self, level: str, index: int) -> int:
        return self.read(level, index)["f1"]

    def prefixes24(self, ips: list[str]) -> dict[str, tuple[int, list[str]]]:
        """How many packets the HashPipe holds for the /24 of each ip, and the stages holding it (a
        key may sit in several: the paper's duplicates); only the slot the key hashes to in each
        stage is read."""
        found = {}
        for ip in ips:
            key = prefix_index(ip, 3)
            total, stages = 0, []
            for i, stage in enumerate(self.stages):
                cell = self.read(f"/24 stage {i}", stage.slot(key))
                if cell["lo"] == stage.cell_key(key) and cell["hi"] > 0:
                    total += cell["hi"]
                    stages.append(f"stage {i}")
            found[ip] = (total, stages)
        return found


# --- scenarios


def send_and_expect_back(ports: Ports, src_port: int, dst_port: int, pkt: Packet) -> None:
    ports.send(src_port, pkt)
    received = expect_packet_from_port(ports, dst_port, pkt)
    assert_equal_packets(pkt, received)


def wan_packet(src: str, proto: str = "udp") -> Packet:
    return build_packet(flow=Flow(src_addr=src, dst_addr="10.0.0.1", src_port=1234, dst_port=5001), proto=proto)


def wait_for_interval_start(tables: Tables, ports: Ports, wan: int, lan: int) -> None:
    """Returns right after the tables were cleared, so the next second is free of clears: a probe
    counted now disappears at the next clear. Without the /8 counter on the switch there is
    nothing to watch, and nothing the clear could disturb either."""
    probe = wan_packet("77.0.0.1")
    send_and_expect_back(ports, wan, lan, probe)
    if not tables.on_switch("/8"):
        return
    index = prefix_index("77.0.0.1", 1)
    deadline = time() + 2 * INTERVAL_SEC + 1.0
    while time() < deadline:
        if tables.count("/8", index) == 0:
            return
        sleep(0.05)
    raise TestFailure("the /8 counter of a probe never cleared: the controller does not clear the tables every second")


def burst_and_check(tables: Tables, ports: Ports, wan: int, lan: int) -> None:
    """Three WAN sources: a heavy one, a kin in its /8 and /16 (own /24) and an other in prefixes
    of its own; every level counts exactly, the hierarchy shares the short prefixes."""
    heavy, kin, other = "10.1.2.3", "10.1.9.9", "20.0.0.5"
    sent = [(heavy, 5), (kin, 2), (other, 1)]
    expected8 = {prefix_index(heavy, 1): 7, prefix_index(other, 1): 1}
    expected16 = {prefix_index(heavy, 2): 7, prefix_index(other, 2): 1}
    expected24 = {heavy: 5, kin: 2, other: 1}

    for attempt in range(1, BURST_RETRIES + 1):
        wait_for_interval_start(tables, ports, wan, lan)
        started = time()
        # Back to back, then one collection: waiting for each reply in turn costs a settle time
        # per packet, more than an interval for the whole burst. Unless a level is counted on the
        # controller: a punted packet carries the cell value read at punt time and the controller
        # writes value + 1 back, so punts in flight together all write the same count. Paced, each
        # write lands before the next packet is punted.
        paced = not all(tables.on_switch(level) for level in ("/8", "/16", "/24"))
        for src, n in sent:
            pkt = wan_packet(src)
            for _ in range(n):
                ports.send(wan, pkt)
                if paced:
                    sleep(0.06)
        received = ports.collect()
        sent_at = time()
        got8 = {i: tables.count("/8", i) for i in expected8} if tables.on_switch("/8") else {}
        got16 = {i: tables.count("/16", i) for i in expected16} if tables.on_switch("/16") else {}
        got24 = tables.prefixes24(list(expected24)) if tables.on_switch("/24") else {}
        if time() - started > INTERVAL_SEC and (got8 or got16 or got24):
            print(f"    the burst took {sent_at - started:.1f}s and its reads {time() - sent_at:.1f}s, past one interval: retrying ({attempt}/{BURST_RETRIES})")
            continue
        break
    else:
        raise TestFailure("could not fit a burst and its reads in one interval")

    forwarded = {src: 0 for src, _ in sent}
    for r in received:
        if r.port != lan or IP not in r.pkt or r.pkt[IP].src not in forwarded:
            raise TestFailure(f"unexpected frame from port {r.port}: {pkt_to_string(r.pkt)}")
        assert_equal_packets(wan_packet(r.pkt[IP].src), r.pkt)
        forwarded[r.pkt[IP].src] += 1
    for src, n in sent:
        if forwarded[src] != n:
            raise TestFailure(f"{n} packets of {src} sent to WAN {wan}, {forwarded[src]} came out of LAN {lan}")

    for i, n in expected8.items():
        if got8 and got8[i] != n:
            raise TestFailure(f"/8 counter [{i}] is {got8[i]}, expected {n}")
    for i, n in expected16.items():
        if got16 and got16[i] != n:
            raise TestFailure(f"/16 counter [{i}] is {got16[i]}, expected {n}")
    for src, n in expected24.items():
        if got24 and got24[src][0] != n:
            total, stages = got24[src]
            raise TestFailure(f"the /24 of {src} holds {total} packets in {stages or 'no stage'}, expected {n}")
        seen = []
        if got8:
            seen.append(f"/8 {got8[prefix_index(src, 1)]}")
        if got16:
            seen.append(f"/16 {got16[prefix_index(src, 2)]}")
        if got24:
            seen.append(f"/24 {got24[src][0]} in {', '.join(got24[src][1])}")
        print(f"    {src}: {', '.join(seen) or 'forwarded (counted on the controller)'}")


def test(ports: Ports) -> None:
    tables = Tables()
    lan = lan_ports()[0]
    wan = wan_of(lan)

    step("not IPv4, and IPv4 but not TCP/UDP: dropped")
    ports.send(wan, build_non_ip_packet())
    expect_no_packet(ports)
    ports.send(wan, build_icmp_packet())
    expect_no_packet(ports)

    step(f"LAN {lan} -> WAN {wan}: forwarded intact, not counted")
    lan_pkt = build_packet(flow=Flow(src_addr="99.1.2.3", dst_addr="20.0.0.5", src_port=1234, dst_port=80))
    send_and_expect_back(ports, lan, wan, lan_pkt)
    if tables.on_switch("/8") and tables.count("/8", prefix_index("99.1.2.3", 1)) != 0:
        raise TestFailure("a LAN source was counted")

    step(f"WAN {wan} -> LAN {lan}: forwarded intact and counted at every level, shorter prefixes shared")
    burst_and_check(tables, ports, wan, lan)

    if tables.on_switch("/8"):
        step("every table is cleared every second")
        deadline = time() + 2 * INTERVAL_SEC + 1.0
        while tables.count("/8", prefix_index("10.1.2.3", 1)) != 0:
            if time() > deadline:
                raise TestFailure("the /8 counter of 10.1.2.3 was not cleared")
            sleep(0.05)
        # One transaction clears every table: once the /8 is gone, so are the others.
        if tables.on_switch("/16") and tables.count("/16", prefix_index("10.1.2.3", 2)) != 0:
            raise TestFailure("the /16 counter of 10.1.2.3 was not cleared")
        if tables.on_switch("/24"):
            total, stages = tables.prefixes24(["10.1.2.3"])["10.1.2.3"]
            if total != 0:
                raise TestFailure(f"the /24 of 10.1.2.3 still holds {total} packets in {stages}")

    for lan in lan_ports()[1:3]:
        wan = wan_of(lan)
        step(f"WAN {wan} -> LAN {lan}: another port pair counts the same way")
        burst_and_check(tables, ports, wan, lan)


if __name__ == "__main__":
    run(test)
