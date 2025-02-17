#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_DIR=$SCRIPT_DIR/build
BUILD_SCRIPT=$SCRIPT_DIR/build.sh

build() {
    $BUILD_SCRIPT
}

install_debug() {
    sudo cmake --install build --config Debug
    sudo ldconfig
}

install_release() {
    sudo cmake --install build --config RelWithDebInfo
    sudo ldconfig
}

build
install_debug
install_release
