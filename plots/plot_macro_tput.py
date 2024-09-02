#!/usr/bin/env python3

import os

from pathlib import Path

from utils.plot_config import *

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

PLOTS_DIR = CURRENT_DIR / "plots"

OUTPUT_FNAME = "macro_throughput"

nfs = [ "KVS", "FW", "NAT", "LB", "CL", "PSD" ]
solutions = [ "Synapse (UNIV1)", "Synapse (UNIV2)", "Synapse (CAIDA)", "Synapse (MAWI)", "Gallium" ]
workloads = [ "UNIV1", "UNIV2", "CAIDA", "MAWI" ]

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

        fig, ax = plt.subplots()

        ax.set_ylim(ymin=0, ymax=100e9)
        ax.set_ylabel("Tput (Gbps)")

        ax.set_yticks(np.arange(0, 101e9, 20e9), labels=np.arange(0, 101, 20))
        ax.set_yticks(np.arange(10e9, 101e9, 20e9), minor=True)

        # LightBlue2DarkBlue10Steps
        colors = [ '#003FFF', '#1965FF', '#3288FF', '#4CA5FF', '#FF3D3D', ]

        pos = ind
        for (sol, throughput_per_workload), hatch, color in zip(data[nf].items(), itertools.cycle(hatch_list), itertools.cycle(colors)):
            y = throughput_per_workload['y']
            yerr = throughput_per_workload['yerr']
            ax.bar(pos, y, bar_width, yerr=yerr,
                   label=sol, alpha=.99, hatch=hatch,
                   error_kw=dict(lw=1, capsize=1, capthick=0.3), color=color)
            pos = pos + bar_width

        ax.set_xticks(ind + (3.0/2)*bar_width, workloads)

        # ax.legend(bbox_to_anchor=(0.5, 1.3), loc='upper center', ncols=2)
        # fig.set_size_inches(width * 0.5, height * 0.75)

        # ax.legend(bbox_to_anchor=(0.5, 1.5), loc='upper center', ncols=2, columnspacing=0.5, prop={'size': 6})
        # fig.set_size_inches(width * 0.4, height * 0.5)

        ax.legend(bbox_to_anchor=(0.45, 1.4), loc='upper center', ncols=3, columnspacing=0.5, prop={'size': 5})
        fig.set_size_inches(width * 0.4, height * 0.5)

        fig.tight_layout(pad=0.1)

        print("-> ", fig_file_pdf)

        plt.savefig(str(fig_file_pdf))

def main():
    data = random_data()
    plot(data)

if __name__ == "__main__":
    main()
