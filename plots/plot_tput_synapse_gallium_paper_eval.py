#!/usr/bin/env python3

import os

from pathlib import Path

from utils.heatmap import *
from utils.parser import parse_heatmap_data_file


CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

# SYSTEM_NAME = "Synapse"
SYSTEM_NAME = "Tessera"

WORKLOADS = [
    ("Gallium CL", DATA_DIR / "tput_gallium_cl.csv"),
    (f"{SYSTEM_NAME} CL", DATA_DIR / "tput_synapse_cl.csv"),
    ("Gallium PSD", DATA_DIR / "tput_gallium_psd.csv"),
    (f"{SYSTEM_NAME} PSD", DATA_DIR / "tput_synapse_psd.csv"),
]

OUTPUT_FILE = PLOTS_DIR / "tput_synapse_gallium_paper_eval.pdf"

KEY_FILTER = lambda key: key.s in [0, 0.4, 0.8, 1.2]


def plot(data: list[tuple[str, HeatmapData]], file: Path, cmap="Blues", show_errors: bool = True):
    fig, axes = plt.subplots(1, len(data), constrained_layout=True, sharey=True)

    keys = list(set([k for _, d in data for k in d.get_avg_values().keys() if KEY_FILTER(k)]))
    all_s = sorted(set([key.s for key in keys]))
    all_churn = sorted(set([key.churn_fpm for key in keys]), reverse=True)
    matrix = np.zeros((len(all_churn), len(all_s)))

    for i, (ax, (nf, nf_data)) in enumerate(zip(axes, data)):
        avg_data = nf_data.get_avg_values()
        stdev_data = nf_data.get_stdev_values()

        for key in keys:
            churn_i = all_churn.index(key.churn_fpm)
            skew_j = all_s.index(key.s)
            matrix[churn_i, skew_j] = avg_data[key].dut_egress_pps / 1e6

        s_labels = [f"{s:.1f}" for s in all_s]
        churn_labels = [f"{whole_number_to_label(c)}" for c in all_churn]

        ax.set_title(nf)
        ax.imshow(matrix, vmin=0, vmax=TPUT_MPPS_MAX, cmap=cmap, aspect="auto")

        ax.set_xticks(range(len(all_s)), labels=s_labels)
        ax.set_yticks(range(len(all_churn)), labels=churn_labels)

        # ax.set_xlabel("Skew (Zipf parameter)")

        # Rotate the tick labels and set their alignment.
        # plt.setp(ax.get_xticklabels(), rotation=45, ha="right", rotation_mode="anchor")

        ax.grid(False)

        ax.spines[:].set_visible(False)
        ax.set_xticks(np.arange(matrix.shape[1] + 1) - 0.5, minor=True)
        ax.set_yticks(np.arange(matrix.shape[0] + 1) - 0.5, minor=True)
        ax.tick_params(which="minor", bottom=False, left=False)
        ax.tick_params(axis="both", length=0)

        if i > 0:
            ax.tick_params(left=False)
        else:
            ax.set_ylabel("Churn (fpm)", fontsize=doc.footnotesize)

        for key in keys:
            churn_i = all_churn.index(key.churn_fpm)
            skew_j = all_s.index(key.s)
            pps = int(avg_data[key].dut_egress_pps)
            err = int(stdev_data[key].dut_egress_pps)

            avg_label = int(pps / 1e6)
            err_label = int(err / 1e6)

            if avg_data[key].label is not None:
                label = avg_data[key].label
                assert label is not None
            else:
                label = f"{avg_label}\n±{err_label}" if show_errors else f"{avg_label}"

            color = "black" if pps < TPUT_MPPS_MAX * 1e6 / 2 else "white"

            text = ax.text(skew_j, churn_i, label, ha="center", va="center", color=color)
            text.set_fontweight("bold")

    fig.supxlabel("Skew (Zipf parameter)", y=-0.05)

    fig.set_size_inches(width_full, height * 1)

    print("-> ", file)
    plt.savefig(str(file), bbox_inches="tight", pad_inches=0)


def main():
    data_workloads = [(nf, parse_heatmap_data_file(file)) for nf, file in WORKLOADS]
    plot(data_workloads, OUTPUT_FILE)


if __name__ == "__main__":
    main()
