#!/usr/bin/env python3

import matplotlib.pyplot as plt
import random
import os

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))

SAMPLE_SIZE = 1_000_000
FLOWS = 40_000
HIST_OUT_FILE = os.path.join(SCRIPT_DIR, "uniform_vs_zipfian_hist.pdf")

EPSILON = 1e-6


def plot_hist(flows_per_experiment: dict[str, list[int]]):
    print("Plotting histogram...")

    fig, axs = plt.subplots(len(flows_per_experiment), 1, figsize=(8, 3 * len(flows_per_experiment)))

    for ax, (label, flows) in zip(axs, flows_per_experiment.items()):
        ax.hist(flows, bins=100, alpha=0.7)
        ax.set_xlabel("Flow ID")
        ax.set_ylabel("Frequency")
        ax.set_title(label)
        ax.grid(True)
        ax.set_xlim(0, FLOWS)

    plt.tight_layout()
    plt.savefig(HIST_OUT_FILE)
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

    experiments = {
        "Uniform": [f for i in range(4) for f in uniform_distribution(i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
        "Zipf (s=0)": [f for i in range(4) for f in zipf_distribution(0, i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
        "Zipf (s=0.2)": [f for i in range(4) for f in zipf_distribution(0.2, i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
        "Zipf (s=0.4)": [f for i in range(4) for f in zipf_distribution(0.4, i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
        "Zipf (s=0.6)": [f for i in range(4) for f in zipf_distribution(0.6, i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
        "Zipf (s=0.8)": [f for i in range(4) for f in zipf_distribution(0.8, i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
        "Zipf (s=1.0)": [f for i in range(4) for f in zipf_distribution(1, i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
        "Zipf (s=1.2)": [f for i in range(4) for f in zipf_distribution(1.2, i * FLOWS, (i + 1) * FLOWS - 1, int(SAMPLE_SIZE / 4))],
    }

    # experiments = {
    #     "Uniform": uniform_distribution(),
    #     "Zipf (s=0)": zipf_distribution(0),
    #     "Zipf (s=0.2)": zipf_distribution(0.2),
    #     "Zipf (s=0.4)": zipf_distribution(0.4),
    #     "Zipf (s=0.6)": zipf_distribution(0.6),
    #     "Zipf (s=0.8)": zipf_distribution(0.8),
    #     "Zipf (s=1.0)": zipf_distribution(1.0001),
    #     "Zipf (s=1.2)": zipf_distribution(1.2),
    # }

    plot_hist(experiments)
