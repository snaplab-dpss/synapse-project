#!/usr/bin/env python3

import struct
import random
import os
import multiprocessing

from pathlib import Path

from crc import Calculator, Configuration

from utils.plot_config import *

import matplotlib.pyplot as plt

MIN_HASH_SIZE = 1
MAX_HASH_SIZE = 16
TOTAL_FLOWS = 10_000
HASH_SIZES = [ i for i in range(MIN_HASH_SIZE, MAX_HASH_SIZE+1) ]

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
PLOTS_DIR = CURRENT_DIR / "plots"

class Flow:
    def __init__(self, src_ip: int, dst_ip: int, src_port: int, dst_port: int):
        self.src_ip = src_ip
        self.dst_ip = dst_ip
        self.src_port = src_port
        self.dst_port = dst_port
    
    def get_id(self) -> int:
        id = self.src_ip
        id = id << 32
        id = self.dst_ip
        id = id << 32
        id = self.src_port
        id = id << 16
        id = self.dst_port
        return id

class CRC():
    def __init__(
        self,
        width=32,
        poly=0x04C11DB7,
        reflect_in=True,
        xor_in=0xffffffff,
        reflect_out=True,
        xor_out=0xffffffff
    ):
        super().__init__()
        config = Configuration(
            width=width,
            polynomial=poly,
            init_value=xor_in,
            final_xor_value=xor_out,
            reverse_input=reflect_in,
            reverse_output=reflect_out,
        )

        self.calculator = Calculator(config)
    
    def hash(self, flow: Flow, bits: int) -> int:
        stream = struct.pack("!LLHH", flow.src_ip, flow.dst_ip, flow.src_port, flow.dst_port)
        mask = ((1<<bits)-1)
        return self.calculator.checksum(stream) & mask

class Data:
    def __init__(self):
        self.hash_sizes = []
        self.rel_collisions = []

def random_value(bits):
    return random.randint(0, (1 << bits) - 1)

def random_ip():
    return random_value(32)

def random_port():
    return random_value(16)

def build_random_flow() -> Flow:
    src_ip = random_ip()
    dst_ip = random_ip()
    src_port = random_port()
    dst_port = random_port()
    return Flow(src_ip, dst_ip, src_port, dst_port)

def get_collisions(hash_size: int) -> float:
    crc = CRC()
    hashes = set()

    collisions = 0
    print(f"Computing for hash size {hash_size}...")

    for _ in range(TOTAL_FLOWS):
        flow = build_random_flow()
        hash = crc.hash(flow, hash_size)

        if hash in hashes:
            collisions += 1

        hashes.add(hash)
    
    relative_collisions = collisions / TOTAL_FLOWS
    return relative_collisions

def get_collisions_per_hash_size() -> Data:
    cpus = multiprocessing.cpu_count()

    data = Data()

    with multiprocessing.Pool(cpus) as pool:
        relative_collisions = pool.map(get_collisions, HASH_SIZES)
        data.hash_sizes = HASH_SIZES
        data.rel_collisions = relative_collisions
    
    return data

def plot_thpt_per_pkt_sz_pps(
    data: Data,
    out_name: str,
):
    fig_file_pdf = Path(PLOTS_DIR / f"{out_name}.pdf")

    x = data.hash_sizes
    y = data.rel_collisions

    x_label = f"Hash size (bits)"
    y_label = f"Relative \\#collisions"

    d = [{
        "values": y,
    }]

    xtick_labels = [ str(hash_size) for hash_size in x ]

    set_figsize = (width / 2, height / 2)
    fig, ax = plt.subplots()
    
    bar_subplot(
        ax,
        x_label,
        y_label,
        d,
        xtick_labels=xtick_labels,
        y_values_hats=True,
    )

    fig.set_size_inches(*set_figsize)
    fig.tight_layout(pad=0.1)

    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf))

def main():
    data = get_collisions_per_hash_size()
    for h, c in zip(data.hash_sizes, data.rel_collisions):
        print(h, c)

    plot_thpt_per_pkt_sz_pps(data, "hash_size")

if __name__ == "__main__":
    main()
