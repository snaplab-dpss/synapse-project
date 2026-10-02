#!/bin/bash

# The hierarchical heavy hitter keeps, per counting interval, the packets of every /8 and /16 and
# a HashPipe of the heaviest /24s, and the debug build reports the three heaviest prefixes of each
# level at the end of every interval as "TOP /<len> #<rank>: <prefix> count <n>". Three WAN sources
# send through it:
#   heavy   10.1.2.3  far above the others: 10.0.0.0/8, 10.1.0.0/16 and 10.1.2.0/24 must head
#                     their levels
#   kin     10.1.9.9  light, but it shares the heavy source's /8 and /16: its /24 must rank
#                     below the heavy one -- the hierarchy
#   other   20.0.0.5  light, in prefixes of its own: second at /8 and /16, never first at /24
# Forwarding is untouched: every packet sent arrives.
# WAN (device 0) is monitored; LAN (device 1) is internal and never is.

set -euo pipefail

SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)

readonly DURATION=10
readonly INTERVAL_US=1000000
readonly HEAVY_IP=10.1.2.3
readonly KIN_IP=10.1.9.9
readonly OTHER_IP=20.0.0.5
readonly HEAVY_RATE=1M  # iperf -b: ~125 packets/s
readonly LIGHT_RATE=50k # ~6 packets/s
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

function fail {
  echo "FAIL: $1" >&2
  exit 1
}

function probe {
  sudo ip netns exec wan python3 -c \
    "import socket; socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(b'probe', ('10.0.0.1', $PROBE_PORT))" >/dev/null 2>&1 || true
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

# The debug build reports the tables.
make clean
make DEBUG=1

# Every background process below runs under setsid, detached from the terminal: sudo (with its
# use_pty default) puts the terminal of a backgrounded command into raw mode for as long as the
# command runs, and a line printed meanwhile comes out staggered.

# tcpdump must CREATE this file: it is AppArmor-confined and may only write files it owns.
pcap=/tmp/hhh-test-$$-${RANDOM}.pcap
sudo rm -f "$pcap"

nf_log=$(mktemp /tmp/hhh-test-nf-XXXX.log)
setsid sudo "$SCRIPT_DIR/build/nf" \
      --vdev "net_tap0,iface=test_wan" \
      --vdev "net_tap1,iface=test_lan" \
      --no-huge \
      --no-shconf -- \
      --internal-devs 1 \
      --fwd-rule 0,1 \
      --fwd-rule 1,0 \
      --stages 6 \
      --width 1024 \
      --interval "$INTERVAL_US" \
      </dev/null >"$nf_log" 2>&1 &
nf_pid=$!

# Never wait forever: if the NF died, say so and show why.
waited=0
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
# fails, the links are never moved, and the test silently measures the old setup.
sudo ip netns delete lan 2>/dev/null || true
sudo ip netns delete wan 2>/dev/null || true

sudo ip netns add lan
sudo ip link set test_lan netns lan
sudo ip netns exec lan ifconfig test_lan up 10.0.0.1
lan_mac=$(sudo ip netns exec lan ifconfig test_lan | head -n 4 | tail -n 1 | awk '{ print $2 }')

sudo ip netns add wan
sudo ip link set test_wan netns wan
sudo ip netns exec wan ifconfig test_wan up 10.0.0.2
wan_mac=$(sudo ip netns exec wan ifconfig test_wan | head -n 4 | tail -n 1 | awk '{ print $2 }')

# The three sources are addresses of the WAN side; the LAN side has a route and an ARP entry
# back to each of them, since the NF forwards without touching either.
for src in "$HEAVY_IP" "$KIN_IP" "$OTHER_IP"; do
  sudo ip netns exec wan ip addr add "$src/8" dev test_wan
  sudo ip netns exec lan ip route add "${src%.*.*.*}.0.0.0/8" dev test_lan 2>/dev/null || true
  sudo ip netns exec lan arp -i test_lan -s "$src" "$wan_mac"
done

sudo ip netns exec lan arp -i test_lan -s 10.0.0.2 "$wan_mac"
sudo ip netns exec wan arp -i test_wan -s 10.0.0.1 "$lan_mac"

# The tap PMD does not carry traffic the moment the netdevs are up inside their namespaces, and
# the dead window is long enough to swallow a whole iperf run. Probe until a packet actually
# crosses instead of sleeping a guess.
ready=0
for attempt in $(seq 1 30); do
  setsid sudo ip netns exec lan timeout 2 tcpdump -i test_lan -nn -c 1 "udp and port $PROBE_PORT" </dev/null >/dev/null 2>&1 &
  probe_pid=$!
  sleep 0.3
  probe
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

# Capture what comes out towards the LAN: that is what was neither dropped nor lost. `timeout`
# ends tcpdump itself, so it flushes and exits cleanly; -U writes each packet straight through.
setsid sudo ip netns exec lan timeout $((DURATION + 6)) \
    tcpdump -i test_lan -nn -U -w "$pcap" "udp and port 5001" </dev/null >/dev/null 2>&1 &
tcpdump_pid=$!
sleep 2

setsid sudo ip netns exec lan iperf -us -i 1 </dev/null >/dev/null 2>&1 &
server_pid=$!

client_pids=""
for src in "$HEAVY_IP:$HEAVY_RATE" "$KIN_IP:$LIGHT_RATE" "$OTHER_IP:$LIGHT_RATE"; do
  setsid sudo ip netns exec wan iperf -uc 10.0.0.1 -B "${src%:*}" -b "${src#*:}" -l "$PKT_LEN" -t "$DURATION" </dev/null >/dev/null 2>&1 &
  client_pids="$client_pids $!"
done
# Only the clients: the server and the capture are waited for below.
wait $client_pids

# A report is written by the first packet of the next interval: send one after the interval so
# the last one gets reported, then let the capture reach its own timeout.
sleep "$((INTERVAL_US / 1000000 + 1))"
probe
wait "$tcpdump_pid" 2>/dev/null || true

sudo killall iperf 2>/dev/null || true
wait "$server_pid" 2>/dev/null || true
sudo killall nf 2>/dev/null || true
wait "$nf_pid" 2>/dev/null || true

sudo ip netns delete lan 2>/dev/null || true
sudo ip netns delete wan 2>/dev/null || true

# The busiest reported interval of a level (the one with the largest top count: the first and the
# last intervals only see the edges of the run), as "<rank> <prefix> <count>" lines.
function busiest_report {
  grep "TOP $1 " "$nf_log" | sed "s|.*TOP $1 #\([0-9]\): \([0-9.]*\) count \([0-9]*\)|\1 \2 \3|" |
    awk '$1 == 1 { keep = $3 > best; if (keep) { best = $3; lines = "" } } keep { lines = lines $0 "\n" } END { printf "%s", lines }'
}

echo "== heaviest prefixes in the busiest reported interval:"
for level in /8 /16 /24; do
  busiest_report $level | awk -v level=$level '{ printf "   %-4s #%s %-16s %s packets\n", level, $1, $2, $3 }'
done

echo "== arrived at the LAN:"
total=$(tcpdump -r "$pcap" -nn "udp and port 5001" 2>/dev/null | wc -l)
for src in "$HEAVY_IP" "$KIN_IP" "$OTHER_IP"; do
  n=$(tcpdump -r "$pcap" -nn "udp and port 5001 and src host $src" 2>/dev/null | wc -l)
  printf "   %-10s %6s packets\n" "$src" "$n"
done

first8=$(busiest_report /8 | awk '$1 == 1 { print $2 }')
second8=$(busiest_report /8 | awk '$1 == 2 { print $2 }')
first16=$(busiest_report /16 | awk '$1 == 1 { print $2 }')
first24=$(busiest_report /24 | awk '$1 == 1 { print $2 }')
rank_kin24=$(busiest_report /24 | awk '$2 == "10.1.9.0" { print $1 }')

sudo rm -f "$pcap"
rm -f "$nf_log"

[ "$total" -gt 100 ] || fail "only $total packets arrived; the setup did not carry traffic"
[ "$first8" = "10.0.0.0" ] || fail "the heaviest /8 is '$first8', not 10.0.0.0"
[ "$second8" = "20.0.0.0" ] || fail "the second /8 is '$second8', not 20.0.0.0"
[ "$first16" = "10.1.0.0" ] || fail "the heaviest /16 is '$first16', not 10.1.0.0"
[ "$first24" = "10.1.2.0" ] || fail "the heaviest /24 is '$first24', not 10.1.2.0"
[ -z "$rank_kin24" ] || [ "$rank_kin24" -gt 1 ] || fail "the kin's /24 outranks the heavy source's"

echo "Done."
