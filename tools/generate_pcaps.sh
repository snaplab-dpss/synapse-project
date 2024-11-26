#!/bin/bash

set -euox pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PCAPS_DIR=$SCRIPT_DIR/pcaps

cd $PCAPS_DIR

# ###################
# #     Firewall    #
# ###################

# parallel \
#     -j $(nproc) \
#     eval \
#     pcap-generator-fw \
#     --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn {1} {2} \
#     ::: 0 1000000 10000000 100000000 \
#     ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"

# ##############################
# # Network Address Translator #
# ##############################

# parallel \
#     -j $(nproc) \
#     eval \
#     pcap-generator-nat \
#     --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn {1} {2} \
#     ::: 0 1000000 10000000 100000000 \
#     ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"

###################
# Key Value Store #
###################

# Uses more flows than the other NFs to have more flows than cache capacity.

parallel \
    -j $(nproc) \
    eval \
    pcap-generator-kvs \
    --seed 0 --packets 1000000 --flows 100000 --churn {1} {2} \
    ::: 0 1000000 10000000 100000000 \
    ::: "--uniform" "--zipf --zipf-param 0.9" "--zipf --zipf-param 0.99" "--zipf --zipf-param 1.26"
