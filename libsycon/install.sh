#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_SCRIPT=$SCRIPT_DIR/build.sh

BUILD_DIR=$SCRIPT_DIR/build
DEBUG_BUILD_DIR=$BUILD_DIR/debug
RELEASE_BUILD_DIR=$BUILD_DIR/release

build() {
    $BUILD_SCRIPT
}

install_debug() {
    pushd $DEBUG_BUILD_DIR
        sudo ninja install
        sudo ldconfig
    popd
}

install_release() {
    pushd $RELEASE_BUILD_DIR
        sudo ninja install
        sudo ldconfig
    popd
}

build
install_debug
install_release
