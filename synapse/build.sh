#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

build() {
  touch $PROJECT_DIR/CMakeLists.txt

  [ -d "$BUILD_DIR" ] || mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  [ -f "Makefile" ] || cmake \
    -DCMAKE_CXX_FLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" \
    -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
    $PROJECT_DIR

  make -kj $(nproc) || exit 1
}

build_profile() {
  touch $PROJECT_DIR/CMakeLists.txt
  
  [ -d "$BUILD_DIR" ] || mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  [ -f "Makefile" ] || cmake \
    -DCMAKE_BUILD_TYPE="RelWithDebInfo" \
    -DCMAKE_EXE_LINKER_FLAGS="-pg" \
    -DCMAKE_SHARED_LINKER_FLAGS="-pg" $PROJECT_DIR

  make -kj $(nproc) || exit 1
}

build
# build_profile
