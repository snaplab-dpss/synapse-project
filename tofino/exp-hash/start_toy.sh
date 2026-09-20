#!/bin/bash
# Start the tofino-model and bf_switchd for a toy program (no controller).
set -u
P4=$1
mkdir -p /tmp/exp-hash && cd /tmp/exp-hash
setsid nohup $SDE/run_tofino_model.sh -p $P4 --arch tf2 -f /home/user/workspace/tofino/tools/ports_tof2.json --log-dir /tmp/exp-hash > model.out 2>&1 < /dev/null &
sleep 8
setsid nohup $SDE/run_switchd.sh -p $P4 --arch tf2 > switchd.out 2>&1 < /dev/null &
sleep 25
tail -2 model.out
grep -i "initialized\|error\|bf_switchd: dev_id" switchd.out | tail -3
