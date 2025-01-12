#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
ROOT_DIR=$SCRIPT_DIR/..

# Constants
DPDK_NFS_DIR="$ROOT_DIR/dpdk-nfs"
DEPS_DIR="$ROOT_DIR/deps"
PATHSFILE="$ROOT_DIR/paths.sh"
BUILDING_CORES=8

# Dependencies
DPDK_DIR="$DEPS_DIR/dpdk"
KLEE_DIR="$DEPS_DIR/klee"
LLVM_DIR="$DEPS_DIR/llvm"
KLEE_UCLIBC_DIR="$DEPS_DIR/klee-uclibc"
Z3_DIR="$DEPS_DIR/z3"
JSON_DIR="$DEPS_DIR/json"
SYNAPSE_DIR="$ROOT_DIR/synapse"

DPDK_TARGET=x86_64-native-linuxapp-gcc
DPDK_BUILD_DIR="$DPDK_DIR/$DPDK_TARGET"
KLEE_BUILD_PATH="$KLEE_DIR/build"
KLEE_UCLIBC_LIB_DIR="$KLEE_UCLIBC_DIR/lib"
LLVM_RELEASE_DIR="$LLVM_DIR/Release"
Z3_BUILD_DIR="$Z3_DIR/build"
JSON_BUILD_DIR="$JSON_DIR/build"
SYNAPSE_BUILD_DIR="$SYNAPSE_DIR/build"

# Checks if a variable is set in a file. If it is not in the file, add it with
# given value, otherwise change the value to match the current one.
# $1 : the name of the variable
# $2 : the value to set
add_var_to_paths_file() {
	if grep "^export $1" "$PATHSFILE" >/dev/null; then
		# Using sed directly to change the value would be dangerous as
		# we would need to correctly escape the value, which is hard.
		sed -i "/^export $1/d" "$PATHSFILE"
	fi
	echo "export ${1}=${2}" >> "$PATHSFILE"
	. "$PATHSFILE"
}

# Same as line, but without the unicity checks.
# $1 : the name of the variable
# $2 : the value to set
add_multiline_var_to_paths_file() {
	if ! grep "^export ${1}=${2}" "$PATHSFILE" >/dev/null; then
		echo "export ${1}=${2}" >> "$PATHSFILE"
		. "$PATHSFILE"
	fi
}

sync_submodules() {
	# Sync submodules only if the DPDK directory exists but is empty.
	if [ ! "$(ls -A $DPDK_DIR)" ]; then
		git submodule update --init --recursive
	fi
}

clean_dpdk() {
	rm -rf "$DPDK_BUILD_DIR"
}

source_install_dpdk() {
	echo "Installing DPDK..."

	add_var_to_paths_file "RTE_TARGET" "$DPDK_TARGET"
	add_var_to_paths_file "RTE_SDK" "$DPDK_DIR"
	add_multiline_var_to_paths_file "PKG_CONFIG_PATH" "$DPDK_BUILD_DIR/lib/x86_64-linux-gnu/pkgconfig/"

	pushd "$DPDK_DIR"
		meson setup "$DPDK_TARGET" --prefix="$DPDK_BUILD_DIR"

		pushd "$DPDK_BUILD_DIR"
			ninja
			ninja install
		popd
	popd

	echo "Done."
}

clean_z3() {
	rm -rf "$Z3_BUILD_DIR"
}

source_install_z3() {
	echo "Installing Z3..."

	pushd "$Z3_DIR"
		python3 scripts/mk_make.py -p "$Z3_BUILD_DIR"

		pushd "$Z3_BUILD_DIR"
			make -kj$BUILDING_CORES || make
			make install
			add_var_to_paths_file "Z3_DIR" "$Z3_DIR"
		popd
	popd

	echo "Done."
}

clean_llvm() {
	rm -rf "$LLVM_RELEASE_DIR"
	pushd "$LLVM_DIR"
		rm -rf Makefile.config
		make clean || true
	popd
}

source_install_llvm() {
	echo "Installing LLVM..."
	
	add_multiline_var_to_paths_file "PATH" "$LLVM_RELEASE_DIR/bin:\$PATH"

	pushd "$LLVM_DIR"
		CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 -frtti" \
		CC=cc \
		CXX=c++ \
			./configure \
					--enable-optimized \
					--disable-assertions \
					--enable-targets=host \
					--with-python=$(which python3) \
					--enable-cxx11
		
		make clean || true

		# Painfully slow, but allowing the compilation to use many cores
		# consumes a lot of memory, and crashes some systems.
		make -j$BUILDING_CORES
	popd

	add_var_to_paths_file "LLVM_DIR" "$LLVM_DIR"

	echo "Done."
}

clean_klee_uclibc() {
	rm -rf "$KLEE_UCLIBC_LIB_DIR"
}

source_install_klee_uclibc() {
	echo "Installing KLEE uclibc..."

	pushd "$KLEE_UCLIBC_DIR"
		if [ -d "/usr/lib/gcc/x86_64-linux-gnu" ]; then
			SYSTEM=x86_64-linux-gnu
		elif [ -d "/usr/lib/gcc/aarch64-linux-gnu" ]; then
			SYSTEM=aarch64-linux-gnu
		else
			echo "Unknown system, can't find GCC directory."
			exit 1
		fi

		# If there is a single version of GCC and it's a single digit, as in e.g. GCC 9 on Ubuntu 20.04,
		# our clang won't detect it because it expects a version in the format x.y.z with all components
		# so let's create a symlink
		# 0 -> nothing, 2 -> a single dot (because there is also \0)
		GCC_VER=$(ls -1 /usr/lib/gcc/$SYSTEM/ | sort -V | tail -n 1)
		
		if [ $(echo $GCC_VER | grep -Fo . | wc -c) -eq 0 ]; then
			sudo ln -s "/usr/lib/gcc/$SYSTEM/$GCC_VER" "/usr/lib/gcc/$SYSTEM/$GCC_VER.0.0" ;
		fi

		if [ $(echo $GCC_VER | grep -Fo . | wc -c) -eq 2 ]; then
			sudo ln -s "/usr/lib/gcc/$SYSTEM/$GCC_VER" "/usr/lib/gcc/$SYSTEM/$GCC_VER.0" ;
		fi

		./configure \
			--make-llvm-lib \
			--with-llvm-config="$LLVM_DIR/Release/bin/llvm-config" \
			--with-cc="$LLVM_DIR/Release/bin/clang"

		cp "$ROOT_DIR/setup/klee-uclibc.config" '.config'
		
		make clean
		make -kj$BUILDING_CORES
	popd

	echo "Done."
}

clean_klee() {
	rm -rf "$KLEE_BUILD_PATH"
}

source_install_klee() {
	echo "Installing KLEE..."

	add_var_to_paths_file "KLEE_DIR" "$KLEE_DIR"
	add_var_to_paths_file "KLEE_INCLUDE" "$KLEE_DIR/include"
	add_var_to_paths_file "KLEE_BUILD_PATH" "$KLEE_BUILD_PATH"

	add_multiline_var_to_paths_file "PATH" "$KLEE_BUILD_PATH/bin:\$PATH"

	pushd $KLEE_DIR
		./build.sh
	popd

	echo "Done."
}

clean_json() {
	rm -rf "$JSON_BUILD_DIR"
}

source_install_json() {
	echo "Installing nlohmann JSON..."

	add_var_to_paths_file "JSON_BUILD_PATH" "$JSON_BUILD_DIR"

	mkdir -p $JSON_BUILD_DIR
	pushd $JSON_BUILD_DIR
		cmake $JSON_DIR -DCMAKE_INSTALL_PREFIX=$JSON_BUILD_DIR
		make -j$BUILDING_CORES
	popd

	add_multiline_var_to_paths_file "PKG_CONFIG_PATH" "$JSON_BUILD_DIR:\$PKG_CONFIG_PATH"

	echo "Done."
}

build_libnf() {
	pushd "$DPDK_NFS_DIR"
		make lib
	popd

	add_multiline_var_to_paths_file "LD_LIBRARY_PATH" "$DPDK_NFS_DIR/build:\${LD_LIBRARY_PATH:-}"
	sudo ldconfig
}

build_synapse() {
	echo "Building Synapse..."

	pushd "$SYNAPSE_DIR"
		rm -rf build
		./build.sh
	popd

	add_multiline_var_to_paths_file "PATH" "$SYNAPSE_BUILD_DIR/bin:\$PATH"
	echo "Done."
}

# Pull every submodule if needed
sync_submodules

# Clean dependencies
clean_dpdk
clean_z3
clean_llvm
clean_klee_uclibc
clean_klee
clean_json

# Install dependencies
source_install_dpdk
source_install_z3
source_install_llvm
source_install_klee_uclibc
source_install_klee
source_install_json
build_libnf
build_synapse
