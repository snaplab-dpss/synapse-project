#!/bin/bash

set -euo pipefail

if [[ -z "${SDE_INSTALL}" ]]; then
	echo "SDE_INSTALL env var not set."
	exit 1
fi

if [ $# -lt 1 ]; then
	echo "Usage: $0 <p4>"
	exit 1
fi

P4C="$SDE_INSTALL/bin/bf-p4c"
P4_DEBUG_FLAGS="-g --verbose 2 --create-graphs"
BUILD_DIR="./build"

mkdir -p $BUILD_DIR
$P4C --target tofino2 --arch t2na $P4_DEBUG_FLAGS -o $BUILD_DIR $1