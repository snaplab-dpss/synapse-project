#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from utils.constants import *
from utils.kill_hosts import kill_hosts


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts(config)


if __name__ == "__main__":
    main()
