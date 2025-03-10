#!/bin/bash

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

if [ -z ${SDE_INSTALL+x} ]; then
	echo "SDE_INSTALL env var not set. Exiting."
	exit 1
fi

if [ -z ${1+x} ]; then
    echo "Usage: $0 <p4_program>"
    exit 1
fi

./run_tofino_model.sh -p $1 --arch tf2 -f $SCRIPT_DIR/ports_tof2.json