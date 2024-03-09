#!/usr/bin/env python3

import argparse
import sys
import re
import os
import glob

from pathlib import Path
from typing import Optional, Union, NewType

from utils.plot_config import *

from natsort import natsorted

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
PLOTS_DIR = CURRENT_DIR / "plots"

def plot_throughput_under_churn_pps(
    labels: list[Optional[str]],
    multiple_data: list[Data],
    out_name: str,
    generate_png: bool,
    color_group: Optional[int],
):
    x_elem = 1 # churn column
    x_label = "Churn (fpm)"
    x_lim = 1e6 # 1M fpm
    x_ticks = [ 0 ] + [ 10**i for i in range(7)]
    x_ticks_labels = [ "0", "1", "10", "100", "1K", "10K", "100K", "1M", ]

    y_elem = 5 # Throughput pps column
    y_units = "pps"
    y_lim = 150

    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_name}.png")

    d: list[dict] = []

    for label, data in zip(labels, multiple_data):
        agg_values = aggregate_values(data, x_elem, y_elem)

        y_max = max([ v[1] for v in agg_values ])
        y_scaler, prefix = get_unit_multiplier(y_max)

        y_label = f"Throughput ({prefix}{y_units})"

        x    = [ v[0] for v in agg_values ]
        y    = [ v[1] / y_scaler for v in agg_values ]
        yerr = [ v[2] / y_scaler for v in agg_values ]

        d.append({
            "x": x,
            "y": y,
            "yerr": yerr,
            "label": label,
        })

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines_only=False,
        set_palette=[palette[color_group]] if color_group else None,
        set_markers_list=[markers_list[color_group]] if color_group else None,
    )

    ax.set_xscale("symlog")
    ax.set_xlim(0, x_lim)
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_ticks_labels)

    ax.set_ylim(ymin=0, ymax=y_lim)

    if all(label is not None for label in labels):
        ax.legend(loc="lower left")

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_throughput_under_churn_bps(
    labels: list[Optional[str]],
    multiple_data: list[Data],
    out_name: str,
    generate_png: bool,
    color_group: Optional[int],
):
    x_elem = 1 # churn column
    x_label = "Churn (fpm)"
    x_lim = 1e6 # 1M fpm
    x_ticks = [ 0 ] + [ 10**i for i in range(7)]
    x_ticks_labels = [ "0", "1", "10", "100", "1K", "10K", "100K", "1M", ]

    y_elem = 4 # Throughput bps column
    y_units = "bps"
    y_lim = 100

    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_name}.png")

    d: list[dict] = []

    for label, data in zip(labels, multiple_data):
        agg_values = aggregate_values(data, x_elem, y_elem)

        y_max = max([ v[1] for v in agg_values ])
        y_scaler, prefix = get_unit_multiplier(y_max)

        y_label = f"Throughput ({prefix}{y_units})"

        x    = [ v[0] for v in agg_values ]
        y    = [ v[1] / y_scaler for v in agg_values ]
        yerr = [ v[2] / y_scaler for v in agg_values ]

        d.append({
            "x": x,
            "y": y,
            "yerr": yerr,
            "label": label,
        })

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines_only=False,
        set_palette=[palette[color_group]] if color_group else None,
        set_markers_list=[markers_list[color_group]] if color_group else None,
    )

    ax.set_xscale("symlog")
    ax.set_xticks(x_ticks)
    ax.set_xlim(0, x_lim)
    ax.set_xticklabels(x_ticks_labels)

    ax.set_ylim(ymin=0, ymax=y_lim)
    
    if all(label is not None for label in labels):
        ax.legend(loc="lower left")

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_thpt_pps_per_cpu_ratio(
    labels: list[Optional[str]],
    multiple_data: list[Data],
    out_name: str,
    generate_png: bool,
    color_group: Optional[int],
):
    assert len(multiple_data) == 1
    data = Data([])

    for d in multiple_data[0]:
        i, target_cpu_ratio, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(target_cpu_ratio - real_cpu_ratio) / target_cpu_ratio
                assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    x_label = "Fraction of packets sent to switch CPU"

    y_elem = 2 # Throughput pps column
    y_units = "pps"

    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_name}.png")

    agg_values = aggregate_values(data, x_elem, y_elem)

    y_ticks = [ 10_000, 100_000, 1_000_000, 10_000_000, 100_000_000 ]
    y_tick_labels = [ "10K", "100K", "1M", "10M", "100M" ]

    y_label = f"Throughput ({y_units})"

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    d = [{
        "x": x,
        "y": y,
        "yerr": yerr,
    }]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines_only=False,
        set_palette=[palette[color_group]] if color_group else None,
        set_markers_list=[markers_list[color_group]] if color_group else None,
    )

    ax.set_yscale("symlog")
    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_tick_labels)

    ax.set_xlim(0, 1)
    ax.set_ylim(ymin=y_ticks[0], ymax=y_ticks[-1]*2)

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_thpt_pps_per_cpu_ratio_log_x(
    labels: list[Optional[str]],
    multiple_data: list[Data],
    out_name: str,
    generate_png: bool,
    color_group: Optional[int],
):
    assert len(multiple_data) == 1
    data = Data([])

    for d in multiple_data[0]:
        i, target_cpu_ratio, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(target_cpu_ratio - real_cpu_ratio) / target_cpu_ratio
                assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    x_label = "Fraction of packets sent to switch CPU"

    y_elem = 2 # Throughput pps column
    y_units = "pps"

    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_name}.png")

    agg_values = aggregate_values(data, x_elem, y_elem)

    y_label = f"Throughput ({y_units})"

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    d = [{
        "x": x,
        "y": y,
        "yerr": yerr,
    }]

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    x[0] = x[1] / 10

    x_ticks = [ x[0], 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1 ]
    x_tick_labels = [ "0", "$10^{-6}$", "$10^{-5}$", "$10^{-4}$", "$10^{-3}$", "$10^{-2}$", "$10^{-1}$", 1 ]
    
    y_ticks = [ 10_000, 100_000, 1_000_000, 10_000_000, 100_000_000 ]
    y_tick_labels = [ "10K", "100K", "1M", "10M", "100M" ]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines_only=False,
        set_palette=[palette[color_group]] if color_group else None,
        set_markers_list=[markers_list[color_group]] if color_group else None,
    )

    ax.set_xscale("log")
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_tick_labels)

    ax.set_yscale("symlog")
    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_tick_labels)

    ax.set_xlim(xmin=x_ticks[0], xmax=x_ticks[-1])
    ax.set_ylim(ymin=y_ticks[0], ymax=y_ticks[-1]*2)

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_thpt_per_pkt_sz_bps(
    labels: list[Optional[str]],
    multiple_data: list[Data],
    out_name: str,
    generate_png: bool,
    color_group: int,
):
    assert len(multiple_data) == 1
    data = multiple_data[0]

    x_elem = 1 # packet size column
    x_label = "Packet size (bytes)"

    y_elem = 2 # Throughput bps column
    y_units = "bps"

    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_name}.png")

    agg_values = aggregate_values(data, x_elem, y_elem)

    y_max = max([ v[1] for v in agg_values ])
    y_scaler, prefix = get_unit_multiplier(y_max)

    y_label = f"Throughput ({prefix}{y_units})"

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] / y_scaler for v in agg_values ]
    yerr = [ v[2] / y_scaler for v in agg_values ]

    d = [{
        "values": y,
        "errors": yerr,
    }]

    xtick_labels = [ str(pkt_sz) for pkt_sz in x ]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    bar_subplot(
        ax,
        x_label,
        y_label,
        d,
        xtick_labels=xtick_labels,
        set_palette=[palette[color_group]],
        set_hatch_list=[hatch_list[color_group]],
    )

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_thpt_per_pkt_sz_pps(
    labels: list[Optional[str]],
    multiple_data: list[Data],
    out_name: str,
    generate_png: bool,
    color_group: int,
):
    assert len(multiple_data) == 1
    data = multiple_data[0]

    x_elem = 1 # packet size column
    x_label = "Packet size (bytes)"

    y_elem = 3 # Throughput pps column
    y_units = "pps"

    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_name}.png")

    agg_values = aggregate_values(data, x_elem, y_elem)

    y_max = max([ v[1] for v in agg_values ])
    y_scaler, prefix = get_unit_multiplier(y_max)

    y_label = f"Throughput ({prefix}{y_units})"

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] / y_scaler for v in agg_values ]
    yerr = [ v[2] / y_scaler for v in agg_values ]

    d = [{
        "values": y,
        "errors": yerr,
    }]

    xtick_labels = [ str(pkt_sz) for pkt_sz in x ]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()
    
    bar_subplot(
        ax,
        x_label,
        y_label,
        d,
        xtick_labels=xtick_labels,
        set_palette=[palette[color_group]],
        set_hatch_list=[hatch_list[color_group]],
    )

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def cache_capacity_label_builder(name) -> Optional[str]:
    pattern = r".*_(\d+)_flows_(\d+)_cache.*"
    
    match = re.match(pattern, name)
    assert match

    flows = int(match.group(1))
    cache = int(match.group(2))

    label = f"{100 * cache / flows:.0f}\\% cache capacity"    
    return label

csv_pattern_to_plotter = {
    "thpt_per_pkt_sz_forwarder.csv": [
        (plot_thpt_per_pkt_sz_bps, "switch_perf_bps", None, COLOR_GROUP_SWITCH_ASIC),
        (plot_thpt_per_pkt_sz_pps, "switch_perf_pps", None, COLOR_GROUP_SWITCH_ASIC),
    ],
    "thpt_per_pkt_sz_send_to_controller.csv": [
        (plot_thpt_per_pkt_sz_bps, "switch_cpu_perf_bps", None, COLOR_GROUP_SWITCH_CPU),
        (plot_thpt_per_pkt_sz_pps, "switch_cpu_perf_pps", None, COLOR_GROUP_SWITCH_CPU),
    ],
    "churn_table_map.csv": [
        (plot_throughput_under_churn_bps, "churn_table_map_bps", None, COLOR_GROUP_SWITCH_ASIC),
        (plot_throughput_under_churn_pps, "churn_table_map_pps", None, COLOR_GROUP_SWITCH_ASIC),
    ],
    # "churn_cached_table_map_*.csv": [
    #     (plot_throughput_under_churn_bps, "churn_cached_table_map_bps", cache_capacity_label_builder, None),
    #     (plot_throughput_under_churn_pps, "churn_cached_table_map_pps", cache_capacity_label_builder, None),
    # ],
    # "churn_transient_cached_table_map_*.csv": [
    #     (plot_throughput_under_churn_bps, "churn_transient_cached_table_map_bps", cache_capacity_label_builder, None),
    #     (plot_throughput_under_churn_pps, "churn_transient_cached_table_map_pps", cache_capacity_label_builder, None),
    # ],
    "xput_cpu_counters_periodic_cpu_sender.csv": [
        (plot_thpt_pps_per_cpu_ratio, "xput_cpu_counters_periodic_cpu_sender", None, None),
        (plot_thpt_pps_per_cpu_ratio_log_x, "xput_cpu_counters_periodic_cpu_sender_log_x", None, None),
    ],
}

def should_skip(name: str, png: bool):
    fig_file_pdf = Path(PLOTS_DIR / f"{name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{name}.png")

    should_plot = not fig_file_pdf.exists()
    should_plot = should_plot or (png and not fig_file_png.exists())

    return not should_plot

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-f", "--force", action="store_true", default=False,
                        help="Regenerate data/plots, even if they already exist")
    parser.add_argument("--png", action="store_true", default=False,
                        help="Also save plots as PNGs")
    
    args = parser.parse_args()

    PLOTS_DIR.mkdir(parents=True, exist_ok=True)

    for csv_pattern in csv_pattern_to_plotter.keys():
        csvs = [ Path(csv) for csv in glob.glob(f"{DATA_DIR}/{csv_pattern}") ]
        csv_filenames = [ csv.stem for csv in csvs ]

        if len(csvs) == 0:
            continue
        
        for plotter, out_name, label_builder, color_group in csv_pattern_to_plotter[csv_pattern]:
            if not args.force and should_skip(out_name, args.png):
                print(f"{out_name} (skipped)")
                continue

            print(out_name)

            data = [ csv_parser(csv)[1] for csv in csvs ]

            if label_builder:
                labels = [ label_builder(csv) for csv in csv_filenames if label_builder ]
            else:
                labels = [ None for _ in range(len(data)) ]

            labels_data = [ (l, d) for l, d in natsorted(zip(labels, data)) ]

            labels = [ x[0] for x in labels_data ]
            data = [ x[1] for x in labels_data ]

            plotter(labels, data, out_name, args.png, color_group)

if __name__ == "__main__":
    main()
