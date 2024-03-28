#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

MODE="Release"
# MODE="Debug"

CALL_PATHS_TO_BDD="$KLEE_DIR/$MODE/bin/call-paths-to-bdd"
BDD_TO_C="$KLEE_DIR/$MODE/bin/bdd-to-c"

NFS_DIR="$SCRIPT_DIR/../deps/maestro/dpdk-nfs"

run() {
	nf=$1
	
	pushd $NFS_DIR/$nf/ >/dev/null
		make symbex
	popd >/dev/null

	$CALL_PATHS_TO_BDD \
		-out $SCRIPT_DIR/$nf.bdd \
		-gv $SCRIPT_DIR/$nf.gv \
		$NFS_DIR/$nf/klee-last/*.call_path
	
	cp $SCRIPT_DIR/$nf.bdd $NFS_DIR/$nf/$nf.bdd
	cp $SCRIPT_DIR/$nf.gv $NFS_DIR/$nf/$nf.gv
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
