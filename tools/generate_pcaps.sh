#!/bin/bash

set -euox pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PCAPS_DIR=$SCRIPT_DIR/pcaps

cd $PCAPS_DIR

pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --uniform
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.9
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.99
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 1.26

pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --uniform
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.9
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.99
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 1.26

pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --uniform
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.9
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.99
pcap-generator-kvs --seed 0 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 1.26

pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --uniform
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.9
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 0.99
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 0 --zipf --zipf-param 1.26

pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --uniform
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.9
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 0.99
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 1000000 --zipf --zipf-param 1.26

pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --uniform
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.9
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 0.99
pcap-generator-fw --seed 0 --lan-devs 1 --packets 1000000 --flows 10000 --churn 100000000 --zipf --zipf-param 1.26