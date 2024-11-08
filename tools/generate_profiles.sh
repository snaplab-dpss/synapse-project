#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"

SYNTHESIZED_DIR=$(realpath $SCRIPT_DIR/../synthesized)
TOOLS_DIR=$SCRIPT_DIR
PCAPS_DIR=$TOOLS_DIR/pcaps
BDDS_DIR=$(realpath $SCRIPT_DIR/../bdds)

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
    
    log_and_run bdd-synthesizer \
        --target profiler \
        --in $BDDS_DIR/$nf.bdd \
        --out $SYNTHESIZED_DIR/$nf-profiler.cpp

    log_and_run NF=$nf-profiler.cpp make -f $TOOLS_DIR/Makefile.dpdk
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

    warmup_pcap=$pcap-dev0-warmup
    pcap_dev_0=$pcap-dev0
    pcap_dev_1=$pcap-dev1
    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/fw-profiler \
        $SYNTHESIZED_DIR/$report.json \
        --warmup 0:$PCAPS_DIR/$warmup_pcap.pcap \
        0:$PCAPS_DIR/$pcap_dev_0.pcap \
        1:$PCAPS_DIR/$pcap_dev_1.pcap \
    
    log_and_run bdd-visualizer \
        --in $BDDS_DIR/fw.bdd \
        --report $SYNTHESIZED_DIR/$report.json \
        --out $SYNTHESIZED_DIR/$report.dot
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

    warmup_pcap=$pcap-dev0-warmup
    pcap_dev_0=$pcap-dev0
    pcap_dev_1=$pcap-dev1
    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/nat-profiler \
        $SYNTHESIZED_DIR/$report.json \
        --warmup 0:$PCAPS_DIR/$warmup_pcap.pcap \
        0:$PCAPS_DIR/$pcap_dev_0.pcap \
        1:$PCAPS_DIR/$pcap_dev_1.pcap \
    
    log_and_run bdd-visualizer \
        --in $BDDS_DIR/nat.bdd \
        --report $SYNTHESIZED_DIR/$report.json \
        --out $SYNTHESIZED_DIR/$report.dot
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
        $SYNTHESIZED_DIR/$report.json \
        --warmup 0:$PCAPS_DIR/$warmup_pcap.pcap \
        0:$PCAPS_DIR/$pcap_dev_0.pcap
    
    log_and_run bdd-visualizer \
        --in $BDDS_DIR/kvs.bdd \
        --report $SYNTHESIZED_DIR/$report.json \
        --out $SYNTHESIZED_DIR/$report.dot
}

# gen_and_build_profiler fw

# fw 10000 0 unif 0
# fw 10000 0 zipf 0.9
# fw 10000 0 zipf 0.99
# fw 10000 0 zipf 1.26

# fw 10000 1000000 unif 0
# fw 10000 1000000 zipf 0.9
# fw 10000 1000000 zipf 0.99
# fw 10000 1000000 zipf 1.26

# fw 10000 100000000 unif 0
# fw 10000 100000000 zipf 0.9
# fw 10000 100000000 zipf 0.99
# fw 10000 100000000 zipf 1.26

# gen_and_build_profiler nat

# nat 10000 0 unif 0
# nat 10000 0 zipf 0.9
# nat 10000 0 zipf 0.99
# nat 10000 0 zipf 1.26

# nat 10000 1000000 unif 0
# nat 10000 1000000 zipf 0.9
# nat 10000 1000000 zipf 0.99
# nat 10000 1000000 zipf 1.26

# nat 10000 100000000 unif 0
# nat 10000 100000000 zipf 0.9
# nat 10000 100000000 zipf 0.99
# nat 10000 100000000 zipf 1.26

gen_and_build_profiler kvs

kvs 100000 0 unif 0
kvs 100000 0 zipf 0.9
kvs 100000 0 zipf 0.99
kvs 100000 0 zipf 1.26

kvs 100000 1000000 unif 0
kvs 100000 1000000 zipf 0.9
kvs 100000 1000000 zipf 0.99
kvs 100000 1000000 zipf 1.26

kvs 100000 100000000 unif 0
kvs 100000 100000000 zipf 0.9
kvs 100000 100000000 zipf 0.99
kvs 100000 100000000 zipf 1.26

# gen_and_build_profiler pol