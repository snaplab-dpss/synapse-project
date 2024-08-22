#!/bin/bash

set -euox pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR

make -j

./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --uniform
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.9
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.99
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 1.26

./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --uniform
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.9
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.99
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 1.26

./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --uniform
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.9
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.99
./build/pcap_generator_kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 1.26

./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --uniform
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.9
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.99
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 1.26

./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --uniform
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.9
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.99
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 1.26

./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --uniform
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.9
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.99
./build/pcap_generator_fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 1.26