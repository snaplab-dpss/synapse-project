#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SYNTHESIZED_DIR=$SCRIPT_DIR/../synthesized
TOOLS_DIR=$SCRIPT_DIR
PCAPS_DIR=$TOOLS_DIR/pcaps
BDDS_DIR=$SCRIPT_DIR/../bdds

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
    log_and_run bdd-to-c -target bdd-profiler -in $BDDS_DIR/$nf.bdd -out $nf-profiler.cpp
    log_and_run NF=$nf-profiler.cpp make -f $TOOLS_DIR/Makefile.dpdk
}

kvstore() {
    keys=$1
    churn=$2
    distribution=$3
    zipf_param=$4

    pcap="kvstore-k$keys-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        pcap="$pcap$zipf_param"
    fi

    warmup_pcap=$pcap-warmup
    pcap_dev_0=$pcap
    
    log_and_run $SYNTHESIZED_DIR/build/kvstore-profiler \
        --warmup 0:$PCAPS_DIR/$warmup_pcap.pcap \
        0:$PCAPS_DIR/$pcap_dev_0.pcap
    
    report="kvstore-profiler"
    report="$report-dev-0-pcap-$warmup_pcap-warmup"
    report="$report-dev-0-pcap-$pcap_dev_0"
    
    log_and_run bdd-visualizer -in $BDDS_DIR/kvstore.bdd -report $report.json -out $report.dot
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

    log_and_run $SYNTHESIZED_DIR/build/fw-profiler \
        --warmup 0:$PCAPS_DIR/$warmup_pcap.pcap \
        0:$PCAPS_DIR/$pcap_dev_0.pcap \
        1:$PCAPS_DIR/$pcap_dev_1.pcap \
    
    report="fw-profiler"
    report="$report-dev-0-pcap-$warmup_pcap-warmup"
    report="$report-dev-0-pcap-$pcap_dev_0"
    report="$report-dev-1-pcap-$pcap_dev_1"
    
    log_and_run bdd-visualizer -in $BDDS_DIR/fw.bdd -report $report.json -out $report.dot
}

gen_and_build_profiler kvstore

kvstore 10000 0 unif 0
kvstore 10000 0 zipf 0.9
kvstore 10000 0 zipf 0.99
kvstore 10000 0 zipf 1.26

kvstore 10000 1000000 unif 0
kvstore 10000 1000000 zipf 0.9
kvstore 10000 1000000 zipf 0.99
kvstore 10000 1000000 zipf 1.26

kvstore 10000 100000000 unif 0
kvstore 10000 100000000 zipf 0.9
kvstore 10000 100000000 zipf 0.99
kvstore 10000 100000000 zipf 1.26

gen_and_build_profiler fw

fw 10000 0 unif 0
fw 10000 0 zipf 0.9
fw 10000 0 zipf 0.99
fw 10000 0 zipf 1.26

fw 10000 1000000 unif 0
fw 10000 1000000 zipf 0.9
fw 10000 1000000 zipf 0.99
fw 10000 1000000 zipf 1.26

fw 10000 100000000 unif 0
fw 10000 100000000 zipf 0.9
fw 10000 100000000 zipf 0.99
fw 10000 100000000 zipf 1.26
