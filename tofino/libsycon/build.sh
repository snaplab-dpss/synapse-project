#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

ROOT_DIR=$SCRIPT_DIR
DEBUG_BUILD_DIR=$ROOT_DIR/Debug
RELEASE_BUILD_DIR=$ROOT_DIR/Release

build_debug() {
    if [ ! -d $DEBUG_BUILD_DIR ]; then
        mkdir -p $DEBUG_BUILD_DIR

        pushd $DEBUG_BUILD_DIR
            cmake \
                -DCMAKE_BUILD_TYPE=Debug \
                -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=True \
                $ROOT_DIR \
                -G Ninja
        popd
    fi

    ninja -C $DEBUG_BUILD_DIR
}

build_release() {
    if [ ! -d $RELEASE_BUILD_DIR ]; then
        mkdir -p $RELEASE_BUILD_DIR

        pushd $RELEASE_BUILD_DIR
            cmake \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=True \
                $ROOT_DIR \
                -G Ninja
        popd
    fi

    ninja -C $RELEASE_BUILD_DIR
}

build_debug
build_release