#!/usr/bin/env python3

import argparse
import json
import os

RED = '\033[91m'
RESET = '\033[0m'

def create_vscode_cpp_config(sde_path=None, gurobi_path=None):
    config = {
        "configurations": [
            {
                "name": "Synapse Project",
                "includePath": [
                    # Third party dependencies
                    "${workspaceFolder}/deps/dpdk/x86_64-native-linuxapp-gcc/include",
                    "${workspaceFolder}/deps/llvm/include",
                    "${workspaceFolder}/deps/llvm/tools/clang/include",
                    "${workspaceFolder}/deps/klee/include",
                    "${workspaceFolder}/deps/klee/build/include",
                    "${workspaceFolder}/deps/z3/build/include",

                    # Synapse dependencies
                    "${workspaceFolder}/synapse/libraries",
                    "${workspaceFolder}/synapse/build/_deps/cli11-src/include",
                    "${workspaceFolder}/synapse/build/_deps/tomlplusplus-src/include",
                    "${workspaceFolder}/synapse/build/_deps/json-src/include",

                    # LibSycon and DPDK-NFS
                    "${workspaceFolder}/libsycon/include",
                    "${workspaceFolder}/dpdk-nfs/",
                ],
                "defines": [],
                "compilerPath": "/usr/bin/g++",
                "cStandard": "c11",
                "cppStandard": "c++20",
                "intelliSenseMode": "linux-gcc-x64",
            }
        ],
        "version": 4
    }

    if sde_path:
        include_path = os.path.join(sde_path, "install", "include")
        if not os.path.exists(include_path):
            print(f"{RED}Error: The specified SDE include path does not exist: {include_path}{RESET}", file=os.sys.stderr)
        else:
            config["configurations"][0]["includePath"].append(include_path)
    
    if gurobi_path:
        include_path = os.path.join(gurobi_path, "linux64", "include")
        if not os.path.exists(include_path):
            print(f"{RED}Error: The specified Gurobi include path does not exist: {include_path}{RESET}", file=os.sys.stderr)
        else:
            config["configurations"][0]["includePath"].append(include_path)

    # Print the configuration as JSON
    print(json.dumps(config, indent=4))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create VSCode C++ configuration for Synapse project")

    parser.add_argument("--sde", type=str, required=False, help="Path to the Tofino SDE directory")
    parser.add_argument("--gurobi", type=str, required=False, help="Path to the Gurobi directory")

    args = parser.parse_args()

    create_vscode_cpp_config(sde_path=args.sde, gurobi_path=args.gurobi)