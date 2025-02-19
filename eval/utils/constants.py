#!/usr/bin/env python3

import os
from pathlib import Path

REPO_DIR = Path(os.path.abspath(os.path.dirname(__file__))) / ".." / ".."
EVAL_DIR = REPO_DIR / "eval"
DATA_DIR = EVAL_DIR / "data"

FRONT_PANEL_PORTS = [ p for p in range(1, 33) ]
TG_PORT = 1
DUT_CONNECTED_PORTS = [ p for p in range(3, 33) ]

assert TG_PORT in FRONT_PANEL_PORTS
assert all([ p in FRONT_PANEL_PORTS for p in DUT_CONNECTED_PORTS ])
assert all([ p != TG_PORT for p in DUT_CONNECTED_PORTS ])
