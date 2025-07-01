#!/bin/bash

set -euo pipefail

if [ -z ${SDE+x} ]; then
	echo "SDE env var not set. Exiting."
	exit 1
fi

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
CURRENT_DIR=$(pwd)

LOG_DIR="$CURRENT_DIR/logs"

LOG_TOFINO_MODEL="tofino_model.log"
LOG_SYNAPSE_NF="synapse_nf.log"

TOFINO_MODEL_SCRIPT="$SDE/run_tofino_model.sh"
BUILD_SYNAPSE_NF_SCRIPT="$SCRIPT_DIR/build_synapse_nf.sh"
VETH_SETUP_SCRIPT="$SCRIPT_DIR/veth_setup.sh"
PORTS_FILE="$SCRIPT_DIR/ports_tof2.json"

# Expects 2 arguments:
# 1. Path to the Synapse NF name (e.g., "synapse_nf_nat"), so that we can find the following files:
#    - synapse_nf_nat.p4
#    - synapse_nf_nat.cpp
# 2. Path to the testing script (usually a python script) that will run the tests.

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <synapse_nf_name> <test_script>"
    exit 1
fi

SYNAPSE_NF_PATH=$1
test_script=$2

SYNAPSE_NF_NAME=$(basename "$SYNAPSE_NF_PATH")
SYNAPSE_NF_P4="$SYNAPSE_NF_NAME.p4"
SYNAPSE_NF_CPP="$SYNAPSE_NF_NAME.cpp"

BFRT_PYTHON_PATH="$SDE_INSTALL/lib/python3.10/site-packages/tofino/"
BFRT_PYTHON_PATH="$BFRT_PYTHON_PATH:$SDE_INSTALL/lib/python3.10/site-packages/p4testutils/"
BFRT_PYTHON_PATH="$BFRT_PYTHON_PATH:$SDE_INSTALL/lib/python3.10/site-packages/"

if [ ! -f "$SYNAPSE_NF_P4" ]; then
    echo "Error: P4 file not found at $SYNAPSE_NF_P4"
    exit 1
fi

if [ ! -f "$SYNAPSE_NF_CPP" ]; then
    echo "Error: C++ file not found at $SYNAPSE_NF_CPP"
    exit 1
fi

if [ ! -f "$test_script" ]; then
    echo "Error: Test script not found at $test_script"
    exit 1
fi

fix_tty() {
	stty sane
}

cleanup() {
	sudo killall tofino-model > /dev/null 2>&1 || true
	sudo kill -9 $(ps -e | grep bf_switchd | awk '{ print $1 }') > /dev/null 2>&1 || true
	sudo kill -9 $(ps -e | grep run_switchd.sh | awk '{ print $1 }')  > /dev/null 2>&1 || true
    sudo killall $BUILD_SYNAPSE_NF_SCRIPT > /dev/null 2>&1 || true
	sudo killall $SYNAPSE_NF_NAME > /dev/null 2>&1 || true
    fix_tty
}

build_logs() {
    mkdir -p $LOG_DIR
}

build_nf() {
    $BUILD_SYNAPSE_NF_SCRIPT "$SYNAPSE_NF_NAME"
}

run_tofino_model() {
    echo
	echo "[*] Running tofino model in the background for $SYNAPSE_NF_NAME"
	
	pushd $LOG_DIR
        touch $LOG_TOFINO_MODEL

		sudo -E $TOFINO_MODEL_SCRIPT \
			-p $SYNAPSE_NF_NAME \
			--arch tf2 \
			--log-dir . \
			-f $PORTS_FILE \
			>$LOG_TOFINO_MODEL 2>&1 &
	popd

    sleep 3

	if ! ps -e | grep -q "tofino-model"; then
		echo "Tofino model process is not running. Exiting."
		exit 1
	fi
}

run_nf() {
    echo
    echo "[*] Running Synapse NF: $SYNAPSE_NF_NAME"

    pushd $LOG_DIR
        touch $LOG_SYNAPSE_NF
        sudo $VETH_SETUP_SCRIPT
        sudo -E \
            $CURRENT_DIR/build/debug/$SYNAPSE_NF_NAME \
            --ports {1..32} \
            --model \
            >$LOG_SYNAPSE_NF 2>&1 &

        if ! timeout 10s bash -c "tail -f \"$LOG_SYNAPSE_NF\" | grep -q -m 1 \"Controller is running\""; then
            echo "Error: Synapse NF did not start correctly. Exiting."
            exit 1
        fi
    popd
    
}

run_tests() {
    echo
    echo "[*] Running tests for $SYNAPSE_NF_NAME using $test_script"
    sudo PYTHONPATH="$BFRT_PYTHON_PATH" $test_script
}

trap cleanup EXIT
cleanup

build_logs
build_nf
run_tofino_model
fix_tty
run_nf
fix_tty

run_tests
