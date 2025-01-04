#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

if [ -z ${SDE_INSTALL+x} ]; then
	echo "SDE_INSTALL env var not set. Exiting."
	exit 1
fi

P4_COMPILER="$SCRIPT_DIR/p4_build.sh"
VETH_SETUP_SCRIPT="$SCRIPT_DIR/veth_setup.sh"
TOFINO_MODEL_SCRIPT="$SDE/run_tofino_model.sh"
SWITCHD_SCRIPT="$SDE/run_switchd.sh"
P4_TESTS_SCRIPT="$SDE/run_p4_tests.sh"

PORTS_FILE="$SCRIPT_DIR/ports.json"
LOG_DIR="log"

cleanup() {
	echo "[*] Cleaning up"
	sudo kill -9 $(ps -e | grep bf_switchd | awk '{ print $1 }') > /dev/null 2>&1 || true
	sudo kill -9 $(ps -e | grep run_switchd.sh | awk '{ print $1 }')  > /dev/null 2>&1 || true
	sudo killall tofino-model > /dev/null 2>&1 || true
	sudo killall run_p4_tests.sh > /dev/null 2>&1 || true
}

compile() {
	p4=$1

	echo "[*] Compiling"
	$P4_COMPILER $p4
}

setup() {
	if ! ifconfig | grep -q "veth"; then
		sudo $VETH_SETUP_SCRIPT
	fi

	rm -rf $LOG_DIR
	mkdir -p $LOG_DIR
}

fix_tty() {
	stty sane
}

run_tofino_model() {
	program=$1

	echo "[*] Running tofino model in the background"
	
	pushd $LOG_DIR
		$TOFINO_MODEL_SCRIPT \
			-p $program \
			--log-dir . \
			-f $PORTS_FILE \
			-c "$(find $SDE -name "$program.conf" | grep "/tofino/" | head -n 1)" \
			>/dev/null 2>&1 &
	popd

	echo "[*] Sleeping for 3 seconds"
	sleep 3

	if ! ps | grep -q "[r]un_tofino_mode"; then
		echo "Not running :("
		exit 1
	fi
}

run_switchd() {
	program=$1

	echo "[*] Running switch daemon in the background"
	
	pushd $LOG_DIR
		$SWITCHD_SCRIPT \
			-p $program \
			-c "$(find $SDE -name "$program.conf" | grep "/tofino/" | head -n 1)" \
			>/dev/null 2>&1 &
	popd

	echo "[*] Sleeping for 3 seconds"
	sleep 3

	if ! ps | grep -q "[r]un_switchd"; then
		echo "Not running :("
		exit 1
	fi
}

run_tests() {
	program=$1
	test=$(realpath $2)

	test_dir=$(dirname $test)
	test_file=$(basename -- "$test")
	test_file="${test_file%.*}"

	pushd $LOG_DIR
		$P4_TESTS_SCRIPT -p $program -t $test_dir -s $test_file -f $PORTS_FILE
	popd
}

if [ $# -lt 2 ]; then
	echo "Usage: $0 <p4 program> <test file> [--skip-setup]"
	exit 1
fi

P4_SRC=$1
TEST_FILE=$2
SKIP_SETUP=${3:-""}

P4_PROGRAM=$(basename -- "$P4_SRC")
P4_PROGRAM="${P4_PROGRAM%.*}"

if [ "$SKIP_SETUP" != "--skip-setup" ]; then
	trap cleanup EXIT
	cleanup
	compile $P4_SRC
	setup
	run_tofino_model $P4_PROGRAM
	fix_tty
	run_switchd $P4_PROGRAM
	fix_tty
fi

run_tests $P4_PROGRAM $TEST_FILE