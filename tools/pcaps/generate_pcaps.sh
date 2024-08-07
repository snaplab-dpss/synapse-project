#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR

make -j

./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 0 --uniform
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.9
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.99
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 1.26

./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --uniform
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.9
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.99
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 1.26

./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --uniform
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.9
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.99
./build/kvstore_pcap_generator --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 1.26