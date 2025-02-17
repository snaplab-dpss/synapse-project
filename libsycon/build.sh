#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

BUILD_DIR=$SCRIPT_DIR/build

mkdir -p $BUILD_DIR

cmake -G "Ninja Multi-Config" -B $BUILD_DIR
cmake --build build --config Debug
cmake --build build --config RelWithDebInfo
