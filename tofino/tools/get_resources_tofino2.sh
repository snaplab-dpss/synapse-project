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

run() {
	program=$1

	report=$(basename $program)
	report="${report%.*}-resources.txt"

	TEMP_DIR=$(mktemp -d)
	pushd $TEMP_DIR
	
		echo "$program => $report"
		mkdir -p build
		$P4C --target tofino2 --arch t2na $P4_DEBUG_FLAGS -o build $program
	popd

	mv $TEMP_DIR/build/pipe/logs/mau.resources.log $report
	rm -rf $TEMP_DIR
}

for program in "$@"; do
	run $(realpath $program)
done
