#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_DIR=$SCRIPT_DIR/build
DEBUG_BUILD_DIR=$BUILD_DIR/debug
RELEASE_BUILD_DIR=$BUILD_DIR/release

build_debug() {
    mkdir -p $DEBUG_BUILD_DIR
    pushd $DEBUG_BUILD_DIR
        cmake -DCMAKE_BUILD_TYPE=Debug ../.. -G Ninja
        ninja
    popd
}

build_release() {
    mkdir -p $RELEASE_BUILD_DIR
    pushd $RELEASE_BUILD_DIR
        cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../.. -G Ninja
        ninja
    popd
}

build_debug
build_release
