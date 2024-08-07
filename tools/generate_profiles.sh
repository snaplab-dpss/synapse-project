#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SYNTHESIZED_DIR=$SCRIPT_DIR/../synthesized
TOOLS_DIR=$SCRIPT_DIR
PCAPS_DIR=$TOOLS_DIR/pcaps
BDDS_DIR=$SCRIPT_DIR/../bdds

# Everything will be done inside the synthesized directory
cd $SYNTHESIZED_DIR

build() {
    nf=$1
    NF=$nf make -f $TOOLS_DIR/Makefile.dpdk
}

kvstore() {
    bdd=$1
    seed=$2
    packets=$3
    keys=$4
    churn=$5
    distribution=$6
    zipf_param=$7

    pcap="kvstore-seed-$seed-packets-$packets-keys-$keys-churn-$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        pcap="$pcap-$zipf_param"
    fi
    
    $SYNTHESIZED_DIR/build/kvstore-profiler \
        --warmup 0:$PCAPS_DIR/$pcap-warmup.pcap \
        0:$PCAPS_DIR/$pcap.pcap
    
    report="kvstore-profiler"
    report="$report-dev-0-pcap-$pcap-warmup-warmup"
    report="$report-dev-0-pcap-$pcap"
    
    bdd-visualizer -in $BDDS_DIR/$bdd -report "$report.json" -out "$report.dot"

}

build kvstore-profiler.cpp

kvstore kvstore.bdd 0 1000000 10000 0 uniform 0
kvstore kvstore.bdd 0 1000000 10000 0 zipf 0.9
kvstore kvstore.bdd 0 1000000 10000 0 zipf 0.99
kvstore kvstore.bdd 0 1000000 10000 0 zipf 1.26

kvstore kvstore.bdd 0 1000000 10000 1000000 uniform 0
kvstore kvstore.bdd 0 1000000 10000 1000000 zipf 0.9
kvstore kvstore.bdd 0 1000000 10000 1000000 zipf 0.99
kvstore kvstore.bdd 0 1000000 10000 1000000 zipf 1.26

kvstore kvstore.bdd 0 1000000 10000 100000000 uniform 0
kvstore kvstore.bdd 0 1000000 10000 100000000 zipf 0.9
kvstore kvstore.bdd 0 1000000 10000 100000000 zipf 0.99
kvstore kvstore.bdd 0 1000000 10000 100000000 zipf 1.26
