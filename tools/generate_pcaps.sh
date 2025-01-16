#!/bin/bash

set -euox pipefail

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
PROJECT_DIR=$(realpath $SCRIPT_DIR/..)

PCAPS_DIR=$PROJECT_DIR/pcaps
SYNAPSE_DIR=$PROJECT_DIR/synapse

mkdir -p $PCAPS_DIR
cd $PCAPS_DIR

TOTAL_PACKETS=10000000

###################
#     Firewall    #
###################

parallel \
    -j $(nproc) \
    eval \
    $SYNAPSE_DIR/build/bin/pcap-generator-fw \
    --seed 0 --lan-devs 1 --packets $TOTAL_PACKETS --flows 10000 --churn {1} {2} \
    ::: 0 1000000 10000000 \
    ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"

##############################
# Network Address Translator #
##############################

parallel \
    -j $(nproc) \
    eval \
    $SYNAPSE_DIR/build/bin/pcap-generator-nat \
    --seed 0 --lan-devs 1 --packets $TOTAL_PACKETS --flows 10000 --churn {1} {2} \
    ::: 0 1000000 10000000 \
    ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"

###################
# Key Value Store #
###################

# Uses more flows than the other NFs to have more flows than cache capacity.

parallel \
    -j $(nproc) \
    eval \
    $SYNAPSE_DIR/build/bin/pcap-generator-kvs \
    --seed 0 --packets $TOTAL_PACKETS --flows 100000 --churn {1} {2} \
    ::: 0 1000000 10000000 \
    ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"
