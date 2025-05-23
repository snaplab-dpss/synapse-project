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

    @staticmethod
    def build_cdf_from_data(data: List[int], bucket_size: Optional[float] = 0.1):
        cdf = CDF(values=[], relative_values=[], probabilities=[])

        cdf.values = []
        cdf.relative_values = []
        cdf.probabilities = []

        total_x = len(data)
        total_y = sum(data)

        commulative_y = 0
        commulative_x = 0

        for y in data:
            commulative_x += 1
            commulative_y += y

            relative_flows = commulative_x / total_x
            relative_pkts = commulative_y / total_y

            if bucket_size is None:
                cdf.values.append(commulative_x)
                cdf.relative_values.append(relative_flows)
            elif len(cdf.relative_values) == 0 or relative_pkts >= cdf.probabilities[-1] + bucket_size:
                cdf.values.append(commulative_x)
                cdf.relative_values.append(relative_flows)
            else:
                continue

            cdf.probabilities.append(relative_pkts)

        if len(cdf.probabilities) > 0 and cdf.probabilities[-1] < 1.0:
            cdf.values.append(commulative_x)
            cdf.relative_values.append(1.0)
            cdf.probabilities.append(1.0)

        return cdf

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

    def get_pkts_per_flow_cdf(self) -> CDF:
        return CDF.build_cdf_from_data(self.pkts_per_flow)


class StatsPerMap(Struct):
    epochs: List[StatsPerMapEpoch]
    nodes: List[StatsPerMapNodes]

    def get_total_warmup_epochs(self) -> int:
        return len([epoch for epoch in self.epochs if epoch.warmup])

    def get_total_non_warmup_epochs(self) -> int:
        return len([epoch for epoch in self.epochs if not epoch.warmup])

    def avg_flows_per_epoch(self) -> int:
        non_warmup_epochs = [epoch for epoch in self.epochs if not epoch.warmup]
        if len(non_warmup_epochs) == 0:
            return 0
        return int(sum([epoch.flows for epoch in non_warmup_epochs]) / len(non_warmup_epochs))

    def get_churn_top_k_flows(self, k: int) -> int:
        avg_churn_fpm = 0
        total_epochs = self.get_total_non_warmup_epochs()

        for epoch in self.epochs:
            if epoch.warmup:
                continue

            i = 0
            j = 0
            top_k_new_flows = 0

            while i < len(epoch.pkts_per_persistent_flow) and j < len(epoch.pkts_per_new_flow) and i + j < k:
                if epoch.pkts_per_persistent_flow[i] > epoch.pkts_per_new_flow[j]:
                    i += 1
                else:
                    j += 1
                    top_k_new_flows += 1

            if epoch.dt_ns > 0:
                churn = (60.0 * top_k_new_flows) / (epoch.dt_ns / 1_000_000_000.0)
                avg_churn_fpm += churn

        if total_epochs > 0:
            avg_churn_fpm /= total_epochs

        return int(avg_churn_fpm)


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

    def get_forwarded_per_port(self) -> dict[int, int]:
        forwarded_per_port = {}
        for node_fwd_stats in self.forwarding_stats.values():
            for port, count in node_fwd_stats.ports.items():
                if port not in forwarded_per_port:
                    forwarded_per_port[port] = 0
                forwarded_per_port[port] += count
        return forwarded_per_port

    def get_total_warmup_epochs(self) -> int:
        all_stats_per_map = self.stats_per_map.values()
        if len(all_stats_per_map) == 0:
            return 0
        return max([stats.get_total_warmup_epochs() for stats in all_stats_per_map])

    def get_total_non_warmup_epochs(self) -> int:
        all_stats_per_map = self.stats_per_map.values()
        if len(all_stats_per_map) == 0:
            return 0
        return max([stats.get_total_non_warmup_epochs() for stats in all_stats_per_map])


class Profile(Struct):
    fname: Path
    data: ProfileData


def parse_profile(file: Path) -> Profile:
    decoder = Decoder(ProfileData)
    with open(file, "rb") as f:
        json_bytes = f.read()
        data = decoder.decode(json_bytes)
        return Profile(fname=file, data=data)


def top_k_flows(total_flows: int) -> List[int]:
    top_k = []
    i = 0
    while 10**i < total_flows:
        top_k.append(10**i)
        i += 1
    top_k.append(10**i)
    return top_k


def build_report(profile: Profile) -> str:
    total_packets = profile.data.meta.pkts
    total_bytes = profile.data.meta.bytes
    avg_bytes_per_pkt = int(total_bytes / total_packets)
    total_warmup_epochs = profile.data.get_total_warmup_epochs()
    total_non_warmup_epochs = profile.data.get_total_non_warmup_epochs()
    elapsed_seconds = profile.data.meta.elapsed / 1_000_000_000.0
    delta = timedelta(seconds=elapsed_seconds)
    elapsed_hr = humanize.precisedelta(delta, minimum_unit="microseconds")

    table = PrettyTable()
    table.header = False
    table.align = "l"
    table.title = f"Report for profile {profile.fname.name}"

    table.add_row(["Total pkts", f"{total_packets:,}"])
    table.add_row(["Total bytes", f"{total_bytes:,}"])
    table.add_row(["Avg pkt size", f"{avg_bytes_per_pkt:,} bytes"])
    table.add_row(["Elapsed", elapsed_hr])
    table.add_row(["Warmup epochs", f"{total_warmup_epochs:,}"])
    table.add_row(["Epochs", f"{total_non_warmup_epochs:,}"])

    forwarding_str = []
    forwarding_str += [f"{profile.data.get_total_dropped() / total_packets:.2%} dropped"]
    forwarding_str += [f"{profile.data.get_total_flooded() / total_packets:.2%} flooded"]
    forwarding_str += [f"{profile.data.get_total_forwarded() / total_packets:.2%} forwarded"]
    forwarding_str += ["Forwarding per port ({port} : {hit rate} ({count}/{total}))"]
    for port, count in sorted(list(profile.data.get_forwarded_per_port().items())):
        forwarding_str += [f"{port:12}: {count/ total_packets:9.4%} ({count:,}/{total_packets:,})"]
    table.add_row(["Forwarding", "\n".join(forwarding_str)])

    for map_addr in profile.data.stats_per_map:
        stats_per_map = profile.data.stats_per_map[map_addr]
        stats_per_map_str = []
        stats_per_map_str += [f"{stats_per_map.avg_flows_per_epoch():,} average flows/epoch"]
        stats_per_map_str += ["Top k churn ({top k flows} : {churn fpm})"]
        stats_per_map_str += [f"{k:12,}: {stats_per_map.get_churn_top_k_flows(k):,}" for k in top_k_flows(stats_per_map.avg_flows_per_epoch())]
        table.add_row([f"Map {map_addr:#x}", "\n".join(stats_per_map_str)])

        for node in stats_per_map.nodes:
            id = node.node
            pkts_per_flow_cdf = node.get_pkts_per_flow_cdf()

            description = "Pkts/flow CDF ({#flows} {flows %} : {pkts %})"
            pkts_per_flow_cdf_str = "\n".join([f"{cdf[0]:12,} {(cdf[1]):7.2%} : {(cdf[2]):7.2%}" for cdf in pkts_per_flow_cdf.zip()])
            pkts_per_flow_cdf_str = f"{description}\n{pkts_per_flow_cdf_str}"
            table.add_row([f"Map {map_addr:#x} @ Node {id}", pkts_per_flow_cdf_str])

        for i, epoch in enumerate(stats_per_map.epochs):
            if epoch.warmup:
                continue

            pkts_per_new_flow_cdf = CDF.build_cdf_from_data(epoch.pkts_per_new_flow)
            description = "Pkts/new flow CDF ({#flows} {flows %} : {pkts %})"
            pkts_per_new_flow_cdf_str = "\n".join([f"{cdf[0]:12,} {(cdf[1]):7.2%} : {(cdf[2]):7.2%}" for cdf in pkts_per_new_flow_cdf.zip()])
            pkts_per_new_flow_cdf_str = f"{description}\n{pkts_per_new_flow_cdf_str}"
            table.add_row([f"Map {map_addr:#x} @ Epoch {i}", pkts_per_new_flow_cdf_str])

            pkts_per_persistent_flow_cdf = CDF.build_cdf_from_data(epoch.pkts_per_persistent_flow)
            description = "Pkts/persistent flow CDF ({#flows} {flows %} : {pkts %})"
            pkts_per_persistent_flow_cdf_str = "\n".join([f"{cdf[0]:12,} {(cdf[1]):7.2%} : {(cdf[2]):7.2%}" for cdf in pkts_per_persistent_flow_cdf.zip()])
            pkts_per_persistent_flow_cdf_str = f"{description}\n{pkts_per_persistent_flow_cdf_str}"
            table.add_row([f"Map {map_addr:#x} @ Epoch {i}", pkts_per_persistent_flow_cdf_str])

    return str(table)


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
        help="Output file path for the report",
    )

    args = parser.parse_args()

    profile_path = Path(args.data)

    if not profile_path.exists():
        print(f"Error: {profile_path} does not exist.")
        sys.exit(1)

    profile = parse_profile(profile_path)
    report = build_report(profile)

    if args.out:
        out_fname = Path(args.out)
        if Path.is_dir(out_fname):
            print(f"Error: {out_fname} is a directory.")
            sys.exit(1)
        with open(args.out, "w") as f:
            f.write(report)
        print(f"Report written to {args.out}")
    else:
        print(report)


if __name__ == "__main__":
    main()
