#!/usr/bin/env python3

import math
import sys
import re
import itertools
import statistics
import os

from pathlib import Path
from typing import Optional, Union, NewType

import matplotlib as mpl
import matplotlib.pyplot as plt

import numpy as np

from pubplot import Document

from natsort import natsorted

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

COLOR_GROUP_SWITCH_ASIC = 0
COLOR_GROUP_SWITCH_CPU = 1
COLOR_GROUP_CPU = 2

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
    '#FF7F00',
    '#654CFF',
    '#E51932',
    '#FFBF7F',
    '#FFFF99',
    '#B2FF8C',
    '#A5EDFF',
    '#CCBFFF',
    '#2ca02c',  # "#32FF00",  # I hate this green, so I changed it... It may
                # not be as color-blind friendly as it was originally but since
                # we also use patterns, it should be fine.
]

hatch_list = ['////////', '\\\\\\\\\\\\\\\\', 'xxxxxx', '-----', '|||||||']

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

Header = NewType("Header", list[str])
Data = NewType("Data", list[list[int | float]])

def parse_csv_elem(elem: str) -> Union[int, float]:
    if re.match(r"^[-\+]?\d+$", elem):
        return int(elem)
    
    try:
        return float(elem)
    except ValueError:
        # So, not an integer
        pass
    
    print(f"Unable to parse csv element {elem}")
    exit(1)

def csv_parser(file: Path) -> tuple[Header, Data]:
    assert file.exists()

    with open(str(file), 'r') as f:
        lines = f.readlines()
        lines = [ l.rstrip().split(',') for l in lines ]
        
        header = Header(lines[0])
        rows   = lines[1:]

        values = [ [ parse_csv_elem(elem) for elem in row ] for row in rows ]

        # Sort data by iteration (1st column)
        values = Data(sorted(values, key=lambda x: x[0]))

        return header, values

def aggregate_values(values: Data,
                     x_elem: int,
                     y_elem: int) -> list[tuple[float,float,float]]:
    new_values = []

    values = Data(sorted(values,  key=lambda x: x[x_elem]))
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
