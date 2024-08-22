#!/usr/bin/env python3

import os

from pathlib import Path

from utils.plot_config import *

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

PLOTS_DIR = CURRENT_DIR / "plots"

OUTPUT_FNAME = "macro_throughput"

nfs = [ "FW", "NAT", "KVS", "LB" ]
solutions = [ "Synapse (CAIDA)", "Synapse (UNIV1)", "Synapse (MAWI)", "Gallium" ]
workloads = [ "CAIDA", "UNIV1", "MAWI" ]

def random_data():
    data = {}
    for nf in nfs:
        data[nf] = {}
        for sol in solutions:
            data[nf][sol] = {
                'y': [ np.random.rand() * 100e9 for _ in range(len(workloads)) ],
                'yerr': [ np.random.rand() * 10e9 for _ in range(len(workloads)) ],
            }
    return data

def plot(data: dict):
    ind = np.arange(len(workloads))
    bar_width = 0.15

    for nf in data.keys():
        fig_file_pdf = Path(PLOTS_DIR / f"{OUTPUT_FNAME}_{nf}.pdf")
        fig_file_png = Path(PLOTS_DIR / f"{OUTPUT_FNAME}_{nf}.png")

        fig, ax = plt.subplots()

        ax.set_ylim(ymin=0, ymax=100e9)
        ax.set_ylabel("Throughput (Gbps)")
        ax.set_yticks(np.arange(0, 101e9, 10e9), labels=np.arange(0, 101, 10))

        colors = [ '#2400D8', '#3D87FF', '#99EAFF', '#FF3D3D', ]
        pos = ind
        for (sol, throughput_per_workload), hatch, color in zip(data[nf].items(), itertools.cycle(hatch_list), itertools.cycle(colors)):
            y = throughput_per_workload['y']
            yerr = throughput_per_workload['yerr']
            ax.bar(pos, y, bar_width, yerr=yerr,
                   label=sol, alpha=.99, hatch=hatch,
                   error_kw=dict(lw=1, capsize=1, capthick=0.3), color=color)
            pos = pos + bar_width

        ax.set_xticks(ind + (3.0/2)*bar_width, workloads)

        ax.legend(bbox_to_anchor=(0.5, 1.3), loc='upper center', ncols=2)
        # ax.legend(loc='best')
        fig.set_size_inches(width / 2, height * 0.75)
        fig.tight_layout(pad=0.1)

        print("-> ", fig_file_pdf)
        print("-> ", fig_file_png)

        plt.savefig(str(fig_file_pdf))
        plt.savefig(str(fig_file_png))

def main():
    data = random_data()
    plot(data)

if __name__ == "__main__":
    main()
