#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

CFG_FILE="$SCRIPT_DIR/../eval/experiment_config.toml"

# Root of the local checkout (this script lives in tools/remote_machines/).
REPO_ROOT=$(cd -- "$SCRIPT_DIR/../.." &> /dev/null && pwd)

# Push the local checkout to a host with rsync over SSH. This mirrors the local
# tree (the source of truth) instead of pulling from GitHub, so it only needs
# SSH access to the host and never hits GitHub's unauthenticated rate limits.
#
# --filter=':- .gitignore' makes rsync honor every .gitignore in the tree (the
# top-level one and each submodule's), and --exclude='.git' skips all git
# metadata. Together they transfer exactly the git-tracked source, including
# submodule contents, while leaving each host's ignored, machine-specific files
# untouched: paths.sh (absolute paths differ per host), build/ outputs, the
# Gurobi license, the Barefoot SDE, pcaps, logs, etc. Excluded files are also
# protected from --delete, so mirroring never removes them.
sync() {
    host=$1
    path_to_repo=$2
    echo "*********************************************"
    echo "Synchronizing host $host"
    echo "*********************************************"
    # rsync must exist on the remote too; install it if it is missing.
    if ! ssh "$host" 'command -v rsync' > /dev/null 2>&1; then
        echo "rsync not found on $host, installing it..."
        ssh "$host" 'if [ "$(id -u)" -eq 0 ]; then SUDO=; else SUDO=sudo; fi; $SUDO apt-get update -qq && $SUDO apt-get install -y rsync'
    fi
    rsync -az --delete --partial -h --info=progress2 \
        --exclude='.git' \
        --filter=':- .gitignore' \
        "$REPO_ROOT/" "$host:$path_to_repo/"
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

sync tofino1 /root/synapse-project
sync tofino2 /home/user/synapse-project
sync geodude '~/synapse-project'
sync graveler '~/synapse-project'
install_libsycon tofino2 /home/user/synapse-project
force_build_pktgen geodude '~/synapse-project'
force_build_pcap_replay geodude '~/synapse-project'