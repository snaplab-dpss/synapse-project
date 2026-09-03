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

# Use `cmake --install` rather than `sudo ninja install`: running ninja under
# sudo rewrites its .ninja_log as root inside the user-owned build tree, which
# then blocks the next unprivileged `ninja` build with a permission error.
# `cmake --install` runs only the install rules and never touches .ninja_log.
install_debug() {
    pushd $DEBUG_BUILD_DIR
        sudo cmake --install .
        sudo ldconfig
    popd
}

install_release() {
    pushd $RELEASE_BUILD_DIR
        sudo cmake --install .
        sudo ldconfig
    popd
}

build
install_debug
install_release
