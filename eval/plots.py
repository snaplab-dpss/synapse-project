#!/usr/bin/env python3

import argparse
import sys
import re
import os
import glob

from pathlib import Path
from typing import Optional

import matplotlib.pyplot as plt

from utils.estimator import estimate_xput_pps
from utils.parser import Data, aggregate_values, csv_parser
from utils.plot_config import *

from natsort import natsorted

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
PLOTS_DIR = CURRENT_DIR / "plots"

def sort_data_with_labels(
    multiple_data: list[Data],
    labels: list[str]
) -> tuple[list[Data], list[str]]:
    labels_data = [ (l, d) for l, d in natsorted(zip(labels, multiple_data)) ]
    labels = [ x[0] for x in labels_data ]
    multiple_data = [ x[1] for x in labels_data ]
    return multiple_data, labels

def plot_throughput_under_churn_pps(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    labels = []
    for data_properties in multiple_data_properties:
        assert "relative_cache_capacity" in data_properties
        relative_cache_capacity = data_properties["relative_cache_capacity"]
        labels.append(f"{100 * relative_cache_capacity:.0f}\\% cache capacity")

    multiple_data, labels = sort_data_with_labels(multiple_data, labels)

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

    ll = labels or [None] * len(multiple_data)
    for label, data in zip(ll, multiple_data):
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
        set_palette=[palette[color_group]] if color_group else None,
        set_markers_list=[markers_list[color_group]] if color_group else None,
    )

    ax.set_xscale("symlog")
    ax.set_xlim(0, x_lim)
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_ticks_labels)

    ax.set_ylim(ymin=0, ymax=y_lim)

    if all(label is not None for label in ll):
        ax.legend(loc="lower left")

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_throughput_under_churn_bps(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    labels = []
    for data_properties in multiple_data_properties:
        assert "relative_cache_capacity" in data_properties
        relative_cache_capacity = data_properties["relative_cache_capacity"]
        labels.append(f"{100 * relative_cache_capacity:.0f}\\% cache capacity")

    multiple_data, labels = sort_data_with_labels(multiple_data, labels)

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

    ll = labels or [None] * len(multiple_data)
    for label, data in zip(ll, multiple_data):
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
        set_palette=[palette[color_group]] if color_group else None,
        set_markers_list=[markers_list[color_group]] if color_group else None,
    )

    ax.set_xscale("symlog")
    ax.set_xticks(x_ticks)
    ax.set_xlim(0, x_lim)
    ax.set_xticklabels(x_ticks_labels)

    ax.set_ylim(ymin=0, ymax=y_lim)
    
    if all(label is not None for label in ll):
        ax.legend(loc="lower left")

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_churn_thpt_pps_per_cpu_ratio(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(multiple_data) == 1
    data = Data([])

    for d in multiple_data[0]:
        i, _, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    x_label = "Fraction of packets sent to switch CPU"

    y_elem = 2 # Throughput pps column
    y_units = "pps"

    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_name}.png")

    agg_values = aggregate_values(data, x_elem, y_elem)

    y_ticks = PPS_TICKS
    y_tick_labels = PPS_TICKS_LABELS

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
        lines=False,
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
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_churn_thpt_pps_per_cpu_ratio_log_x(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(multiple_data) == 1
    data = Data([])

    for d in multiple_data[0]:
        i, _, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
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
    x[0] = 1e-8

    x_ticks = [ x[0], 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1 ]
    x_tick_labels = [ "0", "$10^{-7}$", "$10^{-6}$", "$10^{-5}$", "$10^{-4}$", "$10^{-3}$", "$10^{-2}$", "$10^{-1}$", 1 ]
    
    y_ticks = PPS_TICKS
    y_tick_labels = PPS_TICKS_LABELS

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines=False,
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
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_thpt_pps_per_cpu_ratio(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
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

    y_ticks = PPS_TICKS
    y_tick_labels = PPS_TICKS_LABELS

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
        lines=False,
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
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_thpt_pps_per_cpu_ratio_log_x(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
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
    x[0] = 1e-8

    x_ticks = [ x[0], 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1 ]
    x_tick_labels = [ "0", "$10^{-7}$", "$10^{-6}$", "$10^{-5}$", "$10^{-4}$", "$10^{-3}$", "$10^{-2}$", "$10^{-1}$", 1 ]
    
    y_ticks = PPS_TICKS
    y_tick_labels = PPS_TICKS_LABELS

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines=False,
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
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_thpt_per_pkt_sz_bps(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
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
        set_palette=[palette[color_group]] if color_group else None,
        set_hatch_list=[hatch_list[color_group]] if color_group else None,
    )

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_thpt_per_pkt_sz_pps(
    multiple_data: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
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
        set_palette=[palette[color_group]] if color_group else None,
        set_hatch_list=[hatch_list[color_group]] if color_group else None,
    )

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_cpu_thpt_per_aggregate_thpt(
    cpu_ratio: list[float],
    aggregate_thpt: list[float],
    yerr: list[float],
    out_fname: str,
):
    cpu_thpt = [ c * t for c, t in zip(cpu_ratio, aggregate_thpt) ]

    thpt_units = "pps"
    x_label = f"Aggregate throughput ({thpt_units})"

    x_ticks = PPS_TICKS
    x_tick_labels = PPS_TICKS_LABELS

    y_ticks = PPS_TICKS
    y_tick_labels = PPS_TICKS_LABELS

    y_label = f"Switch CPU\nthroughput ({thpt_units})"

    d = [{
        "x": aggregate_thpt,
        "y": cpu_thpt,
        "yerr": yerr,
    }]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    # print(aggregate_thpt)
    # print(cpu_thpt)
    # exit(0)

    plot_subplot(
        ax,
        x_label,
        y_label,
        d,
        lines=False,
        zorder=1,
    )
    
    ax.set_xscale("symlog")
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_tick_labels)
    ax.set_xlim(xmin=y_ticks[0], xmax=y_ticks[-1]*2)

    ax.set_yscale("symlog")
    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_tick_labels)
    ax.set_ylim(ymin=y_ticks[0], ymax=y_ticks[-1]*2)

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    fig_file_pdf = Path(PLOTS_DIR / f"{out_fname}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{out_fname}.png")

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))
    plt.close()

def plot_thpt_per_cpu_ratio(
    x: list[float],
    y: list[float],
    yerr: list[float],
    out_fname: str,
    loops: int = 0,
):
    x_label = "Fraction of packets sent to switch CPU"
    y_units = "pps"

    y_ticks = PPS_TICKS
    y_tick_labels = PPS_TICKS_LABELS

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
    estimate_y = estimate_xput_pps(estimate_x, loops)
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
    plt.close()

def plot_thpt_per_cpu_ratio_log_x(
    x: list[float],
    y: list[float],
    yerr: list[float],
    out_fname: str,
    loops: int = 0,
):
    x_label = "Fraction of packets sent to switch CPU"
    y_units = "pps"

    base_x = 1e-8

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    if x[0] == 0: x[0] = base_x

    x_ticks = [ 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1 ]
    x_tick_labels = [ "0", "$10^{-7}$", "$10^{-6}$", "$10^{-5}$", "$10^{-4}$", "$10^{-3}$", "$10^{-2}$", "$10^{-1}$", 1 ]
    
    y_ticks = PPS_TICKS
    y_tick_labels = PPS_TICKS_LABELS

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
    
    base_power = math.floor(math.log(base_x, 10))
    estimate_x: list[float] = [0]
    for power in range(base_power, 0, 1):
        floor = 10**power
        ceil = 10**(power+1)
        step = (ceil - floor) / steps_per_power
        estimate_x += [ floor + step*i for i in range(steps_per_power) ]
    estimate_x.append(1)
    estimate_y = estimate_xput_pps(estimate_x, loops)

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
    plt.close()

def plot_periodic_sender(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    loops = 0
    if len(multiple_data_properties) == 1:
        data_properties = multiple_data_properties[0]
        assert "loops" in data_properties
        loops = data_properties["loops"]

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

    plot_thpt_per_cpu_ratio(x, y, yerr, out_name, loops)
    plot_cpu_thpt_per_aggregate_thpt(x, y, yerr, f"{out_name}_thpt_per_load")

def plot_periodic_sender_log_x(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    loops = 0
    if len(multiple_data_properties) == 1:
        data_properties = multiple_data_properties[0]
        assert "loops" in data_properties
        loops = data_properties["loops"]

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

    plot_thpt_per_cpu_ratio_log_x(x, y, yerr, out_name, loops)

def plot_table_map(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

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

    plot_thpt_per_cpu_ratio(x, y, yerr, out_name)

def plot_table_map_log_x(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

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

    plot_thpt_per_cpu_ratio_log_x(x, y, yerr, out_name)

def plot_estimator_ep0(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    data = Data([])
    for d in raw_data:
        i, target_cpu_ratio, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
        expected_cpu_ratio = 1/int(1/target_cpu_ratio) if target_cpu_ratio != 0 else 0

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(expected_cpu_ratio - real_cpu_ratio) / expected_cpu_ratio
                assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    plot_thpt_per_cpu_ratio(x, y, yerr, out_name)

def plot_estimator_ep0_log_x(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    data = Data([])
    for d in raw_data:
        i, target_cpu_ratio, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
        expected_cpu_ratio = 1/int(1/target_cpu_ratio) if target_cpu_ratio != 0 else 0

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(expected_cpu_ratio - real_cpu_ratio) / expected_cpu_ratio
                assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    if x[0] == 0: x[0] = x[1] / 10

    plot_thpt_per_cpu_ratio_log_x(x, y, yerr, out_name)

def plot_estimator_ep1(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    data = Data([])
    for d in raw_data:
        i, r0, r1, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
        target_cpu_ratio = r0 * r1

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            if r0 == 0 or r1 == 0:
                expected_cpu_ratio = 0
            else:
                expected_cpu_ratio = 1/int(1/r0) * 1/int(1/r1)

            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(expected_cpu_ratio - real_cpu_ratio) / expected_cpu_ratio
                assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    plot_thpt_per_cpu_ratio(x, y, yerr, out_name)

def plot_estimator_ep1_log_x(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    data = Data([])
    for d in raw_data:
        i, r0, r1, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
        target_cpu_ratio = r0 * r1

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            if r0 == 0 or r1 == 0:
                expected_cpu_ratio = 0
            else:
                expected_cpu_ratio = 1/int(1/r0) * 1/int(1/r1)

            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(expected_cpu_ratio - real_cpu_ratio) / expected_cpu_ratio
                assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    if x[0] == 0: x[0] = x[1] / 10

    plot_thpt_per_cpu_ratio_log_x(x, y, yerr, out_name)

def plot_estimator_ep2(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    data = Data([])
    for d in raw_data:
        i, r0, r1, r2, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
        target_cpu_ratio = r0 * r1 + r0 * r2

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            lhs = 1/int(1/r0) * 1/int(1/r1) if r0 > 0 and r1 > 0 else 0
            rhs = 1/int(1/r0) * 1/int(1/r2) if r0 > 0 and r2 > 0 else 0
            expected_cpu_ratio = lhs + rhs

            # print(f"[r0={r0} r1={r1} r2={r2}] Target {target_cpu_ratio} Expected {expected_cpu_ratio} Real {real_cpu_ratio}")

            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(expected_cpu_ratio - real_cpu_ratio) / expected_cpu_ratio
                # FIXME: uncomment this
                # assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    plot_thpt_per_cpu_ratio(x, y, yerr, out_name)

def plot_estimator_ep2_log_x(
    raw_datas: list[Data],
    out_name: str,
    multiple_data_properties: list[dict] = [],
    color_group: Optional[int] = None,
):
    assert len(raw_datas) == 1
    raw_data = raw_datas[0]

    data = Data([])
    for d in raw_data:
        i, r0, r1, r2, asic_pkts, cpu_pkts, _, xput_pps = d
        real_cpu_ratio = cpu_pkts / asic_pkts
        target_cpu_ratio = r0 * r1 + r0 * r2

        # Experiment limitations.
        # E.g. the target CPU ratio might be 0.6, but 1/0.6 = 1.(6) ~ 1 = 1/1
        if target_cpu_ratio != 1 and real_cpu_ratio == 1:
            target_cpu_ratio = 1
        else:
            lhs = 1/int(1/r0) * 1/int(1/r1) if r0 > 0 and r1 > 0 else 0
            rhs = 1/int(1/r0) * 1/int(1/r2) if r0 > 0 and r2 > 0 else 0
            expected_cpu_ratio = lhs + rhs

            acceptable_error = 0.25 # 25%
            if target_cpu_ratio != 0:
                error = abs(expected_cpu_ratio - real_cpu_ratio) / expected_cpu_ratio
                # FIXME: uncomment this
                # assert error <= acceptable_error

        data.append([ i, real_cpu_ratio, xput_pps ])
    
    x_elem = 1 # cpu ratio
    y_elem = 2 # Throughput pps column

    agg_values = aggregate_values(data, x_elem, y_elem)

    x    = [ v[0] for v in agg_values ]
    y    = [ v[1] for v in agg_values ]
    yerr = [ v[2] for v in agg_values ]

    # Instead of zero, for the log scale. We still show it as zero though, using labels.
    if x[0] == 0: x[0] = x[1] / 10

    plot_thpt_per_cpu_ratio_log_x(x, y, yerr, out_name)

def property_cache_capacity_extractor(name) -> dict:
    pattern = r".*_(\d+)_flows_(\d+)_cache.*"
    
    match = re.match(pattern, name)
    assert match

    flows = int(match.group(1))
    cache = int(match.group(2))

    relative_cache_capacity = cache / flows

    return {
        "relative_cache_capacity": relative_cache_capacity,
    }

def property_loops_extractor(name) -> dict:
    pattern = r".*_(\d+)_loops.*"
    
    match = re.match(pattern, name)
    assert match

    loops = int(match.group(1))

    return {
        "loops": loops,
    }

# file pattern => plotter, out_name, label_builder, color_group
csv_pattern_to_plotter = {
    # "thpt_per_pkt_sz_forwarder.csv": [
    #     (plot_thpt_per_pkt_sz_bps, "switch_perf_bps", None, COLOR_GROUP_SWITCH_ASIC),
    #     (plot_thpt_per_pkt_sz_pps, "switch_perf_pps", None, COLOR_GROUP_SWITCH_ASIC),
    # ],
    # "thpt_per_pkt_sz_send_to_controller.csv": [
    #     (plot_thpt_per_pkt_sz_bps, "switch_cpu_perf_bps", None, COLOR_GROUP_SWITCH_CPU),
    #     (plot_thpt_per_pkt_sz_pps, "switch_cpu_perf_pps", None, COLOR_GROUP_SWITCH_CPU),
    # ],
    # "churn_table_map.csv": [
    #     (plot_throughput_under_churn_bps, "churn_table_map_bps", None, COLOR_GROUP_SWITCH_ASIC),
    #     (plot_throughput_under_churn_pps, "churn_table_map_pps", None, COLOR_GROUP_SWITCH_ASIC),
    #     (plot_churn_thpt_pps_per_cpu_ratio, "xput_cpu_counters_table_map_pps", None, None),
    #     (plot_churn_thpt_pps_per_cpu_ratio_log_x, "xput_cpu_counters_table_map_pps_log_x", None, None),
    # ],
    # "churn_cached_table_map_*.csv": [
    #     (plot_throughput_under_churn_bps, "churn_cached_table_map_bps", property_cache_capacity_extractor, None),
    #     (plot_throughput_under_churn_pps, "churn_cached_table_map_pps", property_cache_capacity_extractor, None),
    # ],
    # "churn_transient_cached_table_map_*.csv": [
    #     (plot_throughput_under_churn_bps, "churn_transient_cached_table_map_bps", property_cache_capacity_extractor, None),
    #     (plot_throughput_under_churn_pps, "churn_transient_cached_table_map_pps", property_cache_capacity_extractor, None),
    # ],
    # "xput_cpu_counters_periodic_cpu_sender.csv": [
    #     (plot_thpt_pps_per_cpu_ratio, "xput_cpu_counters_periodic_cpu_sender", None, None),
    #     (plot_thpt_pps_per_cpu_ratio_log_x, "xput_cpu_counters_periodic_cpu_sender_log_x", None, None),
    # ],

    ########################
    #      Estimators
    ########################
    "xput_cpu_counters_periodic_cpu_sender_0_loops.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio_0_loops", property_loops_extractor, None),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_0_loops_log_x", property_loops_extractor, None),
    ],
    "xput_cpu_counters_periodic_cpu_sender_100_loops.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio_100_loops", property_loops_extractor, None),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_100_loops_log_x", property_loops_extractor, None),
    ],
    "xput_cpu_counters_periodic_cpu_sender_1000_loops.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio_1000_loops", property_loops_extractor, None),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_1000_loops_log_x", property_loops_extractor, None),
    ],
    "xput_cpu_counters_periodic_cpu_sender_10000_loops.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio_10000_loops", property_loops_extractor, None),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_10000_loops_log_x", property_loops_extractor, None),
    ],
    "xput_cpu_counters_periodic_cpu_sender_100000_loops.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio_100000_loops", property_loops_extractor, None),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_100000_loops_log_x", property_loops_extractor, None),
    ],
    "xput_cpu_counters_periodic_cpu_sender_1000000_loops.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio_1000000_loops", property_loops_extractor, None),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_1000000_loops_log_x", property_loops_extractor, None),
    ],
    "xput_cpu_counters_periodic_cpu_sender_10000000_loops.csv": [
        (plot_periodic_sender, "estimator_cpu_ratio_10000000_loops", property_loops_extractor, None),
        (plot_periodic_sender_log_x, "estimator_cpu_ratio_10000000_loops_log_x", property_loops_extractor, None),
    ],
    "churn_table_map.csv": [
        (plot_table_map, "estimator_cpu_ratio_table_map", None, None),
        (plot_table_map_log_x, "estimator_cpu_ratio_table_map_log_x", None, None),
    ],

    ########################################
    #      Execution Plan Estimators
    ########################################

    # "xput_ep_estimators_ep0.csv": [
    #     (plot_estimator_ep0, "ep_estimator_ep0", None, None),
    #     (plot_estimator_ep0_log_x, "ep_estimator_ep0_log_x", None, None),
    # ],
    # "xput_ep_estimators_ep1.csv": [
    #     (plot_estimator_ep1, "ep_estimator_ep1", None, None),
    #     (plot_estimator_ep1_log_x, "ep_estimator_ep1_log_x", None, None),
    # ],
    # "xput_ep_estimators_ep2.csv": [
    #     (plot_estimator_ep2, "ep_estimator_ep2", None, None),
    #     (plot_estimator_ep2_log_x, "ep_estimator_ep2_log_x", None, None),
    # ],
}

def should_skip(name: str):
    fig_file_pdf = Path(PLOTS_DIR / f"{name}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{name}.png")
    should_plot = not fig_file_pdf.exists() or not fig_file_png.exists()
    return not should_plot

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-f", "--force", action="store_true", default=False,
                        help="Regenerate data/plots, even if they already exist")
    
    args = parser.parse_args()

    PLOTS_DIR.mkdir(parents=True, exist_ok=True)

    match = 0
    total = 0

    for csv_pattern in csv_pattern_to_plotter.keys():
        csvs = [ Path(csv) for csv in glob.glob(f"{DATA_DIR}/{csv_pattern}") ]
        csv_filenames = [ csv.stem for csv in csvs ]

        total += len(csv_pattern_to_plotter[csv_pattern])

        if len(csvs) == 0:
            continue
        
        for plotter, out_name, data_properties_extractor, color_group in csv_pattern_to_plotter[csv_pattern]:
            match += 1

            if not args.force and should_skip(out_name):
                print(f"{out_name} (skipped)")
                continue

            print(out_name)
            data = [ csv_parser(csv)[1] for csv in csvs ]

            if data_properties_extractor:
                data_properties = [ data_properties_extractor(csv) for csv in csv_filenames ]
            else:
                data_properties_extractor = []

            plotter(data, out_name, data_properties, color_group)
    
    print()
    print(f"Matched {match}/{total} plots")

if __name__ == "__main__":
    main()
