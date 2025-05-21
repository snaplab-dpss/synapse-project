#!/usr/bin/env python3

from pathlib import Path

from msgspec.json import Decoder
from msgspec import Struct

from typing import List, Optional
from datetime import timedelta
from prettytable import PrettyTable

import sys
import argparse
import humanize


class CDF(Struct):
    values: List[int]
    relative_values: List[float]
    probabilities: List[float]

    def zip(self):
        return zip(self.values, self.relative_values, self.probabilities)


class Pcap(Struct):
    device: int
    pcap: str
    warmup: bool


class Config(Struct):
    pcaps: List[Pcap]


class NodeForwardingStats(Struct):
    drop: int
    flood: int
    ports: dict[int, int]


class Meta(Struct):
    bytes: int
    elapsed: int
    pkts: int


class StatsPerMapEpoch(Struct):
    dt_ns: int
    flows: int
    pkts: int
    pkts_per_new_flow: List[int]
    pkts_per_persistent_flow: List[int]
    warmup: bool


class StatsPerMapNodes(Struct):
    crc32_hashes_per_mask: dict[int, int]
    flows: int
    node: int
    pkts: int
    pkts_per_flow: List[int]

    def get_pkts_per_flow_cdf(self, bucket_size: Optional[float] = 0.1) -> CDF:
        cdf = CDF(values=[], relative_values=[], probabilities=[])

        total_flows = len(self.pkts_per_flow)
        total_pkts = self.pkts

        cummulative_pkts = 0
        commulative_flows = 0

        for flow_pkts in self.pkts_per_flow:
            commulative_flows += 1
            cummulative_pkts += flow_pkts

            relative_flows = commulative_flows / total_flows
            relative_pkts = cummulative_pkts / total_pkts

            if bucket_size is None:
                cdf.values.append(commulative_flows)
                cdf.relative_values.append(relative_flows)
            elif len(cdf.relative_values) == 0 or relative_pkts >= cdf.probabilities[-1] + bucket_size:
                cdf.values.append(commulative_flows)
                cdf.relative_values.append(relative_flows)
            else:
                continue

            cdf.probabilities.append(relative_pkts)

        return cdf


class StatsPerMap(Struct):
    epochs: List[StatsPerMapEpoch]
    nodes: List[StatsPerMapNodes]

    def avg_flows_per_epoch(self) -> float:
        non_warmup_epochs = [epoch for epoch in self.epochs if not epoch.warmup]
        return sum([epoch.flows for epoch in non_warmup_epochs]) / len(non_warmup_epochs)


class ProfileData(Struct):
    config: Config
    counters: dict[int, int]
    forwarding_stats: dict[int, NodeForwardingStats]
    meta: Meta
    stats_per_map: dict[int, StatsPerMap]

    def get_total_dropped(self) -> int:
        return sum([node_fwd_stats.drop for node_fwd_stats in self.forwarding_stats.values()])

    def get_total_flooded(self) -> int:
        return sum([node_fwd_stats.flood for node_fwd_stats in self.forwarding_stats.values()])

    def get_total_forwarded(self) -> int:
        return sum([sum(node_fwd_stats.ports.values()) for node_fwd_stats in self.forwarding_stats.values()])


class Profile(Struct):
    fname: Path
    data: ProfileData


def parse_profile(file: Path) -> Profile:
    decoder = Decoder(ProfileData)
    with open(file, "rb") as f:
        json_bytes = f.read()
        data = decoder.decode(json_bytes)
        return Profile(fname=file, data=data)


def build_output_report_fname(profile: Profile, out_dir: Path, suffix_with_extension: str) -> Path:
    basename = profile.fname.stem
    fname = f"{basename}-{suffix_with_extension}"
    return out_dir / fname


def build_txt_report(profile: Profile, out_dir: Path) -> None:
    report = build_output_report_fname(profile, out_dir, "report.txt")

    total_packets = profile.data.meta.pkts
    total_bytes = profile.data.meta.bytes
    total_epochs = max([len(stats_per_map.epochs) for stats_per_map in profile.data.stats_per_map.values()])
    elapsed_seconds = profile.data.meta.elapsed / 1_000_000_000.0
    delta = timedelta(seconds=elapsed_seconds)
    elapsed_hr = humanize.precisedelta(delta, minimum_unit="microseconds")
    dropped_hr = profile.data.get_total_dropped() / total_packets
    flooded_hr = profile.data.get_total_flooded() / total_packets
    forwarded_hr = profile.data.get_total_forwarded() / total_packets

    caption = f"Report for profile {profile.fname.name}"

    table = PrettyTable()
    table.header = False
    table.align = "l"
    table.title = caption

    table.add_row(["Total pkts", f"{total_packets:,}"])
    table.add_row(["Total bytes", f"{total_bytes:,}"])
    table.add_row(["Elapsed", elapsed_hr])
    table.add_row(["Epochs", f"{total_epochs:,}"])
    table.add_row(["Forwarding", f"{dropped_hr:.2%} dropped\n{flooded_hr:.2%} flooded\n{forwarded_hr:.2%} forwarded"])

    for map_addr in profile.data.stats_per_map:
        stats_per_map = profile.data.stats_per_map[map_addr]
        table.add_row([f"Map {map_addr:#x}", f"{int(stats_per_map.avg_flows_per_epoch()):,} average flows"])

        for node in stats_per_map.nodes:
            id = node.node
            pkts_per_flow_cdf = node.get_pkts_per_flow_cdf()
            description = "Pkts/flow CDF ({#flows} {flows %} : {pkts %})"
            pkts_per_flow_cdf_str = "\n".join([f"{cdf[0]:12,} {(cdf[1]):6.2%} : {(cdf[2]):6.2%}" for cdf in pkts_per_flow_cdf.zip()])
            pkts_per_flow_cdf_str = f"{description}\n{pkts_per_flow_cdf_str}"
            table.add_row([f"Map {map_addr:#x} @ Node {id}", pkts_per_flow_cdf_str])

    with open(report, "w") as f:
        f.write(str(table))

    print(f"Report written to {report}")


def main():
    parser = argparse.ArgumentParser(description="Plot profile stats")

    parser.add_argument(
        "data",
        type=str,
        help="Path to the JSON file containing the profile stats",
    )

    parser.add_argument(
        "-o",
        "--out",
        type=str,
        required=True,
        help="Output directory",
    )

    args = parser.parse_args()

    profile_path = Path(args.data)

    if not profile_path.exists():
        print(f"Error: {profile_path} does not exist.")
        sys.exit(1)

    profile = parse_profile(profile_path)

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    if not Path.is_dir(out_dir):
        print(f"Error: {out_dir} is not a directory.")
        sys.exit(1)

    build_txt_report(profile, out_dir)


if __name__ == "__main__":
    main()
