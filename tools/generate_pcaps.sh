#!/bin/bash

########################################################################################################################################
#   Note: This is all based on our specific testbed configuration.
#   You may want to adjust the port configuration on each NF.
#   For example, our NFs will run on a Tofino programmable switch with 32 ports (1 to 32). However, we only have ports 3-32 connected.
#   That is why we generate pcaps only for devices 2-31 (devices start by 0).
########################################################################################################################################

set -euox pipefail

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
PROJECT_DIR=$(realpath $SCRIPT_DIR/..)

PCAPS_DIR=$PROJECT_DIR/pcaps
SYNAPSE_DIR=$PROJECT_DIR/synapse

mkdir -p $PCAPS_DIR

TOTAL_PACKETS=10000000
PACKET_SIZE=200

####################
#       Echo       #
####################

generate_pcaps_echo() {
    flows=40000
    devs="2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31"
    parallel \
        -j $(nproc) \
        eval \
        $SYNAPSE_DIR/build/bin/pcap-generator-echo \
        --out $PCAPS_DIR \
        --packets $TOTAL_PACKETS \
        --flows $flows \
        --packet-size $PACKET_SIZE \
        --churn {1} \
        --traffic {2} \
        --devs $devs \
        --seed 0 \
        ::: 0 1000 10000 100000 1000000 \
        ::: "uniform" "zipf --zipf-param 0.2" "zipf --zipf-param 0.4" "zipf --zipf-param 0.6" "zipf --zipf-param 0.8" "zipf --zipf-param 1.0" "zipf --zipf-param 1.2"
}

#########################
#       Forwarder       #
#########################

generate_pcaps_fwd() {
    flows=40000
    devs="2,3 4,5 6,7 8,9 10,11 12,13 14,15 16,17 18,19 20,21 22,23 24,25 26,27 28,29 30,31"
    parallel \
        -j $(nproc) \
        eval \
        $SYNAPSE_DIR/build/bin/pcap-generator-fwd \
        --out $PCAPS_DIR \
        --packets $TOTAL_PACKETS \
        --flows $flows \
        --packet-size $PACKET_SIZE \
        --churn {1} \
        --traffic {2} \
        --devs $devs \
        --seed 0 \
        ::: 0 1000 10000 100000 1000000 \
        ::: "uniform" "zipf --zipf-param 0.2" "zipf --zipf-param 0.4" "zipf --zipf-param 0.6" "zipf --zipf-param 0.8" "zipf --zipf-param 1.0" "zipf --zipf-param 1.2"
}

###################
#     Firewall    #
###################

generate_pcaps_fw() {
    flows=40000
    devs="2,3 4,5 6,7 8,9 10,11 12,13 14,15 16,17 18,19 20,21 22,23 24,25 26,27 28,29 30,31"
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
        ::: 0 1000 10000 100000 1000000 \
        ::: "uniform" "zipf --zipf-param 0.2" "zipf --zipf-param 0.4" "zipf --zipf-param 0.6" "zipf --zipf-param 0.8" "zipf --zipf-param 1.0" "zipf --zipf-param 1.2"
}

##############################
# Network Address Translator #
##############################

generate_pcaps_nat() {
    flows=40000
    devs="2,3 4,5 6,7 8,9 10,11 12,13 14,15 16,17 18,19 20,21 22,23 24,25 26,27 28,29 30,31"
    parallel \
        -j $(nproc) \
        eval \
        $SYNAPSE_DIR/build/bin/pcap-generator-nat \
        --out $PCAPS_DIR \
        --packets $TOTAL_PACKETS \
        --flows $flows \
        --packet-size $PACKET_SIZE \
        --churn {1} \
        --traffic {2} \
        --devs $devs \
        --seed 0 \
        ::: 0 1000 10000 100000 1000000 \
        ::: "uniform" "zipf --zipf-param 0.2" "zipf --zipf-param 0.4" "zipf --zipf-param 0.6" "zipf --zipf-param 0.8" "zipf --zipf-param 1.0" "zipf --zipf-param 1.2"
}

###################
# Key Value Store #
###################

generate_pcaps_kvs() {
    flows=100000 # Uses more flows than the other NFs to have more flows than cache capacity.
    devs="2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31"
    parallel \
        -j $(nproc) \
        eval \
        $SYNAPSE_DIR/build/bin/pcap-generator-kvs \
        --out $PCAPS_DIR \
        --packets $TOTAL_PACKETS \
        --flows $flows \
        --churn {1} \
        --traffic {2} \
        --devs $devs \
        --seed 0 \
        ::: 0 1000 10000 100000 1000000 \
        ::: "uniform" "zipf --zipf-param 0.2" "zipf --zipf-param 0.4" "zipf --zipf-param 0.6" "zipf --zipf-param 0.8" "zipf --zipf-param 1.0" "zipf --zipf-param 1.2"
}

######################
# Connection Limiter #
######################

generate_pcaps_cl() {
    flows=40000
    devs="2,3 4,5 6,7 8,9 10,11 12,13 14,15 16,17 18,19 20,21 22,23 24,25 26,27 28,29 30,31"
    parallel \
        -j $(nproc) \
        eval \
        $SYNAPSE_DIR/build/bin/pcap-generator-cl \
        --out $PCAPS_DIR \
        --packets $TOTAL_PACKETS \
        --flows $flows \
        --packet-size $PACKET_SIZE \
        --churn {1} \
        --traffic {2} \
        --devs $devs \
        --seed 0 \
        ::: 0 1000 10000 100000 1000000 \
        ::: "uniform" "zipf --zipf-param 0.2" "zipf --zipf-param 0.4" "zipf --zipf-param 0.6" "zipf --zipf-param 0.8" "zipf --zipf-param 1.0" "zipf --zipf-param 1.2"
}

# generate_pcaps_echo
# generate_pcaps_fwd
# generate_pcaps_fw
# generate_pcaps_nat
generate_pcaps_kvs
# generate_pcaps_cl