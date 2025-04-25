#!/usr/bin/env python3

import subprocess
import sys
import os

from dataclasses import dataclass
from argparse import ArgumentParser
from pathlib import Path
from typing import Optional, Tuple
from itertools import product

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

BDD_DIR = (CURRENT_DIR / ".." / "bdds").resolve()
SYNTHESIZED_DIR = (CURRENT_DIR / ".." / "synthesized").resolve()
PCAP_DIR = (CURRENT_DIR / ".." / "pcaps").resolve()
TOOLS_DIR = (CURRENT_DIR / ".." / "tools").resolve()
PROFILE_DIR = (CURRENT_DIR / ".." / "profiles").resolve()
SYNAPSE_DIR = (CURRENT_DIR / ".." / "synapse").resolve()
SYNAPSE_BUILD_DIR = (SYNAPSE_DIR / "build").resolve()
SYNAPSE_BIN_DIR = (SYNAPSE_BUILD_DIR / "bin").resolve()

DEFAULT_TOTAL_PACKETS = [50_000_000]
DEFAULT_PACKET_SIZE = [250]
DEFAULT_TOTAL_FLOWS = [40_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]

DEVICES = list(range(2, 32))


@dataclass
class NF:
    name: str
    bdd: str
    pcap_generator: str
    clients: list[int]
    fwd_rules: list[Tuple[int, int]]

    def get_pcap_generator(self) -> Path:
        return SYNAPSE_BIN_DIR / self.pcap_generator


def clients_is_every_other_dev():
    return [c for c in DEVICES if c % 2 == 0]


def connect_every_other_dev():
    return [(i, i + 1) for i in DEVICES if i % 2 == 0]


NFs = {
    "echo": NF("echo", "echo.bdd", "pcap-generator-echo", clients=[], fwd_rules=[]),
    "fwd": NF("fwd", "fwd.bdd", "pcap-generator-fwd", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
    "fw": NF("fw", "fw.bdd", "pcap-generator-fw", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
    "nat": NF("nat", "nat.bdd", "pcap-generator-nat", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
    "kvs": NF("kvs", "kvs.bdd", "pcap-generator-kvs", clients=DEVICES, fwd_rules=[]),
    "cl": NF("cl", "cl.bdd", "pcap-generator-cl", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
}

SEQUENTIAL_MODE = True


def panic(msg: str):
    print(f"ERROR: {msg}")
    exit(1)


def assert_bdd(nf: Path):
    if not nf.is_file():
        panic(f'BDD "{nf}" not found')
    if not nf.name.endswith(".bdd"):
        panic(f'BDD "{nf}" is not a BDD file')


def exec_cmd(cmd: str, env_vars: dict[str, str] = {}, cwd: Optional[Path] = None):
    flattened_env_vars = " ".join([f"{k}={v}" for k, v in env_vars.items()])
    print(f"[exec] {flattened_env_vars} {cmd}")

    if SEQUENTIAL_MODE:
        process = subprocess.Popen(
            cmd.split(" "),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=0,
            cwd=cwd,
            env={**env_vars, **dict(list(os.environ.items()))},
        )

        assert process and process.stdout

        while True:
            byte = process.stdout.read(1)
            if byte == b"" and process.poll() is not None:
                break
            if byte:
                sys.stdout.buffer.write(byte)
                sys.stdout.buffer.flush()

        process.wait()
        return_code = process.returncode
        assert return_code == 0, f"Command failed with return code {return_code}"
    else:
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            cwd=cwd,
            env={**env_vars, **dict(list(os.environ.items()))},
        )

        return_code = result.returncode

        if return_code != 0:
            panic(f"Command failed with error: {result.stderr.decode()}")


def build_synapse():
    exec_cmd("./build.sh", cwd=SYNAPSE_DIR)


def get_pcap_base_name(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
) -> str:
    pcap = f"{nf.name}-f{total_flows}-c{churn_fpm}-{'unif' if zipf_param == 0.0 else 'zipf'}"
    if zipf_param != 0.0:
        s = str(int(zipf_param) if int(zipf_param) == zipf_param else zipf_param).replace(".", "_")
        pcap += s
    return pcap


def generate_pcap(
    nf: NF,
    total_packets: int,
    packet_size: int,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
):
    if nf.fwd_rules:
        dev_list = " ".join([f"{src},{dst}" for src, dst in nf.fwd_rules])
    else:
        dev_list = " ".join([f"{dev}" for dev in DEVICES])

    cmd = f"{nf.get_pcap_generator()}"
    cmd += f" --out {PCAP_DIR}"
    cmd += f" --packets {total_packets}"
    cmd += f" --flows {total_flows}"
    cmd += f" --packet-size {packet_size}"
    cmd += f" --churn {churn_fpm}"
    if zipf_param == 0.0:
        cmd += " --traffic uniform"
    else:
        cmd += f" --traffic zipf --zipf-param {zipf_param}"
    cmd += f" --devs {dev_list}"
    cmd += " --seed 0"

    exec_cmd(cmd)


def generate_profiler(nf: NF):
    profiler_name = f"{nf.name}-profiler.cpp"

    synthesize_profiler_cmd = f"{SYNAPSE_BIN_DIR / 'bdd-synthesizer'}"
    synthesize_profiler_cmd += f" --target profiler"
    synthesize_profiler_cmd += f" --in {BDD_DIR / nf.bdd}"
    synthesize_profiler_cmd += f" --out {SYNTHESIZED_DIR / profiler_name}"
    exec_cmd(synthesize_profiler_cmd)


def generate_profile(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
):
    profiler_name = f"{nf.name}-profiler"
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    report = f"{pcap_base_name}.json"
    dot = f"{pcap_base_name}.dot"

    compile_profiler_cmd = f"make -f {TOOLS_DIR / 'Makefile.dpdk'}"
    exec_cmd(compile_profiler_cmd, env_vars={"NF": f"{SYNTHESIZED_DIR / profiler_name}.cpp"})

    profile_cmd = f"{SYNTHESIZED_DIR / 'build' / profiler_name}"
    profile_cmd += f" {PROFILE_DIR / report}"
    for warmup_dev in nf.clients:
        warmup_pcap = f"{pcap_base_name}-dev{warmup_dev}-warmup.pcap"
        profile_cmd += f" --warmup {warmup_dev}:{PCAP_DIR / warmup_pcap}"
    for dev in DEVICES:
        pcap = f"{pcap_base_name}-dev{dev}.pcap"
        profile_cmd += f" {dev}:{PCAP_DIR / pcap}"

    exec_cmd(profile_cmd)
    print(f"  -> {report}")

    profile_visualizer_cmd = f"{SYNAPSE_BIN_DIR / 'bdd-visualizer'}"
    profile_visualizer_cmd += f" --in {BDD_DIR / nf.bdd}"
    profile_visualizer_cmd += f" --profile {PROFILE_DIR / report}"
    profile_visualizer_cmd += f" --out {PROFILE_DIR / dot}"

    exec_cmd(profile_visualizer_cmd)
    print(f"  -> {dot}")


if __name__ == "__main__":
    parser = ArgumentParser(description="Profiler helper script. This will generate the pcaps and profiles for all the possible combinations of the provided parameters")

    parser.add_argument("--nf", type=str, choices=NFs.keys(), required=True, help="Target NFs to profile (BDD)")
    parser.add_argument("--total-packets", type=int, nargs="+", default=DEFAULT_TOTAL_PACKETS, help="Total packets to send")
    parser.add_argument("--packet-size", type=int, nargs="+", default=DEFAULT_PACKET_SIZE, help="Packet size (bytes)")
    parser.add_argument("--total-flows", type=int, nargs="+", default=DEFAULT_TOTAL_FLOWS, help="Total flows to generate")
    parser.add_argument("--zipf-params", type=float, nargs="+", default=DEFAULT_ZIPF_PARAMS, help="Zipf parameters")
    parser.add_argument("--churn", type=int, nargs="+", default=DEFAULT_CHURN_FPM, help="Churn rate (fpm)")
    parser.add_argument("--skip-pcap-generation", action="store_true", default=False, help="Skip pcap generation")
    parser.add_argument("--skip-profiler-generation", action="store_true", default=False, help="Skip profiler generation")
    parser.add_argument("--dry-run", action="store_true", default=False)

    args = parser.parse_args()

    combinations = list(
        product(
            args.total_packets,
            args.packet_size,
            args.total_flows,
            args.zipf_params,
            args.churn,
        )
    )

    print("============ Requested Configuration ============")
    print(f"Target NF:     {args.nf}")
    print(f"Total packets: {args.total_packets}")
    print(f"Packet sizes:  {args.packet_size}")
    print(f"Total flows:   {args.total_flows}")
    print(f"Zipf params:   {args.zipf_params}")
    print(f"Churn rates:   {args.churn}")
    print("-------------------------------------------------")
    print(f"Skip pcap generation: {args.skip_pcap_generation}")
    print(f"Skip profiler generation: {args.skip_profiler_generation}")
    print(f"Total combinations: {len(combinations)}")
    print("=================================================")

    nf = NFs[args.nf]
    bdd = BDD_DIR / nf.bdd
    assert_bdd(bdd)

    if args.dry_run:
        exit(0)

    build_synapse()

    for total_packets, packet_size, total_flows, zipf_param, churn in combinations:
        print(f"Generating pcap for {total_packets} packets, {packet_size} bytes, {total_flows} flows, zipf_param={zipf_param}, churn={churn}")

        if not args.skip_pcap_generation:
            generate_pcap(
                nf,
                total_packets,
                packet_size,
                total_flows,
                zipf_param,
                churn,
            )

        if not args.skip_profiler_generation:
            generate_profiler(nf)

        generate_profile(nf, total_flows, zipf_param, churn)
