#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
CONTROLLER_BUILDER="$SCRIPT_DIR/../../build_tools/build-controller.sh"

cd $SCRIPT_DIR

if [ -z ${SDE_INSTALL+x} ]; then
	echo "SDE_INSTALL env var not set. Exiting."
	exit 1
fi

if [ $# -eq 0 ]; then
	echo "Usage: run-with-hw.sh timeout (in ms)"
	exit 1
fi

TIMEOUT=$1

CONTROLLER_SRC="$SCRIPT_DIR/timeouts.cpp"
CONTROLLER_EXE="$SCRIPT_DIR/timeouts"

LD_LIBRARY_PATH="/usr/local/lib/:$SDE_INSTALL/lib/"

# Compile
$CONTROLLER_BUILDER $CONTROLLER_SRC

# Run controller with model
LD_LIBRARY_PATH=$LD_LIBRARY_PATH $CONTROLLER_EXE $TIMEOUT --hw