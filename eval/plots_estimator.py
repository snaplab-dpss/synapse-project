#!/usr/bin/env python3

import argparse
import sys
import os
import math

import matplotlib.pyplot as plt

from pathlib import Path
from utils.plot_config import *

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
PLOTS_DIR = CURRENT_DIR / "plots"

MAX_XPUT_PPS_SWITCH_ASIC = 117_191_634
MAX_XPUT_PPS_SWITCH_CPU = 120_736

MAX_STABLE_CPU_RATIO = 0.002

def get_estimator_data():
    csv = Path(DATA_DIR / "xput_cpu_counters_periodic_cpu_sender.csv")
    assert csv.exists()
    raw_data = csv_parser(csv)[1]

    data = Data([])
    for d in raw_data:
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
    y_elem = 2 # Throughput pps column    
    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    return x, y, yerr

def xput_estimator_pps(r: float) -> float:
    C = MAX_STABLE_CPU_RATIO
    tA = MAX_XPUT_PPS_SWITCH_ASIC
    tC = MAX_XPUT_PPS_SWITCH_CPU

    if r <= C:
        return tA
    
    ########################
    # Model: a*(1-r) + b*r
    ########################
    # return tA * (1 - (r - C) / (1 - C)) + tC * (r - C) / (1 - C)

    ########################
    # Model: a*log(b*r+c) + d
    ########################
    # if r == 1: return tC
    # a = tC-tA
    # b = (1-1/math.e)/(1-C)
    # c = ((1/math.e)-C)/(1-C)
    # d = tC
    # return a*math.log(b*r+c) + d

    ########################
    # Model: a/r + b
    ########################
    # a = (tA-tC)*(C/(1-C))
    # b = tC-a
    # return (a/r)+b

    ########################
    # Model: a/r (assuming C ~ tC/tA)
    ########################
    # return tC / r

    # ---------------------------------------------------------
    #            |                              |
    #            | SUCCESSFUL MODELS START HERE |
    #            V                              V
    # ---------------------------------------------------------

    ########################
    # Model: 1 / (a*r + b)
    ########################
    # b = (1/(1-C))*((tC-C*tA)/(tA*tC))
    # a = 1/tC - b
    # return 1 / (a*r + b)

    ########################
    # Model: a / (r + b)
    ########################
    b = (C-(tC/tA))/((tC/tA)-1)
    a = tA * (C + b)
    return a / (r + b)

def estimate_xput_pps(cpu_ratios: list[float]) -> list[float]:
    estimates = []
    for cpu_ratio in cpu_ratios:
        assert cpu_ratio >= 0 and cpu_ratio <= 1
        estimates.append(xput_estimator_pps(cpu_ratio))
    return estimates

def plot_thpt_per_cpu_ratio(
    x: list[float],
    y: list[float],
    yerr: list[float],
    out_fname: str
):
    x_label = "Fraction of packets sent to switch CPU"
    y_units = "pps"

    y_ticks = [ 10_000, 100_000, 1_000_000, 10_000_000, 100_000_000 ]
    y_tick_labels = [ "10K", "100K", "1M", "10M", "100M" ]

    y_label = f"Throughput ({y_units})"

    d = [{
        "x": x,
        "y": y,
        "yerr": yerr,
        "label": "Empirical data",
    }]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines=False,
        zorder=1,
    )
    
    steps = 1000
    estimate_x = [ (1/steps)*i for i in range(steps+1) ]
    estimate_y = estimate_xput_pps(estimate_x)
    ax.plot(
        estimate_x,
        estimate_y,
        label="Prediction",
        color="red",
        linestyle="solid",
        linewidth=0.75,
        zorder=2,
    )

    ax.legend(loc="upper right")

    ax.set_yscale("symlog")
    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_tick_labels)

    ax.set_xlim(0, 1)
    ax.set_ylim(ymin=y_ticks[0], ymax=y_ticks[-1]*2)

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    fig_file_pdf = Path(PLOTS_DIR / f"{out_fname}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_fname}.png")

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))

def plot_thpt_per_cpu_ratio_log_x(
    x: list[float],
    y: list[float],
    yerr: list[float],
    out_fname: str
):
    x_label = "Fraction of packets sent to switch CPU"
    y_units = "pps"

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    x[0] = 1e-8

    x_ticks = [ x[0], 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1 ]
    x_tick_labels = [ "0", "$10^{-7}$", "$10^{-6}$", "$10^{-5}$", "$10^{-4}$", "$10^{-3}$", "$10^{-2}$", "$10^{-1}$", 1 ]
    
    y_ticks = [ 10_000, 100_000, 1_000_000, 10_000_000, 100_000_000 ]
    y_tick_labels = [ "10K", "100K", "1M", "10M", "100M" ]

    y_label = f"Throughput ({y_units})"

    d = [{
        "x": x,
        "y": y,
        "yerr": yerr,
        "label": "Empirical data",
    }]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines=False,
        zorder=1,
    )
    
    steps_per_power = 100
    base_power = math.floor(math.log(x[0], 10))
    estimate_x: list[float] = [0]
    for power in range(base_power, 0, 1):
        floor = 10**power
        ceil = 10**(power+1)
        step = (ceil - floor) / steps_per_power
        estimate_x += [ floor + step*i for i in range(steps_per_power) ]
    estimate_x.append(1)
    estimate_y = estimate_xput_pps(estimate_x)

    ax.plot(
        estimate_x,
        estimate_y,
        label="Prediction",
        color="red",
        linestyle="solid",
        linewidth=0.75,
        zorder=2,
    )

    ax.legend(loc="lower left")

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

    fig_file_pdf = Path(PLOTS_DIR / f"{out_fname}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_fname}.png")

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))

def plot_periodic_sender(raw_data: Data, out_fname: str):
    data = Data([])
    for d in raw_data:
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
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    plot_thpt_per_cpu_ratio(x, y, yerr, out_fname)

def plot_periodic_sender_log_x(raw_data: Data, out_fname: str):
    data = Data([])
    for d in raw_data:
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
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    x[0] = x[1] / 10

    plot_thpt_per_cpu_ratio_log_x(x, y, yerr, out_fname)

def plot_table_map(raw_data: Data, out_fname: str):
    data = Data([])
    for d in raw_data:
        i, _, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    plot_thpt_per_cpu_ratio(x, y, yerr, out_fname)

def plot_table_map_log_x(raw_data: Data, out_fname: str):
    data = Data([])
    for d in raw_data:
        i, _, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    x[0] = x[1] / 10

    plot_thpt_per_cpu_ratio_log_x(x, y, yerr, out_fname)

def should_skip(name: str):
    fig_file_pdf = Path(PLOTS_DIR / f"{name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{name}.png")

    should_plot = not fig_file_pdf.exists()
    should_plot = should_plot or (not fig_file_png.exists())

    return not should_plot

csv_pattern_to_plotter = {
    "xput_cpu_counters_periodic_cpu_sender.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio"),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_log_x"),
    ],
    "churn_table_map.csv": [
        (plot_table_map, "estimator_cpu_ratio_table_map"),
        (plot_table_map_log_x, "estimator_cpu_ratio_table_map_log_x"),
    ],
}

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-f", "--force", action="store_true", default=False,
                        help="Regenerate data/plots, even if they already exist")
    
    args = parser.parse_args()

    PLOTS_DIR.mkdir(parents=True, exist_ok=True)

    for csv_fname in csv_pattern_to_plotter.keys():
        csv = Path(f"{DATA_DIR}/{csv_fname}")

        if not csv.exists():
            continue
        
        for plotter, out_name in csv_pattern_to_plotter[csv_fname]:
            if not args.force and should_skip(out_name):
                print(f"{out_name} (skipped)")
                continue

            print(out_name)

            data = csv_parser(csv)[1]
            plotter(data, out_name)

if __name__ == "__main__":
    main()
