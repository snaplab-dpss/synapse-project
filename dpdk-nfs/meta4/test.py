#!/usr/bin/python3

"""Drives the meta4 NF one packet at a time and checks which decision each packet reached.

The NF watches DNS responses to learn which client-server pairs belong to which domain name, then
attributes those pairs' data packets to that domain. Every scenario below is one packet written to
reach one decision in nf_process; the DEBUG=1 build reports each decision it takes on stderr, and
this compares what it reported against what the scenario expected, packet by packet, so a packet
that stops reaching its decision fails instead of passing silently.

The NF runs on one end of a veth pair through DPDK's af_packet PMD and the packets go into the
other end. (net_tap and net_pcap, which the older NF tests use, are not in this DPDK build.)
--no-pci keeps DPDK away from everything else on the machine.

Run it as ./test.py with paths.sh already sourced; it re-runs itself under `sudo -E` because raw
sockets, `ip link` and DPDK all need root, and -E is what keeps PKG_CONFIG_PATH and the rest of the
DPDK environment alive across the escalation. The build is dropped back to the invoking user so it
does not leave root-owned objects behind.
"""

import argparse
import os
import socket
import struct
import subprocess
import sys
import time

from pathlib import Path

from scapy.all import ARP, IP, TCP, UDP, Ether, Raw, sendp

SCRIPT_DIR = Path(__file__).resolve().parent

NF_IF = "m4nf"
GEN_IF = "m4gen"
LOG = SCRIPT_DIR / "meta4.log"

# Names the NF is told to watch, in this order: the index here is the domain ID it reports.
WATCHED = ["example.com", "a.b.c.d", "*.wild.net", "*.*.deep.net", "exact.wild.net"]

CLIENT = "10.0.0.1"
SERVER_A = "93.184.216.34"
SERVER_B = "93.184.216.35"
SERVER_C = "93.184.216.36"
SERVER_D = "93.184.216.37"
RESOLVER = "8.8.8.8"

# A client whose traffic is deliberately left out of the accounting, and the prefix that says so.
IGNORED_CLIENT = "10.9.0.5"
IGNORED_PREFIX = "10.9.0.0/24"

DNS_PORT = 53
QTYPE_A = 1
QTYPE_CNAME = 5
QCLASS_IN = 1

# How long to wait for a packet's decisions to reach the log before deciding there are none.
SETTLE_S = 0.4


def raw_ip(addr):
    """The value the NF prints: the address' 4 wire bytes read as a host-order word."""
    return int.from_bytes(socket.inet_aton(addr), "little")


def encode_name(labels):
    out = b""
    for label in labels:
        out += bytes([len(label)]) + label.encode()
    return out + b"\x00"


def answer(rr_type, rdata, ttl=60):
    """One resource record, its name a compression pointer back to the question."""
    return struct.pack("!HHHIH", 0xC00C, rr_type, QCLASS_IN, ttl, len(rdata)) + rdata


def cname(labels):
    """A CNAME record pointing at another name, as a resolver puts in front of the address."""
    return answer(QTYPE_CNAME, encode_name(labels))


def address(ip):
    return answer(QTYPE_A, socket.inet_aton(ip))


def dns_message(name_wire, answers, is_response=True):
    """A DNS message laid out as the NF walks it: header, one question, then the answer records."""
    flags = 0x8180 if is_response else 0x0100
    msg = struct.pack("!HHHHHH", 0x1234, flags, 1, len(answers), 0, 0)
    msg += name_wire
    msg += struct.pack("!HH", QTYPE_A, QCLASS_IN)
    return msg + b"".join(answers)


def dns_packet(payload, src=RESOLVER, dst=CLIENT):
    return Ether(src="02:00:00:00:00:01", dst="02:00:00:00:00:02") / IP(src=src, dst=dst) / UDP(sport=DNS_PORT, dport=33333) / Raw(payload)


def data_packet(src, dst, payload_len=64, proto="udp"):
    pkt = Ether(src="02:00:00:00:00:01", dst="02:00:00:00:00:02") / IP(src=src, dst=dst)
    pkt = pkt / (UDP(sport=80, dport=44444) if proto == "udp" else TCP(sport=80, dport=44444))
    return pkt / Raw(b"x" * payload_len)


def scenarios():
    """One entry per packet: what it is, the packet, and the decisions the NF must report for it."""
    out = []

    def send(what, packet, *decisions):
        out.append((what, packet, list(decisions)))

    send("ARP, so not IPv4 at all",
         Ether(src="02:00:00:00:00:01", dst="ff:ff:ff:ff:ff:ff") / ARP())

    send("a DNS query rather than a response",
         dns_packet(dns_message(encode_name(["example", "com"]), [address(SERVER_A)], is_response=False), src=CLIENT, dst=RESOLVER),
         "dns query, ignored")

    send("a response for unwatched.org",
         dns_packet(dns_message(encode_name(["unwatched", "org"]), [address(SERVER_C)])),
         "dns response for an unwatched name")

    send(f"data from {SERVER_A}, whose session nothing has taught yet",
         data_packet(SERVER_A, CLIENT),
         f"unattributed {raw_ip(SERVER_A)} -> {raw_ip(CLIENT)}")

    send(f"a response for example.com pointing at {SERVER_A}",
         dns_packet(dns_message(encode_name(["example", "com"]), [address(SERVER_A)])),
         f"learn session {raw_ip(CLIENT)} -> {raw_ip(SERVER_A)} domain 0")

    pkt = data_packet(SERVER_A, CLIENT)
    send(f"data from {SERVER_A} now that the session is known",
         pkt,
         f"attribute {len(bytes(pkt))} bytes of {raw_ip(SERVER_A)} -> {raw_ip(CLIENT)} to domain 0")

    send("the same flow client-to-server, the direction not tracked",
         data_packet(CLIENT, SERVER_A),
         f"unattributed {raw_ip(CLIENT)} -> {raw_ip(SERVER_A)}")

    pkt = data_packet(SERVER_A, CLIENT, proto="tcp")
    send("TCP on the same session, to show the transport does not matter",
         pkt,
         f"attribute {len(bytes(pkt))} bytes of {raw_ip(SERVER_A)} -> {raw_ip(CLIENT)} to domain 0")

    send("a second response for the pair already tracked",
         dns_packet(dns_message(encode_name(["example", "com"]), [address(SERVER_A)])),
         f"refresh session {raw_ip(CLIENT)} -> {raw_ip(SERVER_A)} domain 0")

    send("a response for a.b.c.d, using all four label slots",
         dns_packet(dns_message(encode_name(["a", "b", "c", "d"]), [address(SERVER_B)])),
         f"learn session {raw_ip(CLIENT)} -> {raw_ip(SERVER_B)} domain 1")

    pkt = data_packet(SERVER_B, CLIENT)
    send(f"data from {SERVER_B}, attributed to the four-label name",
         pkt,
         f"attribute {len(bytes(pkt))} bytes of {raw_ip(SERVER_B)} -> {raw_ip(CLIENT)} to domain 1")

    send("a response for example.com behind one CNAME, as a CDN answers",
         dns_packet(dns_message(encode_name(["example", "com"]), [cname(["edge", "cdn", "net"]), address(SERVER_C)])),
         f"learn session {raw_ip(CLIENT)} -> {raw_ip(SERVER_C)} domain 0")

    pkt = data_packet(SERVER_C, CLIENT)
    send(f"data from {SERVER_C}, the address found behind the CNAME",
         pkt,
         f"attribute {len(bytes(pkt))} bytes of {raw_ip(SERVER_C)} -> {raw_ip(CLIENT)} to domain 0")

    send("an address three CNAMEs deep, since the walk runs to the end of the message",
         dns_packet(dns_message(encode_name(["example", "com"]),
                                [cname(["one", "cdn", "net"]), cname(["two", "cdn", "net"]), cname(["three", "cdn", "net"]),
                                 address(SERVER_D)])),
         f"learn session {raw_ip(CLIENT)} -> {raw_ip(SERVER_D)} domain 0")

    send("a response whose answers are all CNAMEs, so no address is reached",
         dns_packet(dns_message(encode_name(["example", "com"]),
                                [cname(["a", "cdn", "net"]), cname(["b", "cdn", "net"]), cname(["c", "cdn", "net"])])),
         "no name and address in the response")

    send("a watched name, but answered to a client left out of the accounting",
         dns_packet(dns_message(encode_name(["example", "com"]), [address(SERVER_A)]), dst=IGNORED_CLIENT),
         f"dns response to an ignored client {raw_ip(IGNORED_CLIENT)}")

    send("one.wild.net, matching the *.wild.net pattern",
         dns_packet(dns_message(encode_name(["one", "wild", "net"]), [address(SERVER_B)])),
         f"refresh session {raw_ip(CLIENT)} -> {raw_ip(SERVER_B)} domain 2")

    send("other.wild.net, a different subdomain matching the same pattern",
         dns_packet(dns_message(encode_name(["other", "wild", "net"]), [address(SERVER_A)])),
         f"refresh session {raw_ip(CLIENT)} -> {raw_ip(SERVER_A)} domain 2")

    send("wild.net itself, which *.wild.net does not cover",
         dns_packet(dns_message(encode_name(["wild", "net"]), [address(SERVER_C)])),
         "dns response for an unwatched name")

    send("a.b.deep.net, matching *.*.deep.net with two wildcard labels",
         dns_packet(dns_message(encode_name(["a", "b", "deep", "net"]), [address(SERVER_B)])),
         f"refresh session {raw_ip(CLIENT)} -> {raw_ip(SERVER_B)} domain 3")

    send("a.deep.net, one label short of what *.*.deep.net covers",
         dns_packet(dns_message(encode_name(["a", "deep", "net"]), [address(SERVER_C)])),
         "dns response for an unwatched name")

    send("exact.wild.net, named in its own right as well as by *.wild.net",
         dns_packet(dns_message(encode_name(["exact", "wild", "net"]), [address(SERVER_C)])),
         f"refresh session {raw_ip(CLIENT)} -> {raw_ip(SERVER_C)} domain 4")

    send("a response for example.com.au, which only starts like a watched name",
         dns_packet(dns_message(encode_name(["example", "com", "au"]), [address(SERVER_C)])),
         "dns response for an unwatched name")

    return out


GUTTER = "|"


def quote(line):
    """Marks a line as the NF's own output rather than this script's."""
    return f"         {GUTTER} {line}"


def rule():
    print("-" * 94)


def run(cmd, **kwargs):
    return subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, **kwargs)


def build():
    """Builds with DEBUG=1, as the user who invoked sudo so the objects are not left root-owned."""
    prefix = ["sudo", "-E", "-u", os.environ["SUDO_USER"]] if os.environ.get("SUDO_USER") else []
    run(prefix + ["make", "-C", str(SCRIPT_DIR), "clean"])
    run(prefix + ["make", "-C", str(SCRIPT_DIR), "DEBUG=1"])


def veth_up():
    subprocess.run(["ip", "link", "del", NF_IF], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    run(["ip", "link", "add", NF_IF, "type", "veth", "peer", "name", GEN_IF])
    for iface in (NF_IF, GEN_IF):
        run(["ip", "link", "set", iface, "up"])
        # No address and no IPv6, so the only traffic the NF sees is what this script sends.
        run(["sysctl", "-qw", f"net.ipv6.conf.{iface}.disable_ipv6=1"])


def veth_down():
    subprocess.run(["ip", "link", "del", NF_IF], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def start_nf():
    args = [
        str(SCRIPT_DIR / "build/nf"),
        "--no-pci",
        "--vdev", f"net_af_packet0,iface={NF_IF}",
        "--lcores", "0",
        "--no-huge",
        "--no-shconf",
        "--",
        "--drt-capacity", "1024",
        "--drt-expire", "100000000",
        "--known-domains", "64",
        "--ignore-client", IGNORED_PREFIX,
    ]
    for domain in WATCHED:
        args += ["--domain", domain]

    log = open(LOG, "w")
    nf = subprocess.Popen(args, stdout=log, stderr=subprocess.STDOUT)

    for _ in range(30):
        if "forwarding packets" in LOG.read_text():
            return nf
        if nf.poll() is not None:
            break
        time.sleep(1)

    nf.kill()
    print("\nFAIL: the NF did not come up. Its output:\n")
    print("".join(f"  {line}" for line in LOG.read_text().splitlines(keepends=True)))
    raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()

    if os.geteuid() != 0:
        # -E, or the DPDK environment the build needs does not survive.
        os.execvp("sudo", ["sudo", "-E", sys.executable, str(Path(__file__).resolve()), *sys.argv[1:]])

    if subprocess.run(["pkg-config", "--exists", "libdpdk"]).returncode != 0:
        raise SystemExit("pkg-config cannot find libdpdk: source paths.sh before running this")

    rule()
    print(__doc__.strip().split("\n\n")[0])
    print()
    print("  The NF watches DNS responses to learn which client-server pairs belong to which domain")
    print("  name, then attributes those pairs' data packets to that domain. Each packet below is")
    print("  written to reach one decision in nf_process, and is checked against what the NF reports.")
    rule()

    script = scenarios()
    print("Watching " + ", ".join(f"{d} (domain {i})" for i, d in enumerate(WATCHED)))
    print(f"Ignoring clients in {IGNORED_PREFIX}")
    print()

    build()
    veth_up()

    failures = 0
    try:
        nf = start_nf()

        print(f"Everything the NF itself writes is quoted below with {GUTTER}; the rest is this script.\n")
        for line in LOG.read_text().splitlines():
            print(quote(line))
        print()

        log = open(LOG, "r")
        log.read()  # skip what was just shown; from here on only new lines matter

        print(f"Sending {len(script)} packets, one at a time:\n")

        for i, (what, packet, expected) in enumerate(script, start=1):
            sendp(packet, iface=GEN_IF, verbose=0)
            time.sleep(SETTLE_S)

            # Kept verbatim, DEBUG: prefix and all, so what is quoted is exactly what the NF wrote.
            reported = [line.rstrip() for line in log.readlines() if line.startswith("DEBUG: ")]
            decisions = [line[len("DEBUG: "):] for line in reported]

            ok = decisions == expected
            failures += 0 if ok else 1

            print(f"  {i:3d}  {'ok ' if ok else 'BAD'}  {what}")
            for line in reported:
                print(quote(line))
            if not reported:
                print(quote("(the NF wrote nothing)"))
            if not ok:
                for decision in expected or ["nothing"]:
                    print(f"            ^ expected instead: DEBUG: {decision}")

        nf.kill()
        nf.wait()
    finally:
        veth_down()

    rule()
    if failures == 0:
        print(f"PASS: all {len(script)} packets reached the decision they were written for")
        return 0

    print(f"FAIL: {failures} of {len(script)} packets did not. Full NF output is in {LOG}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
