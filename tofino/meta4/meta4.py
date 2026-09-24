#!/usr/bin/env python3

# Controller for the Meta4 dataplane app.
#
# Brings up the front-panel ports, then installs what the data plane cannot carry as const entries:
# the watch list (known_domain_list, one ternary entry per pattern) and the client prefix lists
# (banned_dns_dst, LPM). All three come from the files next to this script; see README.md,
# change 6. Once the tables are populated this script exits; the entries persist in bf_switchd.
#
# A pattern's domain id is its line number in the watch list, counted from zero, the same id the C
# NF (dpdk-nfs/meta4) gives it, so per-domain counters line up between the two.
#
# How a name reaches the table: the parser splits each of up to four labels into chunks of 1, 2, 4
# and 8 bytes (the last as two 4-byte halves) chosen by the binary digits of the label's length,
# smallest chunk first, and sets the chunk headers it used valid. An entry therefore names, per
# label, which chunks are valid and what they hold. A "*" label matches any label: every field of
# its position is a don't-care. A label the pattern does not have must be absent from the name:
# every chunk of that position is required invalid, so a three-label pattern does not match a
# four-label name (upstream's rule generator left those positions don't-care). Among patterns that
# could match the same name, the one with fewer wildcards wins.

import argparse
import ipaddress
from pathlib import Path

import bfrt_grpc.client as gc

GRPC_SERVER_IP = "127.0.0.1"
GRPC_SERVER_PORT = 50052
P4_PROGRAM_NAME = "meta4"
DEFAULT_PORT_SPEED = 100

FRONT_PANEL_PORTS = [p for p in range(1, 33)]

HERE = Path(__file__).resolve().parent
DEFAULT_DOMAINS_FILE = HERE / "known_domains_v1.txt"
DEFAULT_BANNED_FILE = HERE / "banned_dns_dst.txt"
DEFAULT_ALLOWED_FILE = HERE / "allowed_dns_dst.txt"

KNOWN_DOMAIN_LIST = "SwitchIngress.known_domain_list"
MATCH_DOMAIN = "SwitchIngress.match_domain"
BANNED_DNS_DST = "SwitchIngress.banned_dns_dst"
MATCH_BANNED_DNS_DST = "SwitchIngress.match_banned_dns_dst"
NO_ACTION = "NoAction"

MAX_LABELS = 4
MAX_LABEL_LEN = 15  # 1 + 2 + 4 + 8: every chunk in use
WILDCARD = "*"

# Chunk name -> (length bit it stands for, bytes it holds). The 8-byte chunk is two headers.
CHUNKS = [("part1", 1, 1), ("part2", 2, 2), ("part4", 4, 4), ("part8_1", 8, 4), ("part8_2", 8, 4)]


class Ports:
    def __init__(self, bfrt_info):
        self.bfrt_info = bfrt_info
        self.port_table = self.bfrt_info.table_get("$PORT")
        self.port_hdl_info_table = self.bfrt_info.table_get("$PORT_HDL_INFO")
        self.port_stat = self.bfrt_info.table_get("$PORT_STAT")

        self.dev_to_front_panel = {}
        self.front_panel_to_dev_port = {}

    def get_dev_port(self, front_panel_port, lane=0):
        target = gc.Target(device_id=0, pipe_id=0xFFFF)

        key = self.port_hdl_info_table.make_key([gc.KeyTuple("$CONN_ID", front_panel_port), gc.KeyTuple("$CHNL_ID", lane)])
        resp = self.port_hdl_info_table.entry_get(target, [key], {"from_hw": False})

        dev_port = next(resp)[0].to_dict()["$DEV_PORT"]
        self.dev_to_front_panel[dev_port] = front_panel_port
        self.front_panel_to_dev_port[front_panel_port] = dev_port

        return dev_port

    # Port list is a list of tuples: (front panel port, speed).
    # Speed is one of {10, 25, 40, 50, 100}.
    def add_ports(self, port_list):
        speed_conversion_table = {
            10: "BF_SPEED_10G",
            25: "BF_SPEED_25G",
            40: "BF_SPEED_40G",
            50: "BF_SPEED_50G",
            100: "BF_SPEED_100G",
        }

        fec_conversion_table = {
            "none": "BF_FEC_TYP_NONE",
            "fec": "BF_FEC_TYP_FC",
            "rs": "BF_FEC_TYP_RS",
        }

        speed_to_fec = {
            10: "none",
            25: "none",
            40: "none",
            50: "none",
            100: "rs",
        }

        target = gc.Target(device_id=0, pipe_id=0xFFFF)

        for front_panel_port, speed in port_list:
            fec = speed_to_fec[speed]
            key = self.port_table.make_key([gc.KeyTuple("$DEV_PORT", self.get_dev_port(front_panel_port))])
            data = self.port_table.make_data(
                [
                    gc.DataTuple("$SPEED", str_val=speed_conversion_table[speed]),
                    gc.DataTuple("$FEC", str_val=fec_conversion_table[fec]),
                    gc.DataTuple("$PORT_ENABLE", bool_val=True),
                ]
            )

            self.port_table.entry_add(target, [key], [data])

    def add_port(self, front_panel_port, speed):
        self.add_ports([(front_panel_port, speed)])


def read_list(path):
    """Non-empty, non-comment lines of a file."""
    with open(path) as f:
        return [line.strip() for line in f if line.strip() and not line.strip().startswith("#")]


def parse_pattern(pattern):
    labels = pattern.split(".")
    if len(labels) > MAX_LABELS:
        raise ValueError(f"`{pattern}` has more than {MAX_LABELS} labels")
    wildcards = 0
    for i, label in enumerate(labels):
        if not label:
            raise ValueError(f"`{pattern}` has an empty label")
        if label == WILDCARD:
            if i != wildcards:
                raise ValueError(f"`{pattern}` has a wildcard after a spelled-out label")
            wildcards += 1
        elif len(label) > MAX_LABEL_LEN:
            raise ValueError(f"`{pattern}` has a label longer than {MAX_LABEL_LEN} bytes, which the parser cannot read")
    return labels


def chunk_fields(label):
    """(chunk name, value, valid) for each chunk of a spelled-out label, smallest chunk first."""
    fields = []
    at = 0
    for chunk, length_bit, size in CHUNKS:
        if len(label) & length_bit:
            fields.append((chunk, int.from_bytes(label[at : at + size].encode(), "big"), True))
            at += size
        else:
            fields.append((chunk, 0, False))
    return fields


def key_field(q, chunk, suffix):
    return f"headers.{q}_{chunk}.{suffix}"


def domain_key_tuples(labels):
    """The ternary key of a pattern, every field of the table stated: matched exactly, required
    absent, or don't-care."""
    tuples = []
    for i in range(MAX_LABELS):
        q = f"q{i + 1}"
        if i >= len(labels):
            # No such label in the name: its chunk headers must all be invalid.
            for chunk, _, _ in CHUNKS:
                tuples.append(gc.KeyTuple(key_field(q, chunk, "$valid"), 0, 1))
                tuples.append(gc.KeyTuple(key_field(q, chunk, "part"), 0, 0))
        elif labels[i] == WILDCARD:
            for chunk, _, _ in CHUNKS:
                tuples.append(gc.KeyTuple(key_field(q, chunk, "$valid"), 0, 0))
                tuples.append(gc.KeyTuple(key_field(q, chunk, "part"), 0, 0))
        else:
            for chunk, value, valid in chunk_fields(labels[i]):
                tuples.append(gc.KeyTuple(key_field(q, chunk, "$valid"), int(valid), 1))
                part_mask = (1 << (8 * dict((c, s) for c, _, s in CHUNKS)[chunk])) - 1 if valid else 0
                tuples.append(gc.KeyTuple(key_field(q, chunk, "part"), value, part_mask))
    return tuples


def check_key_fields(table):
    expected = {key_field(f"q{i + 1}", chunk, suffix) for i in range(MAX_LABELS) for chunk, _, _ in CHUNKS for suffix in ("part", "$valid")}
    actual = set(table.info.key_field_name_list_get())
    missing = expected - actual
    if missing:
        raise RuntimeError(f"{KNOWN_DOMAIN_LIST} does not have key fields {sorted(missing)}; it has {sorted(actual)}")


def install_domains(bfrt_info, patterns):
    table = bfrt_info.table_get(KNOWN_DOMAIN_LIST)
    check_key_fields(table)
    target = gc.Target(device_id=0, pipe_id=0xFFFF)

    keys = []
    data = []
    for domain_id, pattern in enumerate(patterns):
        labels = parse_pattern(pattern)
        priority = labels.count(WILDCARD)  # lower wins: the most specific pattern
        keys.append(table.make_key(domain_key_tuples(labels) + [gc.KeyTuple("$MATCH_PRIORITY", priority)]))
        data.append(table.make_data([gc.DataTuple("id", domain_id)], MATCH_DOMAIN))

    table.entry_add(target, keys, data)


def install_client_lists(bfrt_info, banned, allowed):
    """banned prefixes are ignored. If there are allowed prefixes, everything else is banned too
    (upstream's allow-list); with none, only the banned prefixes are, which is what our NF does."""
    table = bfrt_info.table_get(BANNED_DNS_DST)
    target = gc.Target(device_id=0, pipe_id=0xFFFF)

    def entry(prefix, action):
        network = ipaddress.ip_network(prefix if "/" in prefix else f"{prefix}/32", strict=False)
        key = table.make_key([gc.KeyTuple("headers.ipv4.dst", int(network.network_address), prefix_len=network.prefixlen)])
        return key, table.make_data([], action)

    keys, data = [], []
    for prefix in banned:
        k, d = entry(prefix, MATCH_BANNED_DNS_DST)
        keys.append(k)
        data.append(d)
    if allowed:
        k, d = entry("0.0.0.0/0", MATCH_BANNED_DNS_DST)
        keys.append(k)
        data.append(d)
        for prefix in allowed:
            k, d = entry(prefix, NO_ACTION)
            keys.append(k)
            data.append(d)

    if keys:
        table.entry_add(target, keys, data)


def main():
    parser = argparse.ArgumentParser(description="Bring up the ports and populate Meta4's tables.")
    parser.add_argument("--domains", type=Path, default=DEFAULT_DOMAINS_FILE, help="watched domain patterns, one per line")
    parser.add_argument("--banned", type=Path, default=DEFAULT_BANNED_FILE, help="client prefixes whose DNS responses are ignored")
    parser.add_argument("--allowed", type=Path, default=DEFAULT_ALLOWED_FILE, help="client prefixes tracked to the exclusion of all others; empty for none")
    parser.add_argument("--no-ports", action="store_true", help="skip the port bring-up")
    args = parser.parse_args()

    patterns = read_list(args.domains)
    banned = read_list(args.banned)
    allowed = read_list(args.allowed)

    grpc_client = gc.ClientInterface("{}:{}".format(GRPC_SERVER_IP, GRPC_SERVER_PORT), 0, 0)
    grpc_client.bind_pipeline_config(P4_PROGRAM_NAME)
    bfrt_info = grpc_client.bfrt_info_get(P4_PROGRAM_NAME)

    if not args.no_ports:
        ports = Ports(bfrt_info)
        for port in FRONT_PANEL_PORTS:
            ports.add_port(port, DEFAULT_PORT_SPEED)
        print("Configured ports: {}".format(FRONT_PANEL_PORTS))

    install_domains(bfrt_info, patterns)
    print(f"Installed {len(patterns)} watched domains from {args.domains}")

    install_client_lists(bfrt_info, banned, allowed)
    print(f"Installed {len(banned)} banned and {len(allowed)} allowed client prefixes")


if __name__ == "__main__":
    main()
