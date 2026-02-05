from pathlib import Path
from dataclasses import dataclass
from statistics import mean, stdev, geometric_mean

RESOURCES_COLUMNS = [
    "Stage Number",
    "Exact Match Input xbar",
    "Ternary Match Input xbar",
    "Hash Bit",
    "Hash Dist Unit",
    "Gateway",
    "SRAM",
    "Map RAM",
    "TCAM",
    "VLIW Instr",
    "Meter ALU",
    "Stats ALU",
    "Stash",
    "Exact Match Search Bus",
    "Exact Match Result Bus",
    "Tind Result Bus",
    "Action Data Bus Bytes",
    "8-bit Action Slots",
    "16-bit Action Slots",
    "32-bit Action Slots",
    "Logical TableID",
]

MAX_STAGES = 20


# All are in between [0.0, 1.0], representing relative occupancy.
@dataclass
class Resources:
    stages: float
    sram: float
    vliw: float
    match_xbar: float
    logical_table_ids: float

    def get_total_avg(self) -> float:
        return mean([self.stages, self.sram, self.vliw, self.match_xbar])

    def get_total_geometric_mean(self) -> float:
        return geometric_mean([self.stages, self.sram, self.vliw, self.match_xbar])

    def get_total_stddev(self) -> float:
        return stdev([self.stages, self.sram, self.vliw, self.match_xbar])

    def compare_to(self, other: "Resources") -> "Resources":
        return Resources(
            stages=self.stages / other.stages if other.stages != 0 else float("inf"),
            sram=self.sram / other.sram if other.sram != 0 else float("inf"),
            vliw=self.vliw / other.vliw if other.vliw != 0 else float("inf"),
            match_xbar=self.match_xbar / other.match_xbar if other.match_xbar != 0 else float("inf"),
            logical_table_ids=self.logical_table_ids / other.logical_table_ids if other.logical_table_ids != 0 else float("inf"),
        )

    @staticmethod
    def average(resources_list: list["Resources"]) -> "Resources":
        return Resources(
            stages=mean([r.stages for r in resources_list]),
            sram=mean([r.sram for r in resources_list]),
            vliw=mean([r.vliw for r in resources_list]),
            match_xbar=mean([r.match_xbar for r in resources_list]),
            logical_table_ids=mean([r.logical_table_ids for r in resources_list]),
        )

    def __str__(self):
        return f"Stages: {100*self.stages:5.2f}%, SRAM: {100*self.sram:5.2f}%, VLIW: {100*self.vliw:5.2f}%, Match Xbar: {100*self.match_xbar:5.2f}%"


def parse_tofino_resources_file(filepath: Path):
    resources = Resources(stages=0, sram=0, vliw=0, match_xbar=0, logical_table_ids=0)

    with open(filepath, "r") as f:
        # print(f"Parsing Tofino resources file: {filepath}")
        lines = f.readlines()

        for line in lines:
            if "Average" in line:
                cols = [c for c in line.rstrip().replace(" ", "").split("|") if c not in [""]]
                assert len(cols) == len(RESOURCES_COLUMNS), f"Unexpected number of columns in resources file {filepath}: {len(cols)} vs {len(RESOURCES_COLUMNS)}"

                resources.sram = float(cols[6].replace("%", "")) / 100.0
                resources.vliw = float(cols[9].replace("%", "")) / 100.0
                resources.match_xbar = float(cols[1].replace("%", "")) / 100.0
                resources.logical_table_ids = float(cols[20].replace("%", "")) / 100.0
                break

        for i in range(len(lines) - 1, -1, -1):
            line = lines[i]
            if line == "\n" or "---------" in line:
                continue
            cols = [c for c in line.rstrip().replace(" ", "").split("|") if c not in [""]]
            resources.stages = float(cols[1]) / MAX_STAGES
            break

    return resources


def calculate_lines_of_p4_code(p4_file: Path) -> int:
    # print(f"Calculating lines of P4 code in file: {p4_file}")
    loc = 0
    with open(p4_file, "r") as f:
        lines = f.readlines()
        for line in lines:
            # We don't deal with block comments, too much hassle for now.
            if line.strip().startswith("//"):
                continue
            if line.strip() == "":
                continue
            loc += 1
    return loc
