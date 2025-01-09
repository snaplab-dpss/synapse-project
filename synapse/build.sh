#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

build() {
  [ -d "$BUILD_DIR" ] || mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
    $PROJECT_DIR

  # make -kj $(nproc) || exit 1
  ninja
}

build_profile() { 
  [ -d "$BUILD_DIR" ] || mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
    -DCMAKE_EXE_LINKER_FLAGS="-pg" \
    -DCMAKE_SHARED_LINKER_FLAGS="-pg" $PROJECT_DIR

  make -kj $(nproc) || exit 1
}

build
# build_profile
