#!/usr/bin/env python3

import argparse
import math
import sys
import re
import itertools
import statistics
import os
import glob

from pathlib import Path
from typing import Any, Callable, Optional, Union

import matplotlib as mpl
import matplotlib.pyplot as plt

import numpy as np

from pubplot import Document

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
PLOTS_DIR = CURRENT_DIR / "plots"

SYSTEM_NAME = "SyNAPSE"

NFs = {
    "proxy": "Proxy",
    "nat": "NAT",
    "cl": "CL",
    "psd": "PSD",
    "fw": "FW",
    "lb": "LB",
    "hhh": "HHH",
}

HEURISTICS = {
    "max_tpt": "Max. Tpt.",
    "max_cpt": "Max. Comp.",
    "min_cpu": "Min. CPU",
}

acm_acmsmall = {
    'documentclass': 'acmart',
    'document_options': 'acmsmall'
}

inches_per_pt = 1.0 / 72.27
golden_ratio = (1.0 + math.sqrt(5.0)) / 2.0
doc = Document(acm_acmsmall)
width = float(doc.columnwidth)
height = width / golden_ratio
width = width * inches_per_pt
height = height * inches_per_pt * 0.8
figsize = (width, height)

width_third = doc.textwidth / 3
height_third = width_third / golden_ratio
width_third = width_third * inches_per_pt
height_third = height_third * inches_per_pt
figsize_third = (width_third, height_third)

tight_layout_pad = 0.21
linewidth = 1
elinewidth = 0.5
capsize = 1
markersize = 2.5
markeredgewidth = 0.5
capthick = 0.5

# This is "colorBlindness::PairedColor12Steps" from R.
# Check others here: https://r-charts.com/color-palettes/#discrete
palette = [
    '#19B2FF',
    '#2ca02c',  # "#32FF00",  # I hate this green, so I changed it... It may
                # not be as color-blind friendly as it was originally but since
                # we also use patterns, it should be fine.
    '#FF7F00',
    '#654CFF',
    '#E51932',
    '#FFBF7F',
    '#FFFF99',
    '#B2FF8C',
    '#A5EDFF',
    '#CCBFFF'
]

hatch_list = ['////////', '-----', '+++++++', '|||||||']

markers_list = ['.', 'x', 'o', 'v', 's', '*']

linestyle = [
    (0, (1, 0)),
    (0, (4, 1)),
    (0, (2, 0.5)),
    (0, (1, 0.5)),
    (0, (0.5, 0.5)),
    (0, (4, 0.5, 0.5, 0.5)),
    (0, (3, 1, 1, 1)),
    (0, (8, 1)),
    (0, (3, 1, 1, 1, 1, 1)),
    (0, (3, 1, 1, 1, 1, 1, 1, 1)),
]

prop_cycle = mpl.cycler(color=palette) + mpl.cycler(linestyle=linestyle)

style = {
    # Line styles.
    'axes.prop_cycle': prop_cycle,

    # Grid.
    'grid.linewidth': 0.2,
    'grid.alpha': 0.4,
    'axes.grid': True,
    'axes.axisbelow': True,

    'axes.linewidth': 0.2,

    # Ticks.
    'xtick.major.width': 0.2,
    'ytick.major.width': 0.2,
    'xtick.minor.width': 0.2,
    'ytick.minor.width': 0.2,

    # Font.
    # You can use any of the predefined LaTeX font sizes here as well as
    # "caption", to match the caption size.
    'font.family': 'serif',
    'font.size': doc.footnotesize,
    'axes.titlesize': doc.footnotesize,
    'axes.labelsize': doc.small,
    "figure.labelsize": doc.footnotesize,
    'legend.fontsize': doc.scriptsize,
    'xtick.labelsize': doc.footnotesize,
    'ytick.labelsize': doc.footnotesize,

    'patch.linewidth': 0.2,

    'figure.dpi': 1000,

    'text.usetex': True,

    "text.latex.preamble": """
        \\usepackage[tt=false,type1=true]{libertine}
        \\usepackage[varqu]{zi4}
        \\usepackage[libertine]{newtxmath}
    """,
}

# Apply style globally.
for k, v in style.items():
    mpl.rcParams[k] = v

def get_unit_multiplier(value) -> tuple[float,str]:
    order = math.log(value, 10)

    if order <= -6:
        return 1e-9, "n"
    
    if order <= -3:
        return 1e-6, "u"
    
    if order <= 0:
        return 1e-3, "m"

    if order <= 3:
        return 1, ""
    
    if order <= 6:
        return 1e3, "K"
    
    if order <= 9:
        return 1e6, "M"
    
    return 1e9, "G"

def csv_parser(file: Path) -> tuple[list[str], list[list[Union[int, float]]]]:
    assert file.exists()

    with open(str(file), 'r') as f:
        lines = f.readlines()
        lines = [ l.rstrip().split(',') for l in lines ]
        
        header = lines[0]
        rows   = lines[1:]

        values = []

        for row in rows:
            cols = [ int(c) if '.' not in c else float(c) for c in row ]
            values.append(cols)
        
        # Sort data by iteration (1st column)
        values = sorted(values, key=lambda x: x[0])

        return header, values

def aggregate_values(values: list[list[Union[int, float]]],
                     x_elem: int,
                     y_elem: int) -> list[tuple[float,float,float]]:
    new_values = []

    values = sorted(values,  key=lambda x: x[x_elem])
    values_grouped = [ list(v[1]) for v in itertools.groupby(values, key=lambda x: x[x_elem]) ]

    for vv in values_grouped:
        x_values = [ v[x_elem] for v in vv ]
        y_values = [ v[y_elem] for v in vv ]

        # Assert that they have the same x value
        assert len(set(x_values)) == 1

        x = x_values[0]
        y = y_values[0]
        yerr = 0

        if len(y_values) > 1:
            y = statistics.mean(y_values)
            yerr = statistics.stdev(y_values)
        
        new_values.append((x, y, yerr))

    return new_values

def bar_subplot(ax,
                xlabel: str,
                ylabel: str,
                data: list[dict],
                xtick_labels: Optional[list] = None,
                width_scale: float = 0.7,
                set_palette: Optional[list[str]] = None,
                set_hatch_list: Optional[list[str]] = None,
                **kwargs) -> None:
    x = None
    nb_catgs = len(data)
    bar_width = width_scale/nb_catgs  # The width of the bars.
    offset = bar_width * (1 - nb_catgs) / 2

    if set_palette is None:
        set_palette = palette

    if set_hatch_list is None:
        set_hatch_list = hatch_list

    for d, color, hatch in zip(data, itertools.cycle(set_palette), itertools.cycle(set_hatch_list)):
        if x is None:
            x = np.arange(len(d['values']))  # The xtick_labels locations.

        # Default options.
        options = {
            "fill": False,
            "hatch": hatch,
            "edgecolor": color,
            "error_kw": dict(elinewidth=elinewidth, capsize=capsize,
                             capthick=capthick),
        }
        options.update(kwargs)

        ax.bar(
            x + offset,
            d["values"],
            bar_width,
            yerr=d["errors"],
            label=d["label"] if "label" in d else None,
            **options
        )

        offset += bar_width

    if xlabel is not None:
        ax.set_xlabel(xlabel)
    if ylabel is not None:
        ax.set_ylabel(ylabel)

    if xtick_labels is None:
        ax.set_xticks([])
    else:
        ax.set_xticks(x)
        ax.set_xticklabels(xtick_labels)

    ax.tick_params(axis='both', length=0)
    ax.grid(visible=False, axis='x')

def plot_subplot(ax,
                 xlabel: str,
                 ylabel: str,
                 data: list[dict],
                 set_palette: Optional[list[str]] = None,
                 set_markers_list: Optional[list[str]] = None,
                 lines_only: bool = False,
                 **kwargs) -> None:
    if set_palette is None:
        set_palette = palette

    if set_markers_list is None:
        set_markers_list = markers_list

    for d, color, marker in zip(data, itertools.cycle(set_palette), itertools.cycle(markers_list)):
        # Default options.
        options = {
            "linewidth": linewidth,
            "capsize": capsize,
            "capthick": capthick,
        }

        if not lines_only:
            options.update({
                "marker": marker,
                "markersize": markersize,
                "markeredgewidth": markeredgewidth,
                "markerfacecolor": color,
            })

        options.update(kwargs)

        ax.errorbar(
            d["x"],
            d["y"],
            yerr=None if lines_only else d["yerr"],
            label=d["label"] if "label" in d else None,
            **options
        )

    if xlabel is not None:
        ax.set_xlabel(xlabel)
    if ylabel is not None:
        ax.set_ylabel(ylabel)

    ax.tick_params(axis='both', length=0)
    ax.grid(visible=False, axis='x')

def plot_throughput_under_churn_pps(
    data: list[list[Union[int, float]]],
    out_name: str,
    generate_png: bool,
):
    x_elem = 1 # churn column
    x_label = "Churn (fpm)"

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
        "x": x,
        "y": y,
        "yerr": yerr,
    }]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(ax, x_label, y_label, d, lines_only=False)

    ax.set_xscale("symlog")

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_throughput_under_churn_bps(
    data: list[list[Union[int, float]]],
    out_name: str,
    generate_png: bool,
):
    x_elem = 1 # churn column
    x_label = "Churn (fpm)"

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
        "x": x,
        "y": y,
        "yerr": yerr,
    }]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()

    plot_subplot(ax, x_label, y_label, d, lines_only=False)

    ax.set_xscale("symlog")

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_thpt_per_pkt_sz_bps(
    data: list[list[Union[int, float]]],
    out_name: str,
    generate_png: bool,
):
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
    bar_subplot(ax, x_label, y_label, d, xtick_labels=xtick_labels)

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

def plot_thpt_per_pkt_sz_pps(
    data: list[list[Union[int, float]]],
    out_name: str,
    generate_png: bool,
):
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
    bar_subplot(ax, x_label, y_label, d, xtick_labels=xtick_labels)

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    plt.savefig(str(fig_file_pdf))

    if generate_png:
        plt.savefig(str(fig_file_png))

csv_to_plotter = {
    "thpt_per_pkt_sz_forwarder": [
        ("switch_perf_bps", plot_thpt_per_pkt_sz_bps),
        ("switch_perf_pps", plot_thpt_per_pkt_sz_pps),
    ],
    "thpt_per_pkt_sz_send_to_controller": [
        ("switch_cpu_perf_bps", plot_thpt_per_pkt_sz_bps),
        ("switch_cpu_perf_pps", plot_thpt_per_pkt_sz_pps),
    ],
    "churn_table_map": [
        ("churn_table_map_bps", plot_throughput_under_churn_bps),
        ("churn_table_map_pps", plot_throughput_under_churn_pps),
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

    csv_files = [ Path(f) for f in glob.glob(f"{DATA_DIR}/*.csv") ]
    for csv_file in csv_files:
        csv_name = csv_file.stem

        if csv_name not in csv_to_plotter:
            print(f"WARNING: {csv_name} has no plotter assigned. Skipping.")
            continue
        
        for out_name, plotter in csv_to_plotter[csv_name]:
            if not args.force and should_skip(out_name, args.png):
                print(f"{csv_name} -> {out_name} (skipped)")
                continue

            print(f"{csv_name} -> {out_name}")
            _, data = csv_parser(csv_file)
            plotter(data, out_name, args.png)

if __name__ == "__main__":
    main()
