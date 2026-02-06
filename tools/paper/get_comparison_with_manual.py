#!/usr/bin/env python3

import os

from pathlib import Path
from statistics import mean

from tput_eval_data import parse_tput_eval_data_file, EvalDataKey
from tofino_resource_parser import parse_tofino_resources_file, Resources

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()
DATA_DIR = PROJECT_DIR / "eval" / "data"
SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"

TESSERA_TPUT_DATA = DATA_DIR / "tput_tessera_kvs.csv"
GALLIUM_TPUT_DATA = DATA_DIR / "tput_gallium_kvs.csv"
NETCACHE_TPUT_DATA = DATA_DIR / "tput_netcache.csv"
SWITCHAROO_TPUT_DATA = DATA_DIR / "tput_switcharoo.csv"

GALLIUM_RESOURCES_FILE = SYNTHESIZED_DIR / "gallium-kvs-resources.txt"
NETCACHE_RESOURCES_FILE = PROJECT_DIR / "tofino" / "netcache" / "p4" / "netcache-resources.txt"
SWITCHAROO_RESOURCES_FILE = PROJECT_DIR / "tofino" / "switcharoo" / "p4" / "switcharoo-resources.txt"

TOTAL_FLOWS = 40_000


def build_tessera_nf_name(nf: str, total_flows: int, churn: int, zipf: float) -> str:
    dist = f"{'unif' if zipf == 0.0 else 'zipf'}{str(int(zipf) if int(zipf) == zipf else zipf).replace('.', '_') if zipf != 0.0 else ''}"
    heuristic = "max-tput"
    return f"{nf}-f{total_flows}-c{churn}-{dist}-h{heuristic}"


def main():
    tessera_data = parse_tput_eval_data_file(TESSERA_TPUT_DATA)
    gallium_data = parse_tput_eval_data_file(GALLIUM_TPUT_DATA)
    netcache_data = parse_tput_eval_data_file(NETCACHE_TPUT_DATA)
    switcharoo_data = parse_tput_eval_data_file(SWITCHAROO_TPUT_DATA)

    assert NETCACHE_RESOURCES_FILE.exists(), f"Synthesized resources file {NETCACHE_RESOURCES_FILE} does not exist!"
    assert SWITCHAROO_RESOURCES_FILE.exists(), f"Synthesized resources file {SWITCHAROO_RESOURCES_FILE} does not exist!"
    assert GALLIUM_RESOURCES_FILE.exists(), f"Synthesized resources file {GALLIUM_RESOURCES_FILE} does not exist!"

    netcache_resources = parse_tofino_resources_file(NETCACHE_RESOURCES_FILE)
    switcharoo_resources = parse_tofino_resources_file(SWITCHAROO_RESOURCES_FILE)
    gallium_resources = parse_tofino_resources_file(GALLIUM_RESOURCES_FILE)

    print(f"NetCache resources: {netcache_resources}")
    print(f"Switcharoo resources: {switcharoo_resources}")
    print(f"Gallium resources: {gallium_resources}")
    print()

    tessera_keys = set(key for key in tessera_data.keys())
    gallium_keys = set(key for key in gallium_data.keys())
    netcache_keys = set(key for key in netcache_data.keys())
    switcharoo_keys = set(key for key in switcharoo_data.keys())
    common_keys = tessera_keys.intersection(gallium_keys, netcache_keys, switcharoo_keys)

    tessera_tput_relative_comparisons = []
    gallium_tput_relative_comparisons = []

    tessera_resources_relative_comparisons = []
    gallium_resources_relative_comparisons = []

    for key in common_keys:
        if key.skew != 1.2:
            continue
        tessera_pps = tessera_data[key].get_avg_pps()[0]
        gallium_pps = gallium_data[key].get_avg_pps()[0]
        netcache_pps = netcache_data[key].get_avg_pps()[0]
        switcharoo_pps = switcharoo_data[key].get_avg_pps()[0]

        max_pps = max(netcache_pps, switcharoo_pps)
        if max_pps == 0:
            print(f"Skipping key {key} due to zero max pps among NetCache and Switcharoo")
            continue

        nf_name = build_tessera_nf_name("kvs", TOTAL_FLOWS, key.churn, key.skew)
        resources_file = SYNTHESIZED_DIR / f"{nf_name}-resources.txt"

        assert resources_file.exists(), f"Synthesized resources file {resources_file} does not exist!"

        tessera_resources = parse_tofino_resources_file(resources_file)

        if netcache_pps > switcharoo_pps:
            print(f"Using NetCache as baseline for key {key}")
            tessera_resources_relative_comparisons.append(tessera_resources.compare_to(netcache_resources))
            gallium_resources_relative_comparisons.append(gallium_resources.compare_to(netcache_resources))
        else:
            print(f"Using Switcharoo as baseline for key {key}")
            tessera_resources_relative_comparisons.append(tessera_resources.compare_to(switcharoo_resources))
            gallium_resources_relative_comparisons.append(gallium_resources.compare_to(switcharoo_resources))

        tessera_tput_relative_comparisons.append(tessera_pps / max_pps)
        gallium_tput_relative_comparisons.append(gallium_pps / max_pps)

    tessera_avg_tput_rel_comparison = mean(tessera_tput_relative_comparisons)
    gallium_avg_tput_rel_comparison = mean(gallium_tput_relative_comparisons)

    print()
    print(f"Comparison of Tessera and Gallium KVS designs against manual designs (NetCache, Switcharoo):")
    print(f"Tessera: {tessera_avg_tput_rel_comparison}")
    print(f"Gallium: {gallium_avg_tput_rel_comparison}")

    tessera_avg_resources_rel_comparison = Resources.average(tessera_resources_relative_comparisons)
    gallium_avg_resources_rel_comparison = Resources.average(gallium_resources_relative_comparisons)

    print()
    print(f"Resources comparison of Tessera and Gallium KVS designs against manual designs (NetCache, Switcharoo):")
    print(f"Tessera: {tessera_avg_resources_rel_comparison} ({tessera_avg_resources_rel_comparison.get_total_geometric_mean()})")
    print(f"Gallium: {gallium_avg_resources_rel_comparison} ({gallium_avg_resources_rel_comparison.get_total_geometric_mean()})")


if __name__ == "__main__":
    main()
