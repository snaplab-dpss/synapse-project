#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [[ -z "${SDE_INSTALL}" ]]; then
	echo "SDE_INSTALL env var not set."
	exit 1
fi

if [ $# -lt 1 ]; then
	echo "Usage: $0 <p4 program>"
	exit 1
fi

P4C="$SDE_INSTALL/bin/bf-p4c"
P4_DEBUG_FLAGS="-g --verbose 2 --create-graphs"
INSTALL_SCRIPT="$SCRIPT_DIR/p4_build.sh"

BUILD_DIR="$SCRIPT_DIR/build"
RESOURCES_FILE="$BUILD_DIR/pipe/logs/mau.resources.log"

PROGRAM=$1

run() {
	program=$1
	report="$program"
	report="${report%.*}.txt"

	echo "$program => $report"

	mkdir -p $BUILD_DIR
	$P4C $P4_DEBUG_FLAGS -o $BUILD_DIR $program
	mv $RESOURCES_FILE $report
	rm -rf $BUILD_DIR
}

for program in "$@"; do
	run "$program"
done
