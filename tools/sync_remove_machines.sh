#!/bin/bash

set -euo pipefail

sync() {
    host=$1
    path_to_repo=$2
    echo "***************************"
    echo "Synchronizing host $host"
    echo "***************************"
    ssh $host "cd $path_to_repo; git reset HEAD --hard; git pull origin main"
}

sync tofino2-b2 /home/user/synapse
sync graveler /home/fcp/synapse