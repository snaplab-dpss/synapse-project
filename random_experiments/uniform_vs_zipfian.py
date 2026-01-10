#!/usr/bin/env python3

import matplotlib.pyplot as plt
import random
import os

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))

SAMPLE_SIZE = 1_000_000
FLOWS = 40_000

EPSILON = 1e-6


def plot_cdf(flows_per_experiment: dict[str, list[int]], out_file: str):
    print(f"Plotting CDF to {out_file}...")

    fig, ax = plt.subplots(figsize=(8, 6))

    for label, flows in flows_per_experiment.items():
        all_flows = set(flows)
        flow_counts = {flow: 0 for flow in all_flows}
        for flow in flows:
            flow_counts[flow] += 1
        sorted_flows = sorted(flow_counts.items(), key=lambda item: item[1], reverse=True)
        total_samples = len(flows)
        total_flows = len(all_flows)

        x = []
        y = []

        cummulative_count = 0
        for i, (flow, count) in enumerate(sorted_flows):
            cummulative_count += count
            x.append(100.0 * (i + 1) / total_flows)
            y.append(cummulative_count / total_samples)

        ax.plot(x, y, label=label)

    ax.set_xlabel("Total flows (%)")
    ax.set_ylabel("CDF")
    ax.set_title("CDF of Flow Distributions")
    ax.grid(True)
    ax.legend()
    ax.set_xlim(0, 100)
    ax.set_ylim(0, 1)

    plt.tight_layout()
    plt.savefig(out_file)
    plt.close()


def plot_hist(flows_per_experiment: dict[str, list[int]], out_file: str):
    print(f"Plotting histogram to {out_file}...")

    fig, axs = plt.subplots(len(flows_per_experiment), 1, figsize=(8, 3 * len(flows_per_experiment)))

    for ax, (label, flows) in zip(axs, flows_per_experiment.items()):
        ax.hist(flows, bins=100, alpha=0.7)
        ax.set_xlabel("Flow ID")
        ax.set_ylabel("Frequency")
        ax.set_title(label)
        ax.grid(True)
        ax.set_xlim(0, FLOWS)

    plt.tight_layout()
    plt.savefig(out_file)
    plt.close()


def uniform_distribution(start: int = 0, end: int = FLOWS - 1, sample_size: int = SAMPLE_SIZE) -> list[int]:
    flows = []
    for i in range(sample_size):
        flow = random.randint(start, end)
        flows.append(flow)
        print(f"Generating uniform flows ({int(100 * (i + 1) / sample_size):3}%)", end="\r")
    print()
    return flows


def zipf_distribution(zipf_param: float, start: int = 0, end: int = FLOWS - 1, sample_size: int = SAMPLE_SIZE) -> list[int]:
    if zipf_param == 0 or zipf_param == 1:
        zipf_param += EPSILON

    flows = []
    # N = FLOWS + 1
    N = end - start + 2
    for i in range(sample_size):
        p = random.random()
        x = N / 2.0

        D = p * (12.0 * (N ** (1.0 - zipf_param) - 1) / (1.0 - zipf_param) + 6.0 - 6.0 * N ** (-zipf_param) + zipf_param - N ** (-1.0 - zipf_param) * zipf_param)

        while True:
            m = x ** (-2 - zipf_param)
            mx = m * x
            mxx = mx * x
            mxxx = mxx * x

            a = 12.0 * (mxxx - 1) / (1.0 - zipf_param) + 6.0 * (1.0 - mxx) + (zipf_param - (mx * zipf_param)) - D
            b = 12.0 * mxx + 6.0 * (zipf_param * mx) + (m * zipf_param * (zipf_param + 1.0))
            newx = max(1.0, x - a / b)

            if abs(newx - x) <= 0.01:
                flow_id = int(newx - 1)
                assert flow_id < FLOWS and "Invalid index"
                flow_id += start
                flows.append(flow_id)
                break

            x = newx

        print(f"Generating zipfian flows s={zipf_param:.1f} ({int(100 * (i + 1) / sample_size):3}%)", end="\r")
    print()
    return flows


if __name__ == "__main__":
    for streams in [1, 4, 30]:
        hist_out_file = os.path.join(SCRIPT_DIR, f"uniform_vs_zipfian_hist_{streams}_streams.pdf")
        cdf_out_file = os.path.join(SCRIPT_DIR, f"uniform_vs_zipfian_cdf_{streams}_streams.pdf")

        experiments = {
            "Uniform": [f for i in range(streams) for f in uniform_distribution(i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
            "Zipf (s=0.0)": [f for i in range(streams) for f in zipf_distribution(0, i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
            "Zipf (s=0.2)": [f for i in range(streams) for f in zipf_distribution(0.2, i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
            "Zipf (s=0.4)": [f for i in range(streams) for f in zipf_distribution(0.4, i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
            "Zipf (s=0.6)": [f for i in range(streams) for f in zipf_distribution(0.6, i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
            "Zipf (s=0.8)": [f for i in range(streams) for f in zipf_distribution(0.8, i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
            "Zipf (s=1.0)": [f for i in range(streams) for f in zipf_distribution(1, i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
            "Zipf (s=1.2)": [f for i in range(streams) for f in zipf_distribution(1.2, i * int(FLOWS / streams), (i + 1) * int(FLOWS / streams) - 1, int(SAMPLE_SIZE / streams))],
        }

        plot_hist(experiments, hist_out_file)
        plot_cdf(experiments, cdf_out_file)
