#!/bin/bash

########################################################################################################################################
#   Note: This is all based on our specific testbed configuration.
#   You may want to adjust the port configuration on each NF.
#   For example, our NFs will run on a Tofino programmable switch with 32 ports (1 to 32). However, we only have ports 3-32 connected.
#   That is why we generate pcaps only for devices 2-31 (devices start by 0).
########################################################################################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
PROJECT_DIR=$(realpath $SCRIPT_DIR/..)

TOOLS_DIR=$PROJECT_DIR/tools
SYNTHESIZED_DIR=$PROJECT_DIR/synthesized
PCAPS_DIR=$PROJECT_DIR/pcaps
BDDS_DIR=$PROJECT_DIR/bdds
SYNAPSE_DIR=$PROJECT_DIR/synapse

# Everything will be done inside the synthesized directory
cd $SYNTHESIZED_DIR

log_and_run() {
    echo
    echo "[*] Running cmd: $@"
    echo
    eval $@
}

gen_and_build_profiler() {
    nf=$1
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-synthesizer \
        --target profiler \
        --in $BDDS_DIR/$nf.bdd \
        --out $SYNTHESIZED_DIR/$nf-profiler.cpp

    log_and_run NF=$nf-profiler.cpp make -f $TOOLS_DIR/Makefile.dpdk
}

nop() {
    flows=$1
    churn=$2
    distribution=$3
    zipf_param=$4

    pcap="nop-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        pcap="$pcap$zipf_param"
    fi

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/nop-profiler \
        $PCAPS_DIR/$report.json \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/nop.bdd \
        --profile $PCAPS_DIR/$report.json \
        --out $PCAPS_DIR/$report.dot
}

fw() {
    flows=$1
    churn=$2
    distribution=$3
    zipf_param=$4

    pcap="fw-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        pcap="$pcap$zipf_param"
    fi

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/fw-profiler \
        $PCAPS_DIR/$report.json \
        --warmup 2:$PCAPS_DIR/$pcap-dev2-warmup.pcap \
        --warmup 4:$PCAPS_DIR/$pcap-dev4-warmup.pcap \
        --warmup 6:$PCAPS_DIR/$pcap-dev6-warmup.pcap \
        --warmup 8:$PCAPS_DIR/$pcap-dev8-warmup.pcap \
        --warmup 10:$PCAPS_DIR/$pcap-dev10-warmup.pcap \
        --warmup 12:$PCAPS_DIR/$pcap-dev12-warmup.pcap \
        --warmup 14:$PCAPS_DIR/$pcap-dev14-warmup.pcap \
        --warmup 16:$PCAPS_DIR/$pcap-dev16-warmup.pcap \
        --warmup 18:$PCAPS_DIR/$pcap-dev18-warmup.pcap \
        --warmup 20:$PCAPS_DIR/$pcap-dev20-warmup.pcap \
        --warmup 22:$PCAPS_DIR/$pcap-dev22-warmup.pcap \
        --warmup 24:$PCAPS_DIR/$pcap-dev24-warmup.pcap \
        --warmup 26:$PCAPS_DIR/$pcap-dev26-warmup.pcap \
        --warmup 28:$PCAPS_DIR/$pcap-dev28-warmup.pcap \
        --warmup 30:$PCAPS_DIR/$pcap-dev30-warmup.pcap \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/fw.bdd \
        --profile $PCAPS_DIR/$report.json \
        --out $PCAPS_DIR/$report.dot
}

nat() {
    flows=$1
    churn=$2
    distribution=$3
    zipf_param=$4

    pcap="nat-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        pcap="$pcap$zipf_param"
    fi

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/nat-profiler \
        $PCAPS_DIR/$report.json \
        --warmup 2:$PCAPS_DIR/$pcap-dev2-warmup.pcap \
        --warmup 4:$PCAPS_DIR/$pcap-dev4-warmup.pcap \
        --warmup 6:$PCAPS_DIR/$pcap-dev6-warmup.pcap \
        --warmup 8:$PCAPS_DIR/$pcap-dev8-warmup.pcap \
        --warmup 10:$PCAPS_DIR/$pcap-dev10-warmup.pcap \
        --warmup 12:$PCAPS_DIR/$pcap-dev12-warmup.pcap \
        --warmup 14:$PCAPS_DIR/$pcap-dev14-warmup.pcap \
        --warmup 16:$PCAPS_DIR/$pcap-dev16-warmup.pcap \
        --warmup 18:$PCAPS_DIR/$pcap-dev18-warmup.pcap \
        --warmup 20:$PCAPS_DIR/$pcap-dev20-warmup.pcap \
        --warmup 22:$PCAPS_DIR/$pcap-dev22-warmup.pcap \
        --warmup 24:$PCAPS_DIR/$pcap-dev24-warmup.pcap \
        --warmup 26:$PCAPS_DIR/$pcap-dev26-warmup.pcap \
        --warmup 28:$PCAPS_DIR/$pcap-dev28-warmup.pcap \
        --warmup 30:$PCAPS_DIR/$pcap-dev30-warmup.pcap \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/nat.bdd \
        --profile $PCAPS_DIR/$report.json \
        --out $PCAPS_DIR/$report.dot
}

kvs() {
    keys=$1
    churn=$2
    distribution=$3
    zipf_param=$4

    pcap="kvs-k$keys-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        pcap="$pcap$zipf_param"
    fi

    warmup_pcap=$pcap-warmup
    pcap_dev_0=$pcap
    report=$pcap
    
    log_and_run $SYNTHESIZED_DIR/build/kvs-profiler \
        $PCAPS_DIR/$report.json \
        --warmup 0:$PCAPS_DIR/$warmup_pcap.pcap \
        0:$PCAPS_DIR/$pcap_dev_0.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/kvs.bdd \
        --profile $PCAPS_DIR/$report.json \
        --out $PCAPS_DIR/$report.dot
}

gen_and_build_profiler nop

nop 10000 0 unif 0
nop 10000 0 zipf 0.9
nop 10000 0 zipf 0.99
nop 10000 0 zipf 1.26

nop 10000 1000000 unif 0
nop 10000 1000000 zipf 0.9
nop 10000 1000000 zipf 0.99
nop 10000 1000000 zipf 1.26

gen_and_build_profiler fw

fw 10000 0 unif 0
fw 10000 0 zipf 0.9
fw 10000 0 zipf 0.99
fw 10000 0 zipf 1.26

fw 10000 1000000 unif 0
fw 10000 1000000 zipf 0.9
fw 10000 1000000 zipf 0.99
fw 10000 1000000 zipf 1.26

# gen_and_build_profiler nat

# nat 10000 0 unif 0
# nat 10000 0 zipf 0.9
# nat 10000 0 zipf 0.99
# nat 10000 0 zipf 1.26

# nat 10000 1000000 unif 0
# nat 10000 1000000 zipf 0.9
# nat 10000 1000000 zipf 0.99
# nat 10000 1000000 zipf 1.26

# gen_and_build_profiler kvs

# kvs 100000 0 unif 0
# kvs 100000 0 zipf 0.9
# kvs 100000 0 zipf 0.99
# kvs 100000 0 zipf 1.26

# kvs 100000 1000000 unif 0
# kvs 100000 1000000 zipf 0.9
# kvs 100000 1000000 zipf 0.99
# kvs 100000 1000000 zipf 1.26

# gen_and_build_profiler pol