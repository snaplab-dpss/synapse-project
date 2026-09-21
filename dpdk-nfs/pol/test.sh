#!/bin/bash

# The policer marks out-of-profile packets instead of dropping them (CS1 / Lower Effort, so the
# IPv4 TOS byte reads 0x20 once the two ECN bits are masked off), which is what lets a throughput
# measurement see the forwarding capacity rather than the policed rate. Two runs of the same
# traffic prove both halves of that:
#   generous  rate and burst far above the offered load  -> nothing is marked
#   strict    rate and burst far below the offered load  -> most packets are marked, and just as
#             many arrive as in the generous run, i.e. marking replaced dropping
# WAN (device 0) is policed; LAN (device 1) is internal and never is.

set -euo pipefail

SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)

# POLICED_DSCP is 8 in policer_main.c; the TOS byte carries it shifted left by 2.
readonly MARKED_TOS=0x20
readonly DURATION=10
readonly OFFERED=1M     # iperf -b: comfortably above the strict rate, below the generous one
readonly PKT_LEN=1000
readonly PROBE_PORT=9999

# Run this as yourself: it sudoes the commands that need it. Under sudo the build fails instead,
# because PKG_CONFIG_PATH does not survive and the shared Makefile then cannot find libdpdk.
if [ "$(id -u)" -eq 0 ]; then
  echo "Run $(basename "$0") without sudo; it elevates the commands that need it." >&2
  exit 1
fi

function cleanup {
  sudo killall tcpdump 2>/dev/null || true
  sudo killall nf 2>/dev/null || true
  sudo killall iperf 2>/dev/null || true
  sudo ip netns delete lan 2>/dev/null || true
  sudo ip netns delete wan 2>/dev/null || true
}
trap cleanup EXIT

# Sends OFFERED for DURATION through a policer with the given rate/burst and reports, on stdout,
# "<packets that arrived> <of those, marked>".
function run_case {
  local rate=$1
  local burst=$2
  local pcap
  # tcpdump must CREATE this file. It is AppArmor-confined and drops to the `tcpdump` user, and the
  # profile's rules are `owner` rules: it may write only files it owns, so handing it a path that
  # already exists (mktemp) fails with "Permission denied" and leaves an empty pcap.
  pcap=/tmp/pol-test-$$-${RANDOM}.pcap
  sudo rm -f "$pcap"

  # Device 0 is net_tap0 (WAN, policed), device 1 is net_tap1 (LAN, internal). Traffic is
  # forwarded between the two.
  local nf_log
  nf_log=$(mktemp /tmp/pol-test-nf-XXXX.log)
  sudo "$SCRIPT_DIR/build/nf" \
        --vdev "net_tap0,iface=test_wan" \
        --vdev "net_tap1,iface=test_lan" \
        --no-huge \
        --no-shconf -- \
        --internal-devs 1 \
        --fwd-rule 0,1 \
        --fwd-rule 1,0 \
        --rate "$rate" \
        --burst "$burst" \
        --capacity 65536 \
        >"$nf_log" 2>&1 &
  local nf_pid=$!

  # Never wait forever: if the NF died, say so and show why.
  local waited=0
  while [ ! -f /sys/class/net/test_lan/tun_flags -o \
          ! -f /sys/class/net/test_wan/tun_flags ]; do
    if ! sudo kill -0 "$nf_pid" 2>/dev/null; then
      echo "the NF exited before its interfaces appeared:" >&2
      tail -n 15 "$nf_log" >&2
      exit 1
    fi
    if [ "$waited" -ge 30 ]; then
      echo "the NF is running but test_lan/test_wan never appeared after ${waited}s:" >&2
      tail -n 15 "$nf_log" >&2
      exit 1
    fi
    echo "  waiting for the NF to launch (${waited}s)..." >&2
    sleep 1
    waited=$((waited + 1))
  done
  sleep 2

  # Start from a known state: a previous run may have left these behind, and then `netns add`
  # fails, the links are never moved, and the case silently measures the old setup.
  sudo ip netns delete lan 2>/dev/null || true
  sudo ip netns delete wan 2>/dev/null || true

  sudo ip netns add lan
  sudo ip link set test_lan netns lan
  sudo ip netns exec lan ifconfig test_lan up 10.0.0.1
  local lan_mac
  lan_mac=$(sudo ip netns exec lan ifconfig test_lan | head -n 4 | tail -n 1 | awk '{ print $2 }')

  sudo ip netns add wan
  sudo ip link set test_wan netns wan
  sudo ip netns exec wan ifconfig test_wan up 10.0.0.2
  local wan_mac
  wan_mac=$(sudo ip netns exec wan ifconfig test_wan | head -n 4 | tail -n 1 | awk '{ print $2 }')

  sudo ip netns exec lan arp -i test_lan -s 10.0.0.2 "$wan_mac"
  sudo ip netns exec wan arp -i test_wan -s 10.0.0.1 "$lan_mac"

  # The tap PMD does not carry traffic the moment the netdevs are up inside their namespaces, and
  # the dead window is long enough to swallow a whole iperf run. Probe until a packet actually
  # crosses instead of sleeping a guess.
  local ready=0 attempt
  for attempt in $(seq 1 30); do
    sudo ip netns exec lan timeout 2 tcpdump -i test_lan -nn -c 1 "udp and port $PROBE_PORT" >/dev/null 2>&1 &
    local probe_pid=$!
    sleep 0.3
    sudo ip netns exec wan python3 -c \
      "import socket; socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(b'probe', ('10.0.0.1', $PROBE_PORT))" >/dev/null 2>&1 || true
    if wait "$probe_pid" 2>/dev/null; then
      ready=1
      break
    fi
  done
  if [ "$ready" -ne 1 ]; then
    echo "no packet crossed the NF after 30 probes; it is running but not forwarding:" >&2
    tail -n 15 "$nf_log" >&2
    exit 1
  fi

  # Capture what actually comes out towards the LAN: that is what was neither dropped nor lost.
  # `timeout` ends tcpdump itself, so it flushes and exits cleanly; killing the backgrounded job
  # would only kill the `sudo` wrapper and leave tcpdump buffering into a pcap we then read empty.
  # -U writes each packet straight through, so the file is complete however the capture ends.
  sudo ip netns exec lan timeout $((DURATION + 6)) \
      tcpdump -i test_lan -nn -U -w "$pcap" "udp and port 5001" >/dev/null 2>&1 &
  local tcpdump_pid=$!
  sleep 2

  sudo ip netns exec lan iperf -us -i 1 >/dev/null 2>&1 &
  local server_pid=$!

  sudo ip netns exec wan iperf -uc 10.0.0.1 -b "$OFFERED" -l "$PKT_LEN" -t "$DURATION" >/dev/null 2>&1

  # Let the capture reach its own timeout rather than killing it.
  wait "$tcpdump_pid" 2>/dev/null || true

  sudo killall iperf 2>/dev/null || true
  wait "$server_pid" 2>/dev/null || true
  sudo killall nf 2>/dev/null || true
  wait "$nf_pid" 2>/dev/null || true

  sudo ip netns delete lan 2>/dev/null || true
  sudo ip netns delete wan 2>/dev/null || true
  rm -f "$nf_log"

  local total marked
  total=$(tcpdump -r "$pcap" -nn "udp and port 5001" 2>/dev/null | wc -l)
  marked=$(tcpdump -r "$pcap" -nn "udp and port 5001 and (ip[1] & 0xfc) == $MARKED_TOS" 2>/dev/null | wc -l)
  sudo rm -f "$pcap"

  echo "$total $marked"
}

function fail {
  echo "FAIL: $1" >&2
  exit 1
}

cd "$SCRIPT_DIR"

# Checked before building, because a wrong DPDK here would have `make clean` throw away a working
# binary and replace it with one the test cannot use.
if ! pkg-config --exists libdpdk; then
  echo "pkg-config finds no libdpdk." >&2
  exit 1
fi
if ! pkg-config --static --libs libdpdk | grep -q rte_net_tap; then
  echo "libdpdk $(pkg-config --modversion libdpdk) has no tap PMD, which this test runs on:" >&2
  echo "the NF would die with 'failed to parse device net_tap0'. Source paths.sh first." >&2
  exit 1
fi

make clean
make ADDITIONAL_FLAGS="-g"

echo "== generous policer (nothing should be marked)"
generous=$(run_case 10000000 10000000)
read -r generous_total generous_marked <<<"$generous"
echo "   arrived $generous_total, marked $generous_marked"

echo "== strict policer (most should be marked, none dropped)"
strict=$(run_case 12500 25000)
read -r strict_total strict_marked <<<"$strict"
echo "   arrived $strict_total, marked $strict_marked"

[ "$generous_total" -gt 100 ] || fail "only $generous_total packets arrived unpoliced; the setup did not carry traffic"
[ "$generous_marked" -eq 0 ] || fail "$generous_marked packets were marked by a policer that should not police any"
[ "$strict_marked" -gt $((strict_total / 2)) ] || fail "only $strict_marked of $strict_total packets were marked; the policer is not policing"
# The point of marking: a policed run delivers as many packets as an unpoliced one.
[ "$strict_total" -ge $((generous_total * 9 / 10)) ] ||
  fail "only $strict_total packets survived policing against $generous_total unpoliced; packets are being dropped, not marked"

echo "Done."
