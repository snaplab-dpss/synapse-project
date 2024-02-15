#!/bin/bash

set -euo pipefail

if [[ -z "${SDE_INSTALL}" ]]; then
	echo "SDE_INSTALL env var not set."
	exit 1
fi

if [ $# -lt 1 ]; then
	echo "Usage: $0 <cpp source> [-d|--debug]"
	exit 1
fi

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
	echo "Usage: $0 <cpp source> [-d|--debug]"
	exit 1
fi

PERF_FLAGS="-O3 -DNDEBUG=1"

if [ $# -ge 2 ]; then
	if [ "$2" = "-d" ] || [ "$2" = "--debug" ]; then
		echo "Warning: building in debug mode"
		PERF_FLAGS="-O0 -g"
		ulimit -c unlimited
	fi
fi

PROGRAM=$1
BIN="${PROGRAM%.*}"

g++ \
	$PERF_FLAGS \
	-std=c++11 \
	-mcrc32 \
	-DSDE_INSTALL=$SDE_INSTALL \
	-DPROGRAM=\"$(basename $BIN)\" \
	-I$SDE_INSTALL/include \
	$PROGRAM \
	-o $BIN \
	-L$SDE_INSTALL/lib \
	-ldriver -lm -pthread -lpcap -ldl -levent
