#!/usr/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

ROOT_DIR="$SCRIPT_DIR/../"
DEPS_DIR="$ROOT_DIR/deps"

sync() {
    dep=$1
    branch=$2

    pushd "$DEPS_DIR/$dep"
        remote_https=$(git remote -v | head -n 1 | awk '{print $2}')

        if ! grep -q "git@github.com" <<< $remote_https; then
            remote_ssh=$(echo $remote_https | sed -En "s/https:\/\/github.com\/(.*)/git@github.com:\1/p")
            git remote set-url origin $remote_ssh
        fi

        git fetch
        git checkout $branch
    popd
}

pushd $ROOT_DIR
    git submodule update --init --recursive
popd

sync dpdk synapse
sync klee main
sync klee-uclibc synapse
sync llvm synapse
sync z3 synapse
