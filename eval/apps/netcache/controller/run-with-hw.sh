#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "$SCRIPT_DIR"

if [ -z ${SDE_INSTALL+x} ]; then
	echo "SDE_INSTALL env var not set. Exiting."
	exit 1
fi

CONTROLLER_EXE="$SCRIPT_DIR/build/Debug/netcache-controller"
CONTROLLER_CONF_FILE="$SCRIPT_DIR/conf/conf-hw.json"
HW_CONF_FILE="$SCRIPT_DIR/conf/AS9516-32D.conf"

LD_LIBRARY_PATH="/usr/local/lib/:$SDE_INSTALL/lib/"

# Compile
# make release -j

# Run controller using the switch
LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
	NETCACHE_HW_CONF=$HW_CONF_FILE \
	$CONTROLLER_EXE \
	-m 8192 --no-huge --vdev "net_tap0,iface=test_rx" --vdev "net_tap1,iface==test_tx" --no-shconf \
	-- \
	"$CONTROLLER_CONF_FILE"
