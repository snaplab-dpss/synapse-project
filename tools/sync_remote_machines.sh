#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

CFG_FILE="$SCRIPT_DIR/../eval/experiment_config.toml"

sync() {
    host=$1
    path_to_repo=$2
    echo "*********************************************"
    echo "Synchronizing host $host"
    echo "*********************************************"
    ssh $host "cd $path_to_repo && git reset HEAD --hard && git pull origin main"
}

sync_cfg() {
    host=$1
    path_to_repo=$2

    echo "*********************************************"
    echo "Synchronizing cfg file (target: $host)"
    echo "*********************************************"

    if [ ! -f "$CFG_FILE" ]; then
        echo "Configuration file not found."
        exit 0
    fi

    scp $CFG_FILE $host:$path_to_repo/eval/
}

install_libsycon() {
    host=$1
    path_to_repo=$2

    echo "*********************************************"
    echo "Installing libsycon (target: $host)"
    echo "*********************************************"

    # Touching CMakeLists allows cmake to detect new files (because of glob)
    ssh $host "cd $path_to_repo/libsycon && touch CMakeLists.txt && rm -rf Release Debug && ./install.sh"
}

build_pktgen() {
    host=$1
    path_to_repo=$2

    echo "*********************************************"
    echo "Building pktgen (target: $host)"
    echo "*********************************************"

    # Touching CMakeLists allows cmake to detect new files (because of glob)
    ssh $host "cd $path_to_repo && ./build.sh"
}

sync tofino2-b2 /home/user/synapse
sync graveler /home/fcp/synapse
sync graveler /home/fcp/synapse/eval/deps/pktgen
sync_cfg graveler /home/fcp/synapse
install_libsycon tofino2-b2 /home/user/synapse
build_pktgen graveler /home/fcp/synapse/eval/deps/pktgen