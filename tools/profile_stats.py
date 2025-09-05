#!/usr/bin/env python3

from helpers.profile import parse_profile, Profile, CDF, top_k_flows

from pathlib import Path
from datetime import timedelta
from prettytable import PrettyTable

import humanize
import sys
import argparse


def build_report(profile: Profile) -> str:
    total_packets = profile.data.meta.pkts
    total_bytes = profile.data.meta.bytes
    avg_bytes_per_pkt = int(total_bytes / total_packets)
    total_warmup_epochs = profile.data.get_total_warmup_epochs()
    total_non_warmup_epochs = profile.data.get_total_non_warmup_epochs()
    elapsed_seconds = profile.data.meta.elapsed / 1_000_000_000.0
    delta = timedelta(seconds=elapsed_seconds)
    elapsed_hr = humanize.precisedelta(delta, minimum_unit="microseconds")

    total_bytes_on_the_write = (8 + avg_bytes_per_pkt + 12) * total_packets
    packet_rate_bps = int(total_bytes_on_the_write * 8 / elapsed_seconds)
    packet_rate_pps = int(total_packets / elapsed_seconds)

    table = PrettyTable()
    table.header = False
    table.align = "l"
    table.title = f"Report for profile {profile.fname.name}"

    table.add_row(["Total pkts", f"{total_packets:,}"])
    table.add_row(["Total pkt bytes", f"{total_bytes:,}"])
    table.add_row(["Avg pkt size", f"{avg_bytes_per_pkt:,} bytes"])
    table.add_row(["Bytes on the wire", f"{total_bytes_on_the_write:,}"])
    table.add_row(["Packet rate", f"{packet_rate_bps:,} bps ({packet_rate_pps:,} pps)"])
    table.add_row(["Elapsed", elapsed_hr])
    table.add_row(["Warmup epochs", f"{total_warmup_epochs:,}"])
    table.add_row(["Epochs", f"{total_non_warmup_epochs:,}"])

    expirations_per_epoch = [f"{n:,}" for n in profile.data.expirations_per_epoch]
    table.add_row(["Expirations/epoch", "\n".join(expirations_per_epoch)])

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
