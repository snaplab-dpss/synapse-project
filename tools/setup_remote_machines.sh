#!/bin/bash

set -euox pipefail

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

$SCRIPT_DIR/sync_remote_machines.sh

ssh geodude "~/synapse-project/tools/setup_geodude.sh"
ssh graveler "~/synapse-project/tools/setup_graveler.sh"
ssh tofino1 "~/synapse-project/tools/setup_tofino1.sh"
ssh tofino2 "~/synapse-project/tools/setup_tofino2.sh"