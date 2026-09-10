#!/usr/bin/env python3

import os

from dataclasses import dataclass

from pathlib import Path
from statistics import mean

from tput_eval_data import parse_tput_eval_data_file, EvalDataKey, EvalDataValues

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()
DATA_DIR = PROJECT_DIR / "eval" / "data"

# A 0 in the data means the throughput search never found a rate the solution could
# sustain, not that it forwards nothing. Floor those readings so the comparison keeps
# them instead of dropping the cases where synapse wins by the widest margin.
ZERO_TPUT_PPS = 100_000

# Extra breakdown: the skewed workload the KVS section of the paper quotes.
KVS_NF = "KVS"
KVS_HI_ZIPF_SKEW = 1.2
KVS_HI_CHURN = 1_000_000

DATA = [
    {
        "nf": "KVS",
        "synapse_data_file": DATA_DIR / "tput_synapse_kvs.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_kvs.csv",
    },
    {
        "nf": "FW",
        "synapse_data_file": DATA_DIR / "tput_synapse_fw.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_fw.csv",
    },
    {
        "nf": "NAT",
        "synapse_data_file": DATA_DIR / "tput_synapse_nat.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_nat.csv",
    },
    {
        "nf": "PSD",
        "synapse_data_file": DATA_DIR / "tput_synapse_psd.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_psd.csv",
    },
    {
        "nf": "CL",
        "synapse_data_file": DATA_DIR / "tput_synapse_cl.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_cl.csv",
    },
    {
        "nf": "HLL",
        "synapse_data_file": DATA_DIR / "tput_synapse_hyperloglog.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_hyperloglog.csv",
    },
]


def get_avg_pps(values: EvalDataValues) -> float:
    return mean([max(pps, ZERO_TPUT_PPS) for pps in values.pps])


def get_speedups(
    synapse_data: dict[EvalDataKey, EvalDataValues],
    gallium_data: dict[EvalDataKey, EvalDataValues],
    keys: set[EvalDataKey],
) -> list[float]:
    return [get_avg_pps(synapse_data[key]) / get_avg_pps(gallium_data[key]) for key in keys]


def get_measured_keys(
    gallium_data: dict[EvalDataKey, EvalDataValues],
    keys: set[EvalDataKey],
) -> set[EvalDataKey]:
    """Keys where every gallium reading is a real measurement, i.e. none was floored."""
    return set(key for key in keys if all(pps > 0 for pps in gallium_data[key].pps))


@dataclass
class Comparison:
    label: str
    speedups: list[float]
    measured_speedups: list[float]

    @property
    def total(self) -> int:
        return len(self.speedups)

    def get_speedups(self, measured: bool) -> list[float]:
        return self.measured_speedups if measured else self.speedups


def report(label: str, speedups: list[float], total: int | None = None) -> None:
    line = f"{label:<14}: Avg Speedup = {mean(speedups):8.2f}x, Min Speedup = {min(speedups):8.2f}x, Max Speedup = {max(speedups):8.2f}x"

    if total is not None:
        line += f"  ({len(speedups)}/{total} pts)"

    print(line)


def print_block(
    title: str,
    nfs: list[Comparison],
    breakdowns: list[Comparison],
    measured: bool,
) -> None:
    print(title)

    avgs = []
    mins = []
    maxs = []

    for comparison in nfs:
        speedups = comparison.get_speedups(measured)

        if not speedups:
            print(f"{comparison.label:<14}: No gallium data point sustained a measurable rate.")
            continue

        avgs.append(mean(speedups))
        mins.append(min(speedups))
        maxs.append(max(speedups))

        report(comparison.label, speedups, comparison.total if measured else None)

    overall = f"{'Overall':<14}: Avg Speedup = {mean(avgs):8.2f}x, Min Speedup = {min(mins):8.2f}x, Max Speedup = {max(maxs):8.2f}x"
    if measured:
        overall += f"  ({len(avgs)}/{len(nfs)} NFs)"
    print(overall)

    print()

    for comparison in breakdowns:
        speedups = comparison.get_speedups(measured)

        if not speedups:
            print(f"{comparison.label:<14}: No gallium data point sustained a measurable rate.")
            continue

        report(comparison.label, speedups, comparison.total if measured else None)


def main():
    nfs: list[Comparison] = []
    breakdowns: list[Comparison] = []

    for nf_data in DATA:
        synapse_data = parse_tput_eval_data_file(nf_data["synapse_data_file"])
        gallium_data = parse_tput_eval_data_file(nf_data["gallium_data_file"])

        synapse_keys = set(key for key in synapse_data.keys())
        gallium_keys = set(key for key in gallium_data.keys())
        common_keys = synapse_keys.intersection(gallium_keys)

        if not common_keys:
            print(f"{nf_data['nf']:<14}: No common data points to compare.")
            continue

        measured_keys = get_measured_keys(gallium_data, common_keys)

        nfs.append(
            Comparison(
                label=nf_data["nf"],
                speedups=get_speedups(synapse_data, gallium_data, common_keys),
                measured_speedups=get_speedups(synapse_data, gallium_data, measured_keys),
            )
        )

        if nf_data["nf"] != KVS_NF:
            continue

        zipf_keys = set(key for key in common_keys if key.skew == KVS_HI_ZIPF_SKEW)
        breakdowns.append(
            Comparison(
                label=f"{KVS_NF} s={KVS_HI_ZIPF_SKEW}",
                speedups=get_speedups(synapse_data, gallium_data, zipf_keys),
                measured_speedups=get_speedups(synapse_data, gallium_data, zipf_keys & measured_keys),
            )
        )

        churn_keys = set(key for key in common_keys if key.churn == KVS_HI_CHURN)
        breakdowns.append(
            Comparison(
                label=f"{KVS_NF} c={KVS_HI_CHURN}",
                speedups=get_speedups(synapse_data, gallium_data, churn_keys),
                measured_speedups=get_speedups(synapse_data, gallium_data, churn_keys & measured_keys),
            )
        )

    print_block("=== All data points (a 0 pps gallium reading counts as 100 kpps) ===", nfs, breakdowns, measured=False)
    print()
    print_block("=== Ignoring the 0 pps gallium readings ===", nfs, breakdowns, measured=True)


if __name__ == "__main__":
    main()
