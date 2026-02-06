from dataclasses import dataclass
from pathlib import Path
from dacite import from_dict

import json


@dataclass(frozen=True)
class GlobalStats:
    num_phase1_speculations: int
    num_phase2_speculations: int
    num_speculated_modules: int
    num_execution_plans_generated: int
    time_per_speculation_us: float
    time_per_instantiation_us: float


@dataclass(frozen=True)
class HeuristicMeta:
    name: str
    description: str


@dataclass(frozen=True)
class Heuristic:
    meta: list[HeuristicMeta]
    score: list[int]


@dataclass(frozen=True)
class Implementation:
    addr: int
    implementation: str


@dataclass(frozen=True)
class AvgNodeChildren:
    avg: float
    node_id: int


@dataclass(frozen=True)
class SearchMeta:
    avg_bdd_size: int
    avg_children_per_node: list[AvgNodeChildren]
    backtracks: int
    branching_factor: float
    elapsed_time_seconds: float
    finished_eps: int
    ss_size: int
    steps: int
    total_ss_size_estimation: float
    unfinished_eps: int


@dataclass(frozen=True)
class TesseraCompilationReport:
    bdd: str
    global_stats: GlobalStats
    heuristic: Heuristic
    implementations: list[Implementation]
    name: str
    profile_file: str
    search_meta: SearchMeta
    seed: int
    targets_config_file: str
    tput_estimation_bps: int
    tput_estimation_pps: int


def parse_tessera_compilation_report(filepath: Path) -> TesseraCompilationReport:
    with open(filepath, "r") as f:
        data = json.load(f)
        return from_dict(data_class=TesseraCompilationReport, data=data)
