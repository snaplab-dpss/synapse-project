#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)

function cleanup {
  sudo killall nf 2>/dev/null || true
  sudo killall tcpreplay 2>/dev/null || true
  sudo killall test_pcap_gen.py 2>/dev/null || true
}
trap cleanup EXIT

function test_nat {
  python3 test_pcap_gen.py --output nat.pcap

  sudo ./build/nf \
        --vdev "net_tap0,iface=test_lan" \
        --vdev "net_tap1,iface=test_wan" \
        --lcores 0 \
        --no-huge \
        --no-shconf -- \
        --internal-devs 0 \
        --fwd-rule 0,1 --fwd-rule 1,0 \
        --expire 1000000 \
        --max-flows 65536 \
        --extip 1.2.3.4 &
  NF_PID=$!

  while [ ! -f /sys/class/net/test_lan/tun_flags -o \
          ! -f /sys/class/net/test_lan/tun_flags ]; do
    echo "Waiting for NF to launch...";
    sleep 1;
  done

  sudo tcpreplay -M 10 -i "test_lan" --duration 10 -K -l 1 nat.pcap > /dev/null 2>/dev/null

  sudo killall nf
  wait $NF_PID 2>/dev/null || true
}

make clean
DEBUG=1 make

test_nat
