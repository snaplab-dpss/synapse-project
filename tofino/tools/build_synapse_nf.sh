#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

TOFINO_MAKEFILE="$SCRIPT_DIR/Makefile"
LIBSYCON="$SCRIPT_DIR/../../libsycon"

if [[ -z "${SDE_INSTALL}" ]]; then
	echo "SDE_INSTALL env var not set."
	exit 1
fi

nf=$1

if [ -z "$nf" ]; then
	echo "Usage: $0 <nf>"
	exit 1
fi

app="${nf%.*}"
p4_program=$app.p4
cpp_program=$app.cpp

if [ ! -f "$p4_program" ]; then
	echo "File not found: $p4_program"
	exit 1
fi

if [ ! -f "$cpp_program" ]; then
	echo "File not found: $cpp_program"
	exit 1
fi

APP=$app make -f $TOFINO_MAKEFILE install-tofino2
APP=$app make -f $TOFINO_MAKEFILE controller -j

