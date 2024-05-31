#!/bin/bash

set -euo pipefail

export SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
export REPO_DIR="$SCRIPT_DIR/.."

export BDDS_DIR="$REPO_DIR/bdds"
export SYNTHESIZED_DIR="$REPO_DIR/synthesized"
export NFS_DIR="$REPO_DIR/dpdk-nfs"
export KLEE_BINS_DIR="$KLEE_BUILD_PATH/bin"

export CALL_PATHS_TO_BDD="$KLEE_BINS_DIR/call-paths-to-bdd"
export BDD_VISUALIZER="$KLEE_BINS_DIR/bdd-visualizer"
export BDD_TO_C="$KLEE_BINS_DIR/bdd-to-c"

run() {
	nf=$1
	
	echo "$nf..."

	mkdir -p $BDDS_DIR
	mkdir -p $SYNTHESIZED_DIR
	
	pushd $NFS_DIR/$nf > /dev/null
		make symbex 2>/dev/null
	popd > /dev/null
	
	$CALL_PATHS_TO_BDD -out $BDDS_DIR/$nf.bdd $NFS_DIR/$nf/klee-last/*.call_path 2>/dev/null
	$BDD_VISUALIZER -in $BDDS_DIR/$nf.bdd -out $BDDS_DIR/$nf.dot 2>/dev/null
	# $BDD_TO_C -target bdd-analyzer -in $BDDS_DIR/$nf.bdd -out $SYNTHESIZED_DIR/$nf-bdd-analysis.cpp > /dev/null
	# $BDD_TO_C -target seq -in $BDDS_DIR/$nf.bdd -out $SYNTHESIZED_DIR/$nf.cpp > /dev/null
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
run "kvstore"