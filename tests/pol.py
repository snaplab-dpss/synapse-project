#!/usr/bin/env python3
"""
pol: per destination IP token bucket policer (a TNA DirectMeter on the flow table, or libnf's
bucket on the controller).
Reference: dpdk-nfs/pol/policer_main.c, built with the arguments in dpdk-nfs/pol/Makefile:
  - internal (LAN) devices are the even ones, so LAN = odd front panel ports (dev = port - 1);
  - forwarding rules pair device 2k with 2k+1, i.e. port p with p+1 (odd p);
  - table capacity 65536, a bucket expires after 1 s without traffic.
Semantics:
  - not IPv4, or IPv4 but not TCP/UDP: drop (from either side);
  - from LAN: always forwarded to the paired WAN port, never policed;
  - from WAN: a tracked destination is charged for the packet and forwarded, remarked to CS1
    (DSCP 8, so the TOS byte reads 0x20) if it is out of profile; an untracked destination starts
    a bucket and is forwarded.
The NF's own bucket (131072 B refilled at 17 GB/s) can never run dry on the model, so the
controller is started with --test-token-bucket-bytes-per-sec/-burst-bytes: every bucket it
builds, a data-plane meter or libnf's, takes these instead. A burst sent back to back at a
tracked destination comes back in order, the first few unmarked and the rest marked (how many
get through is ADMITTED, see there), and a destination left alone refills (or, past the idle
expiry, is readmitted).
The first packet to an untracked destination takes the controller path in the synthesized
solutions, so a burst is always preceded by a packet that gets the destination tracked.
"""

from time import sleep

from util import *

EXPIRATION_SEC = 1.0
CPU_PATH_TIMEOUT = 3.0  # the first packet to an untracked destination goes through the controller

# The rate is the meter's floor (1 kbps), and the frames are big enough that the bucket takes
# seconds to refill half a frame: the model can take hundreds of ms per packet of a burst. The
# bucket refills in BURST / RATE = 20 s, well past the 1 s idle expiry, so a destination left
# alone that long is readmitted rather than refilled.
RATE = 125  # bytes/s
BURST = 2500  # bytes
FRAME = 1000  # bytes on the wire
REFILL_SEC = BURST / RATE
# How many packets of a burst of BURST_LEN at a tracked destination get through: two and a half
# frames fit the burst, and the solutions differ by a frame at either end. libnf charges the
# packet that got the destination tracked, the switch meter is only installed after it; and
# libnf admits a packet only if its frame fits, while the switch meter colors on the level
# before charging, so it lets one more through and goes negative. The model refills a meter at
# this rate in rare, coarse steps, and one landing on a full bucket overshoots it by a frame
# (about 3% of bursts), hence one more.
BURST_LEN = 6
ADMITTED = range(1, 5)
CONTROLLER_ARGS = ("--test-token-bucket-bytes-per-sec", str(RATE), "--test-token-bucket-burst-bytes", str(BURST))

# POLICED_DSCP is 8 in policer_main.c, carried in the top six bits of the TOS byte.
POLICED_TOS = 0x20


def data(flow: Flow, proto: str = "udp") -> Packet:
    """A FRAME-byte packet of `flow`."""
    headers = 14 + 20 + (8 if proto == "udp" else 20)
    return build_packet(flow=flow, payload=b"x" * (FRAME - headers), proto=proto)


def marked(pkt: Packet) -> Packet:
    """`pkt` as the policer remarks it: DSCP 8, the ECN bits kept."""
    out = pkt.copy()
    out[IP].tos = (pkt[IP].tos & 0x03) | POLICED_TOS
    return out


def track(ports: Ports, wan: int, lan: int, pkt: Packet) -> None:
    """One packet to get `pkt`'s destination tracked, forwarded unmarked; it may go through the
    controller."""
    ports.send(wan, pkt)
    expect_unmarked(ports, lan, pkt, timeout=CPU_PATH_TIMEOUT)


def expect_burst(ports: Ports, port: int, pkts: list[Packet], what: str) -> None:
    """`pkts` were sent back to back to a tracked destination; they come out of `port` in order,
    the first ADMITTED of them unmarked and the rest marked."""
    received = ports.collect(CPU_PATH_TIMEOUT)
    if len(received) != len(pkts):
        summary = ", ".join(f"port {r.port}: {pkt_to_string(r.pkt)}" for r in received)
        raise TestFailure(f"{what}: sent {len(pkts)} packets, got {len(received)} back: {summary}")
    marks = [r.pkt[IP].tos & 0xFC == POLICED_TOS for r in received]
    admitted = marks.index(True) if True in marks else len(marks)
    if admitted not in ADMITTED or not all(marks[admitted:]):
        summary = ", ".join("marked" if m else "unmarked" for m in marks)
        raise TestFailure(f"{what}: came back {summary}, expected {ADMITTED.start} to {ADMITTED.stop - 1} unmarked then the rest marked")
    print(f"    {admitted} of {len(pkts)} admitted")
    for i, (r, pkt, mark) in enumerate(zip(received, pkts, marks), 1):
        if r.port != port:
            raise TestFailure(f"{what}: packet {i} came out of port {r.port}, expected {port}")
        assert_equal_packets(marked(pkt) if mark else pkt, r.pkt)


def burst(ports: Ports, wan: int, lan: int, pkts: list[Packet], what: str) -> None:
    assert len(pkts) == BURST_LEN
    for pkt in pkts:
        ports.send(wan, pkt)
    expect_burst(ports, lan, pkts, what)


def expect_unmarked(ports: Ports, port: int, pkt: Packet, timeout: float = DEFAULT_RX_TIMEOUT) -> None:
    """Forwarded whole and in profile: the policer left the DSCP alone."""
    received = expect_packet_from_port(ports, port, pkt, timeout=timeout)
    tos = received[IP].tos
    if tos & 0xFC == POLICED_TOS:
        raise TestFailure(f"packet from port {port} was marked out of profile (tos {hex(tos)}) but is within the rate")


def test(ports: Ports) -> None:
    for lan in lan_ports():
        wan = wan_of(lan)
        step(f"LAN {lan} <-> WAN {wan}: LAN forwarded unpoliced; a WAN destination admitted, then a burst policed")
        lan_pkt = build_packet(flow=build_flow())
        ports.send(lan, lan_pkt)
        expect_unmarked(ports, wan, lan_pkt)

        pkt = data(build_flow())
        track(ports, wan, lan, pkt)
        burst(ports, wan, lan, [pkt] * BURST_LEN, f"WAN {wan}: a burst at a tracked destination")

    step(f"a destination left alone for {REFILL_SEC:.0f}s is admitted again")
    sleep(REFILL_SEC + DEFAULT_SETTLE_TIME)
    ports.send(wan, pkt)
    expect_unmarked(ports, lan, pkt, timeout=CPU_PATH_TIMEOUT)

    lan, wan = lan_ports()[0], wan_of(lan_ports()[0])

    step("the bucket is per destination: a second source to the same destination shares it")
    tracked = build_flow()
    other_source = tracked.clone(new_src_addr="9.9.9.9", new_src_port=4321)
    first, second = data(tracked), data(other_source)
    track(ports, wan, lan, first)
    burst(ports, wan, lan, [first, first] + [second] * (BURST_LEN - 2), "a burst from both sources")

    step("a different destination has a bucket of its own")
    other_dst = data(tracked.clone(new_dst_addr="8.8.8.8"))
    track(ports, wan, lan, other_dst)
    burst(ports, wan, lan, [other_dst] * BURST_LEN, "a burst at the other destination")

    step("TCP flows are handled like UDP flows")
    tcp = data(build_flow(), proto="tcp")
    track(ports, wan, lan, tcp)
    burst(ports, wan, lan, [tcp] * BURST_LEN, "a TCP burst")
    ports.send(lan, tcp)
    expect_unmarked(ports, wan, tcp)

    step("a packet that already carries a DSCP keeps it: the policer only marks what it drops")
    premarked = build_packet(flow=build_flow())
    premarked[IP].tos = 0x28
    ports.send(lan, premarked)
    expect_packet_from_port(ports, wan, premarked)

    step("IPv4 packets that are not TCP/UDP are dropped from both sides")
    ports.send(lan, build_icmp_packet())
    expect_no_packet(ports)
    ports.send(wan, build_icmp_packet())
    expect_no_packet(ports)

    step("non-IPv4 frames are dropped from both sides")
    ports.send(lan, build_non_ip_packet())
    expect_no_packet(ports)
    ports.send(wan, build_non_ip_packet())
    expect_no_packet(ports)

    step(f"a destination seen every {EXPIRATION_SEC / 2}s stays tracked; after {4 * EXPIRATION_SEC}s idle its bucket is gone")
    wan_pkt = build_packet(flow=build_flow())
    ports.send(wan, wan_pkt)
    expect_unmarked(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)
    for _ in range(int(4 * EXPIRATION_SEC / (EXPIRATION_SEC / 2))):
        sleep(EXPIRATION_SEC / 2)
        ports.send(wan, wan_pkt)
        expect_unmarked(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)
    sleep(4 * EXPIRATION_SEC)
    ports.send(wan, wan_pkt)
    expect_unmarked(ports, lan, wan_pkt, timeout=CPU_PATH_TIMEOUT)


if __name__ == "__main__":
    run(test, controller_args=CONTROLLER_ARGS)
