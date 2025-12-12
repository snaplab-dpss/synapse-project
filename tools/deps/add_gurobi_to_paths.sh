#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

# Expect 2 arguments: the gurobi installation path and the gurobi license path
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <gurobi_installation_path> <gurobi_license_path>"
    exit 1
fi

gurobi_path="$1"
gurobi_license_path="$2"

if [ ! -d "$gurobi_path" ]; then
    echo "Error: Gurobi installation path '$gurobi_path' does not exist."
    exit 1
fi

if [ ! -f "$gurobi_license_path" ]; then
    echo "Error: Gurobi license file '$gurobi_license_path' does not exist."
    exit 1
fi

# Get full path from relative paths
gurobi_path=$(realpath "$gurobi_path")
gurobi_license_path=$(realpath "$gurobi_license_path")

PATHS_FILE=$SCRIPT_DIR/../../paths.sh

if [ ! -f "$PATHS_FILE" ]; then
    echo "Error: paths.sh file not found at '$PATHS_FILE'. Please run the build_deps.sh script first."
    exit 1
fi

# Add Gurobi paths to paths.sh
if grep -q "^export GUROBI_HOME=" "$PATHS_FILE"; then
    sed -i "/^export GUROBI_HOME=/d" "$PATHS_FILE"
fi
echo "export GUROBI_HOME=${gurobi_path}/linux64" >> "$PATHS_FILE"

if grep -q "^export GRB_LICENSE_FILE=" "$PATHS_FILE"; then
    sed -i "/^export GRB_LICENSE_FILE=/d" "$PATHS_FILE"
fi
echo "export GRB_LICENSE_FILE=${gurobi_license_path}" >> "$PATHS_FILE"