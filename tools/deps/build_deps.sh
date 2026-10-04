#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
ROOT_DIR=$(realpath $SCRIPT_DIR/../..)

# Constants
DPDK_NFS_DIR="$ROOT_DIR/dpdk-nfs"
DEPS_DIR="$ROOT_DIR/deps"
PATHSFILE="$ROOT_DIR/paths.sh"

# Dependencies
DPDK_DIR="$DEPS_DIR/dpdk"
KLEE_DIR="$DEPS_DIR/klee"
KLEE_UCLIBC_DIR="$DEPS_DIR/klee-uclibc"
Z3_DIR="$DEPS_DIR/z3"
JSON_DIR="$DEPS_DIR/json"
SYNAPSE_DIR="$ROOT_DIR/synapse"

DPDK_TARGET=x86_64-native-linuxapp-gcc
DPDK_BUILD_DIR="$DPDK_DIR/$DPDK_TARGET"
KLEE_BUILD_PATH="$KLEE_DIR/build"
KLEE_UCLIBC_LIB_DIR="$KLEE_UCLIBC_DIR/lib"
Z3_BUILD_DIR="$Z3_DIR/build"
JSON_BUILD_DIR="$JSON_DIR/build"
SYNAPSE_BUILD_DIR="$SYNAPSE_DIR/build"

LLVM_VERSION=16
LLVM_DIR="/usr/lib/llvm-$LLVM_VERSION"


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

clean_dpdk() {
	rm -rf "$DPDK_BUILD_DIR"
}

source_install_dpdk() {
	echo "Installing DPDK..."

	add_var_to_paths_file "RTE_TARGET" "$DPDK_TARGET"
	add_var_to_paths_file "RTE_SDK" "$DPDK_DIR"
	add_multiline_var_to_paths_file "PKG_CONFIG_PATH" "$DPDK_BUILD_DIR/lib/x86_64-linux-gnu/pkgconfig/"

	pushd "$DPDK_DIR"
		rm -rf "$DPDK_BUILD_DIR" || true
		meson setup "$DPDK_TARGET" --prefix="$DPDK_BUILD_DIR" || true

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
		# Stale generated files break the build (https://github.com/Z3Prover/z3/issues/6552);
		# a copy without git history (the Docker image) has none.
		if git rev-parse --git-dir > /dev/null 2>&1; then git clean -fx; fi
		python3 scripts/mk_make.py -p "$Z3_BUILD_DIR"

		pushd "$Z3_BUILD_DIR"
			make -j$(nproc) || make
			make install
			add_var_to_paths_file "Z3_DIR" "$Z3_DIR"
		popd
	popd

	echo "Done."
}

setup_llvm() {
	echo "Using the system's LLVM $LLVM_VERSION..."

	if [ ! -x "$LLVM_DIR/bin/llvm-config" ]; then
		echo "LLVM $LLVM_VERSION not found at $LLVM_DIR: run tools/deps/install_package_deps.sh first."
		exit 1
	fi

	add_var_to_paths_file "LLVM_DIR" "$LLVM_DIR"
	add_multiline_var_to_paths_file "PATH" "$LLVM_DIR/bin:\$PATH"

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

		GCC_VER=$(gcc --version | head -n 1 | awk '{print $3}' | cut -d '.' -f 1)
		
		if [ $(echo $GCC_VER | grep -Fo . | wc -c) -eq 0 ]; then
			sudo ln -s "/usr/lib/gcc/$SYSTEM/$GCC_VER" "/usr/lib/gcc/$SYSTEM/$GCC_VER.0.0" ;
		fi

		if [ $(echo $GCC_VER | grep -Fo . | wc -c) -eq 2 ]; then
			sudo ln -s "/usr/lib/gcc/$SYSTEM/$GCC_VER" "/usr/lib/gcc/$SYSTEM/$GCC_VER.0" ;
		fi

		./configure \
			--make-llvm-lib \
			--with-llvm-config="$LLVM_DIR/bin/llvm-config" \
			--with-cc="$LLVM_DIR/bin/clang"

		cp "$ROOT_DIR/setup/klee-uclibc.config" '.config'
		
		make clean
		make -j$(nproc)
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

	# KLEE copies klee-uclibc's libc.a into its runtime at build time: build klee-uclibc first.
	# PIC because synapse links the KLEE archives into its shared libraries; no tcmalloc so that
	# synapse does not inherit it as a dependency.
	pushd $KLEE_DIR
		[ -d "$KLEE_BUILD_PATH" ] || mkdir -p "$KLEE_BUILD_PATH"
		pushd $KLEE_BUILD_PATH
			[ -f "build.ninja" ] || \
				cmake -G Ninja \
				-DCMAKE_BUILD_TYPE=RelWithDebInfo \
				-DLLVM_DIR="$LLVM_DIR/lib/cmake/llvm" \
				-DLLVMCC="$LLVM_DIR/bin/clang" \
				-DLLVMCXX="$LLVM_DIR/bin/clang++" \
				-DENABLE_SOLVER_Z3=ON \
				-DCMAKE_PREFIX_PATH="$Z3_BUILD_DIR" \
				-DENABLE_SOLVER_STP=OFF \
				-DENABLE_SOLVER_METASMT=OFF \
				-DENABLE_POSIX_RUNTIME=ON \
				-DKLEE_UCLIBC_PATH="$KLEE_UCLIBC_DIR" \
				-DENABLE_UNIT_TESTS=OFF \
				-DENABLE_SYSTEM_TESTS=OFF \
				-DENABLE_KLEE_ASSERTS=ON \
				-DENABLE_TCMALLOC=OFF \
				-DENABLE_DOCS=OFF \
				-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
				$KLEE_DIR

			ninja || exit 1
			# KLEE copies the archive when it configures, not when it builds: a klee-uclibc
			# rebuilt afterwards would otherwise not reach the runtime.
			cp "$KLEE_UCLIBC_LIB_DIR/libc.a" "$KLEE_BUILD_PATH/runtime/lib/klee-uclibc.bca"
		popd
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
		make -j$(nproc)
	popd

	add_multiline_var_to_paths_file "PKG_CONFIG_PATH" "$JSON_BUILD_DIR:\$PKG_CONFIG_PATH"

	echo "Done."
}

# libnf is built with `make -C dpdk-nfs lib`; only its directory goes on the library path here.
add_libnf_to_paths() {
	add_multiline_var_to_paths_file "LD_LIBRARY_PATH" "$DPDK_NFS_DIR/build:\${LD_LIBRARY_PATH:-}"
}

# Synapse itself is built with synapse/build-release.sh; only its binaries go on the PATH here.
add_synapse_to_paths() {
	add_multiline_var_to_paths_file "PATH" "$SYNAPSE_BUILD_DIR/bin:\$PATH"
}

install() {
	source_install_dpdk
	source_install_z3
	setup_llvm
	source_install_klee_uclibc
	source_install_klee
	source_install_json
	add_libnf_to_paths
	add_synapse_to_paths
}

reinstall() {
	clean_dpdk
	clean_z3
	clean_klee_uclibc
	clean_klee
	clean_json
	install
}

# reinstall
install