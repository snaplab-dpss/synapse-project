#!/bin/bash

set -euo pipefail

P4I=~/p4i.linux.mod/p4i

if [[ -z "${SDE_INSTALL}" ]]; then
	echo "SDE_INSTALL env var not set."
	exit 1
fi

cleanup() {
    sudo killall p4itofino
}

trap cleanup EXIT

if [ -f $SDE/bf-sde-9.13.1.manifest ] || [ -f $SDE/bf-sde-9.13.2.manifest  ]; then
    # No license needed, let's use the provided p4i
    $SDE/install/bin/p4i --no-browser -w . --loglevel info
else
    $P4I --no-browser -w .
fi
