#!/usr/bin/env python3

import os

from pathlib import Path

from utils.heatmap import *
from utils.parser import parse_heatmap_data_file


CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

NFS = [
    # {
    #     "title": "KVS HHTable",
    #     "data_file": DATA_DIR / "tput_synapse_kvs_hhtable.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_heatmap.pdf",
    # },
    # {
    #     "title": "KVS GuardedMapTable",
    #     "data_file": DATA_DIR / "tput_synapse_kvs_guardedmaptable.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_heatmap.pdf",
    # },
    # {
    #     "title": "KVS MapTable",
    #     "data_file": DATA_DIR / "tput_synapse_kvs_maptable.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_heatmap.pdf",
    # },
    # {
    #     "title": "KVS Cuckoo",
    #     "data_file": DATA_DIR / "tput_synapse_kvs_cuckoo.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_heatmap.pdf",
    # },
    {
        "title": "KVS",
        "data_file": DATA_DIR / "tput_synapse_kvs.csv",
        "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_bps.pdf",
        "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_pps.pdf",
        "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_bps_scatter.pdf",
        "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_pps_scatter.pdf",
        "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_heatmap.pdf",
        "barplot_output_file": PLOTS_DIR / "tput_synapse_kvs_barplot.pdf",
    },
    # {
    #     "title": "Firewall",
    #     "data_file": DATA_DIR / "tput_synapse_fw.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_fw_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_fw_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_fw_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_fw_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_fw_heatmap.pdf",
    #     "barplot_output_file": PLOTS_DIR / "tput_synapse_fw_barplot.pdf",
    # },
    # {
    #     "title": "NAT",
    #     "data_file": DATA_DIR / "tput_synapse_nat.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_nat_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_nat_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_nat_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_nat_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_nat_heatmap.pdf",
    #     "barplot_output_file": PLOTS_DIR / "tput_synapse_nat_barplot.pdf",
    # },
    # {
    #     "title": "PSD",
    #     "data_file": DATA_DIR / "tput_synapse_psd.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_psd_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_psd_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_psd_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_psd_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_psd_heatmap.pdf",
    #     "barplot_output_file": PLOTS_DIR / "tput_synapse_psd_barplot.pdf",
    # },
    # {
    #     "title": "CL",
    #     "data_file": DATA_DIR / "tput_synapse_cl.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_cl_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_cl_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_cl_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_cl_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_cl_heatmap.pdf",
    #     "barplot_output_file": PLOTS_DIR / "tput_synapse_cl_barplot.pdf",
    # },
    # {
    #     "title": "Gallium KVS",
    #     "data_file": DATA_DIR / "tput_gallium_kvs.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_gallium_kvs_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_gallium_kvs_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_gallium_kvs_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_gallium_kvs_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_gallium_kvs_heatmap.pdf",
    #     "barplot_output_file": PLOTS_DIR / "tput_gallium_kvs_barplot.pdf",
    # },
    # {
    #     "title": "Gallium FW",
    #     "data_file": DATA_DIR / "tput_gallium_fw.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_gallium_fw_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_gallium_fw_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_gallium_fw_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_gallium_fw_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_gallium_fw_heatmap.pdf",
    # },
    # {
    #     "title": "Gallium NAT",
    #     "data_file": DATA_DIR / "tput_gallium_nat.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_gallium_nat_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_gallium_nat_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_gallium_nat_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_gallium_nat_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_gallium_nat_heatmap.pdf",
    # },
    # {
    #     "title": "Gallium PSD",
    #     "data_file": DATA_DIR / "tput_gallium_psd.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_gallium_psd_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_gallium_psd_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_gallium_psd_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_gallium_psd_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_gallium_psd_heatmap.pdf",
    # },
    # {
    #     "title": "Gallium CL",
    #     "data_file": DATA_DIR / "tput_gallium_cl.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_gallium_cl_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_gallium_cl_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_gallium_cl_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_gallium_cl_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_gallium_cl_heatmap.pdf",
    # },
]


def plot_barplot_churn_x_axis(data: HeatmapData, file: Path):
    fig, ax = plt.subplots(constrained_layout=True)

    ax.set_ylim(ymin=1, ymax=TPUT_MPPS_MAX)
    ax.set_ylabel("Tput (Mpps)")
    ax.set_yticks(np.arange(0, TPUT_MPPS_MAX + 1, TPUT_MPPS_MAX / 5))

    colors = [
        "#2400D8",
        "#3D87FF",
        "#FF7F00",
        "#FF3D3D",
        "#293132",
    ]

    skew = [0, 0.4, 0.8, 1.2]
    churns = [0, 1_000, 10_000, 100_000, 1_000_000]

    ind = np.arange(len(churns))
    bar_width = 0.15
    pos = ind

    for s, hatch, color in zip(skew, itertools.cycle(hatch_list), itertools.cycle(colors)):
        chosen_workloads = [Key(s=s, churn_fpm=c) for c in churns]

        filtered_data = data.filter(chosen_workloads)

        avg_values = filtered_data.get_avg_values()
        stdev_values = filtered_data.get_stdev_values()

        ys = [avg_values[Key(s=s, churn_fpm=c)].dut_egress_pps for c in churns]
        yerrs = [stdev_values[Key(s=s, churn_fpm=c)].dut_egress_pps for c in churns]

        y_Mpps = [y / 1e6 for y in ys]
        yerr_Mpps = [yerr / 1e6 for yerr in yerrs]

        ax.bar(pos, y_Mpps, bar_width, yerr=yerr_Mpps, label=f"s={s}", alpha=0.99, hatch=hatch, error_kw=dict(lw=1, capsize=1, capthick=0.3), color=color)
        pos = pos + bar_width

    labels = [whole_number_to_label(c) for c in churns]

    ax.set_xlabel("Churn (fpm)")
    ax.set_xticks(ind + 1.5 * bar_width, labels)

    ax.legend(bbox_to_anchor=(0.4, 1.35), loc="upper center", ncols=4, columnspacing=1, handletextpad=0.2, fontsize="small")
    fig.set_size_inches(width / 2, height * 0.5)

    fig_file_pdf = Path(file)
    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf), bbox_inches="tight", pad_inches=0)


def main():
    for nf in NFS:
        DATA_FILE = nf["data_file"]
        BPS_OUTPUT_FILE = nf["bps_output_file"]
        PPS_OUTPUT_FILE = nf["pps_output_file"]
        BPS_SCATTER_OUTPUT_FILE = nf["bps_scatter_output_file"]
        HEATMAP_OUTPUT_FILE = nf["heatmap_output_file"]
        BARPLOT_OUTPUT_FILE = nf["barplot_output_file"]

        heatmap_data = parse_heatmap_data_file(DATA_FILE)
        plot_bps(heatmap_data, BPS_OUTPUT_FILE)
        plot_pps(heatmap_data, PPS_OUTPUT_FILE)
        plot_bps_scatter(heatmap_data, BPS_SCATTER_OUTPUT_FILE)
        plot_heatmap(heatmap_data, HEATMAP_OUTPUT_FILE)
        plot_barplot_churn_x_axis(heatmap_data, PLOTS_DIR / BARPLOT_OUTPUT_FILE)


if __name__ == "__main__":
    main()
