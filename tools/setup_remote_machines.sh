#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

$SCRIPT_DIR/sync_remote_machines.sh

echo "*********************************************"
echo "Setting up geodude"
echo "*********************************************"

ssh geodude "~/synapse-project/tools/setup_geodude.sh"

echo "*********************************************"
echo "Setting up graveler"
echo "*********************************************"

ssh graveler "~/synapse-project/tools/setup_graveler.sh"

echo "*********************************************"
echo "Setting up tofino1"
echo "*********************************************"

ssh tofino1 "~/synapse-project/tools/setup_tofino1.sh"

echo "*********************************************"
echo "Setting up tofino2"
echo "*********************************************"
ssh tofino2 "~/synapse-project/tools/setup_tofino2.sh"