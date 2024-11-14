#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

if [ -z ${SDE_INSTALL+x} ]; then
	echo "SDE_INSTALL env var not set. Exiting."
	exit 1
fi

CONTROLLER_EXE="$SCRIPT_DIR/build/netcache-controller"
TOPOLOGY_FILE="$SCRIPT_DIR/topology-model-t2na.json"
TOFINO_MODEL_EXE_NAME="tofino-model"
CONF_DIR="$SDE_INSTALL/share/p4/targets/tofino2"
CONF_FILE="$CONF_DIR/peregrine.conf"

# If the tofino model is not running in the background, launch it
if ! ps -e | grep -q "$TOFINO_MODEL_EXE_NAME"; then
	echo "Tofino model not running. Exiting."
	exit 1
fi

# Hugepages config
if [ "$(grep HugePages_Total /proc/meminfo | awk '{print $2}')" -lt "512" ]; then \
	sudo sysctl -w vm.nr_hugepages=512 > /dev/null
fi

# Compile
make debug -j

# Run controller with model
echo "Running sudo -E $CONTROLLER_EXE $TOPOLOGY_FILE --tofino-model"
NETCACHE_HW_CONF=$CONF_FILE \
	sudo -E $CONTROLLER_EXE \
	$TOPOLOGY_FILE \
	--tofino-model
	# --bf-prompt
