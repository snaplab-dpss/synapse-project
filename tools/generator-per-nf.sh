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
	
	mkdir -p $BDDS_DIR
	mkdir -p $SYNTHESIZED_DIR
	
	if [ ! -f $BDDS_DIR/$nf.bdd ]; then
		pushd $NFS_DIR/$nf/ >/dev/null
			echo "[$nf] Running symbex"
			make symbex
		popd >/dev/null
		$CALL_PATHS_TO_BDD -out $BDDS_DIR/$nf.bdd $NFS_DIR/$nf/klee-last/*.call_path
	fi

	if [ ! -f $BDDS_DIR/$nf.gv ]; then
		echo "[$nf] Generating BDD"
		$BDD_VISUALIZER -in $BDDS_DIR/$nf.bdd -out $BDDS_DIR/$nf.gv
	fi

	if [ ! -f $SYNTHESIZED_DIR/$nf-bdd-analysis.cpp ]; then
		echo "[$nf] Generating BDD analysis code"
		$BDD_TO_C -target bdd-analyzer -in $BDDS_DIR/$nf.bdd -out $SYNTHESIZED_DIR/$nf-bdd-analysis.cpp
	fi

	echo "[$nf] done"
}

export -f run
parallel run ::: \
	"fwd" "nop" "sbridge" "bridge" "pol" "fw" "nat" "cl" "psd" "lb" "hhh" \
	"gallium-fw" "gallium-lb" "gallium-nat" "gallium-proxy"
