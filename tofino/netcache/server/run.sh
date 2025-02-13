#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "$SCRIPT_DIR"

CORES="2"
MEM=1
SOCKET_MEM=128

CONF=$SCRIPT_DIR/conf.json

# Run the netcache server
"$SCRIPT_DIR"/build/release/server -m 8192 --no-huge --vdev "net_tap0,iface=test_rx" --vdev "net_tap1,iface=test_tx" --no-shconf -- $CONF
