#!/bin/bash

set -euo pipefail

# KLEE builds against the system's LLVM: Ubuntu 24.04 ships it, older releases get it from apt.llvm.org.
LLVM_VERSION=16

sudo apt-get update -qq

if ! apt-cache show llvm-$LLVM_VERSION-dev > /dev/null 2>&1; then
	echo "Adding apt.llvm.org for LLVM $LLVM_VERSION..."
	sudo apt-get install -yqq wget lsb-release software-properties-common gnupg
	wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh
	sudo bash /tmp/llvm.sh $LLVM_VERSION
	rm -f /tmp/llvm.sh
fi

sudo apt-get install -yqq \
	man \
	build-essential \
	wget curl \
	git \
	vim \
	cmake \
	tzdata \
	tmux \
	iputils-ping \
	iproute2 \
	net-tools \
	tcpreplay \
	iperf \
	psmisc \
	htop \
	gdb \
	xdg-utils \
	time \
	parallel \
	libcanberra-gtk-module libcanberra-gtk3-module \
	zsh \
	python3-pip python3-venv python3-scapy python-is-python3 python3-pyelftools xdot \
	gperf libgoogle-perftools-dev libpcap-dev meson pkg-config \
	bison flex zlib1g-dev libncurses5-dev libpcap-dev \
	opam m4 libgmp-dev \
	linux-headers-generic libnuma-dev \
	clang-format \
	llvm-$LLVM_VERSION-dev llvm-$LLVM_VERSION-tools clang-$LLVM_VERSION libsqlite3-dev ninja-build
