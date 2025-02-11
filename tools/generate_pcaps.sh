#!/bin/bash

set -euox pipefail

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
PROJECT_DIR=$(realpath $SCRIPT_DIR/..)

PCAPS_DIR=$PROJECT_DIR/pcaps
SYNAPSE_DIR=$PROJECT_DIR/synapse

mkdir -p $PCAPS_DIR

TOTAL_PACKETS=10000000
PACKET_SIZE=200

###################
#       NOP       #
###################

nop() {
    flows=10000
    devs="0,1 2,3 4,5 6,7 8,9 10,11 12,13 14,15 16,17 18,19 20,21 22,23 24,25 26,27 28,29"
    parallel \
        -j $(nproc) \
        eval \
        $SYNAPSE_DIR/build/bin/pcap-generator-nop \
        --out $PCAPS_DIR \
        --packets $TOTAL_PACKETS \
        --flows $flows \
        --packet-size $PACKET_SIZE \
        --churn {1} \
        --traffic {2} \
        --devs $devs \
        --seed 0 \
        ::: 0 1000000 10000000 \
        ::: "uniform" "zipf --zipf-param 0.9" "zipf --zipf-param 0.99" "zipf --zipf-param 1.26"
}

###################
#     Firewall    #
###################

# FIXME:
# fw() {
#     parallel \
#         -j $(nproc) \
#         eval \
#         $SYNAPSE_DIR/build/bin/pcap-generator-fw \
#         --seed 0 --lan-devs 1 --packets $TOTAL_PACKETS --flows 10000 --churn {1} {2} \
#         ::: 0 1000000 10000000 \
#         ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"
# }

fw() {
    flows=10000
    devs="0,1 2,3 4,5 6,7 8,9 10,11 12,13 14,15 16,17 18,19 20,21 22,23 24,25 26,27 28,29"
    parallel \
        -j $(nproc) \
        eval \
        $SYNAPSE_DIR/build/bin/pcap-generator-fw \
        --out $PCAPS_DIR \
        --packets $TOTAL_PACKETS \
        --flows $flows \
        --packet-size $PACKET_SIZE \
        --churn {1} \
        --traffic {2} \
        --devs $devs \
        --seed 0 \
        ::: 0 1000000 10000000 \
        ::: "uniform" "zipf --zipf-param 0.9" "zipf --zipf-param 0.99" "zipf --zipf-param 1.26"
}

##############################
# Network Address Translator #
##############################

# FIXME:
# nat() {
#     parallel \
#         -j $(nproc) \
#         eval \
#         $SYNAPSE_DIR/build/bin/pcap-generator-nat \
#         --seed 0 --lan-devs 1 --packets $TOTAL_PACKETS --flows 10000 --churn {1} {2} \
#         ::: 0 1000000 10000000 \
#         ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"
# }

###################
# Key Value Store #
###################

# Uses more flows than the other NFs to have more flows than cache capacity.

# FIXME:
# kvs() {
#     parallel \
#         -j $(nproc) \
#         eval \
#         $SYNAPSE_DIR/build/bin/pcap-generator-kvs \
#         --seed 0 --packets $TOTAL_PACKETS --flows 100000 --churn {1} {2} \
#         ::: 0 1000000 10000000 \
#         ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"
# }

nop
fw
# nat
# kvs