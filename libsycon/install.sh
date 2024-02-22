#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

ROOT_DIR=$SCRIPT_DIR
DEBUG_BUILD_DIR=$ROOT_DIR/Debug
RELEASE_BUILD_DIR=$ROOT_DIR/Release
BUILD_SCRIPT=$ROOT_DIR/build.sh

build() {
    $BUILD_SCRIPT
}

install_debug() {
    ninja -C $DEBUG_BUILD_DIR
    sudo ninja -C $DEBUG_BUILD_DIR install
    sudo ldconfig
}

install_release() {
    ninja -C $RELEASE_BUILD_DIR
    sudo ninja -C $RELEASE_BUILD_DIR install
    sudo ldconfig
}

build
install_debug
install_release
