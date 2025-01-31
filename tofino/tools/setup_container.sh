#!/bin/bash

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

# Setup veth interfaces
sudo $SCRIPT_DIR/veth_setup.sh

# Build and install the libsycon library
$SCRIPT_DIR/../libsycon/install.sh

# Install scapy globally
sudo pip3 install scapy