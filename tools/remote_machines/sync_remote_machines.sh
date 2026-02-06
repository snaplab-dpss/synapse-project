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
    ssh $host "cd $path_to_repo && git reset HEAD --hard && git pull origin main && git submodule update --init --recursive && cd $path_to_repo/deps/pktgen && git pull origin main && cd $path_to_repo/deps/pcap-replay && git pull origin main"
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
    ssh $host "cd $path_to_repo/libsycon && touch CMakeLists.txt && ./install.sh"
}

force_build_pktgen() {
    host=$1
    path_to_repo=$2
    pktgen_dir=$path_to_repo/deps/pktgen
    paths_file=$path_to_repo/paths.sh

    echo "*********************************************"
    echo "Building pktgen (target: $host)"
    echo "*********************************************"

    # Touching CMakeLists allows cmake to detect new files (because of glob)
    ssh $host "cd $pktgen_dir && touch CMakeLists.txt && source $paths_file && ./build.sh"
}

force_build_pcap_replay() {
    host=$1
    path_to_repo=$2
    pcap_replay_dir=$path_to_repo/deps/pcap-replay
    paths_file=$path_to_repo/paths.sh

    echo "*********************************************"
    echo "Building pcap-replay (target: $host)"
    echo "*********************************************"

    # Touching CMakeLists allows cmake to detect new files (because of glob)
    ssh $host "cd $pcap_replay_dir && touch CMakeLists.txt && source $paths_file && ./build.sh"
}

sync tofino1 /root/tessera-project
sync tofino2 /home/user/tessera-project
sync geodude ~/tessera-project
sync graveler ~/tessera-project
install_libsycon tofino2 /home/user/tessera-project
force_build_pktgen geodude ~/tessera-project
force_build_pcap_replay geodude ~/tessera-project