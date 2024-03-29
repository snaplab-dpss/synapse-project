#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
REPO_DIR="$SCRIPT_DIR/.."

MODE="Release"
# MODE="Debug"

BDDS_DIR="$REPO_DIR/bdds"
NFS_DIR="$REPO_DIR/dpdk-nfs"
KLEE_BINS_DIR="$KLEE_DIR/$MODE/bin"

CALL_PATHS_TO_BDD="$KLEE_BINS_DIR/call-paths-to-bdd"

run() {
	nf=$1
	
	pushd $NFS_DIR/$nf/ >/dev/null
		make symbex
	popd >/dev/null

	$CALL_PATHS_TO_BDD \
		-out $BDDS_DIR/$nf.bdd \
		-gv $BDDS_DIR/$nf.gv \
		$NFS_DIR/$nf/klee-last/*.call_path
}

run "fwd"
run "nop"
run "sbridge"
run "bridge"
run "pol"
run "fw"
run "nat"
run "cl"
run "psd"
run "lb"
run "hhh"
run "gallium-fw"
run "gallium-lb"
run "gallium-nat"
run "gallium-proxy"
