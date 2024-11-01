#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)

function cleanup {
  sudo killall nf 2>/dev/null || true
  sudo killall tcpreplay 2>/dev/null || true
  sudo killall cl.py 2>/dev/null || true
}
trap cleanup EXIT

function test_cl {
  MAX_CLIENTS=$1

  python3 cl.py --output cl.pcap

  sudo ./build/nf \
        --vdev "net_tap0,iface=test_wan" \
        --vdev "net_tap1,iface=test_lan" \
        --lcores 0 \
        --no-huge \
        --no-shconf -- \
        --lan 1 \
        --wan 0 \
        --max-flows 65536 \
        --max-clients $MAX_CLIENTS \
        --expire 1000000 \
        --sketch-height 4 \
        --sketch-width 1024 \
        --sketch-cleanup-interval 2000000 &
  NF_PID=$!

  while [ ! -f /sys/class/net/test_lan/tun_flags -o \
          ! -f /sys/class/net/test_lan/tun_flags ]; do
    echo "Waiting for NF to launch...";
    sleep 1;
  done

  sudo tcpreplay -M 10 -i "test_wan" --duration 10 -K -l 10000 cl.pcap > /dev/null 2>/dev/null

  sudo killall nf
  wait $NF_PID 2>/dev/null || true
}

make clean
make DEBUG=1

max_clients=64
test_cl $max_clients
