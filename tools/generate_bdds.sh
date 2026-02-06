#!/bin/bash

set -euo pipefail

export SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
export REPO_DIR=$(realpath "$SCRIPT_DIR/..")

export BDDS_DIR="$REPO_DIR/bdds"
export SYNTHESIZED_DIR="$REPO_DIR/synthesized"
export NFS_DIR="$REPO_DIR/dpdk-nfs"
export TESSERA_DIR="$REPO_DIR/tessera"

export TESSERA_BINS_DIR="$TESSERA_DIR/build/bin"

export CALL_PATHS_TO_BDD="$TESSERA_BINS_DIR/call-paths-to-bdd"
export BDD_VISUALIZER="$TESSERA_BINS_DIR/bdd-visualizer"
export BDD_INSPECTOR="$TESSERA_BINS_DIR/bdd-inspector"

run() {
	nf=$1
	
	echo "$nf..."

	mkdir -p $BDDS_DIR
	mkdir -p $SYNTHESIZED_DIR
	
	pushd $NFS_DIR/$nf > /dev/null
		make symbex 2>/dev/null
	popd > /dev/null
	
	$CALL_PATHS_TO_BDD --out $BDDS_DIR/$nf.bdd $NFS_DIR/$nf/klee-last/*.call_path 2>/dev/null
	$BDD_INSPECTOR --in $BDDS_DIR/$nf.bdd
	$BDD_VISUALIZER --in $BDDS_DIR/$nf.bdd --out $BDDS_DIR/$nf.dot 2>/dev/null
}

run "echo"
run "fw"
run "nat"
run "kvs"
run "cl"
run "psd"
