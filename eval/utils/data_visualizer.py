#!/usr/bin/env python3

from argparse import ArgumentParser
from pathlib import Path
from typing import Union
from prettytable import PrettyTable
from dataclasses import dataclass


@dataclass(frozen=True)
class Values:
    requested_bps: int
    pktgen_bps: int
    pktgen_pps: int
    dut_ingress_bps: int
    dut_ingress_pps: int
    dut_egress_bps: int
    dut_egress_pps: int


def str_to_n(key: str) -> Union[int, float]:
    if key.isdigit():
        return int(key)
    return float(key)


if __name__ == "__main__":
    parser = ArgumentParser(description="Data Visualizer")
    parser.add_argument("csv", type=Path, help="Path to data file")
    args = parser.parse_args()

    data: dict[tuple, list[Values]] = {}
    keys_size = 0
    keys_labels = []

    with open(args.csv, "r") as f:
        for line in f.readlines():
            entries = line.strip().split(",")
            assert len(entries) > 7

            i = entries[0]

            if line.startswith("#"):
                keys_labels = entries[1:-7]
                keys_size = len(keys_labels)
                continue

            keys = tuple(map(str_to_n, entries[1:-7]))
            values = Values(*map(int, entries[-7:]))

            if keys not in data:
                data[keys] = []

            data[keys].append(values)

    t = PrettyTable(list(keys_labels) + ["DUT Egress BPS"])
    t.align = "l"

    for keys in sorted(data.keys()):
        list_of_values = data[keys]

        first = True
        for values in list_of_values:
            k = [f"{key:,}" for key in keys] if first else [""] * keys_size
            bps = f"{values.requested_bps:,}"
            t.add_row(k + [bps])
            first = False

    print(t)
