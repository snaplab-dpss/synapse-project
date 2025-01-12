#!/bin/bash

set -euo pipefail

OCAML_RELEASE='4.06.0'
GCC_VERSION='10'

install_system_deps() {
	sudo apt-get update -qq
	sudo apt-get install -yqq \
		man \
		build-essential \
		wget curl \
		git \
		vim \
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
		libcanberra-gtk-module libcanberra-gtk3-module \
		zsh \
		python3-pip python3-venv python3-scapy python-is-python3 xdot \
		gperf libgoogle-perftools-dev libpcap-dev meson pkg-config \
		bison flex zlib1g-dev libncurses5-dev libpcap-dev \
		opam m4 libgmp-dev \
		gcc-$GCC_VERSION g++-$GCC_VERSION \
		linux-headers-generic
}

set_gcc_version() {
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 100
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION 100

	sudo update-alternatives --set g++ /usr/bin/g++-$GCC_VERSION
	sudo update-alternatives --set gcc /usr/bin/gcc-$GCC_VERSION

	sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 100
	sudo update-alternatives --set cc /usr/bin/gcc

	sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 100
	sudo update-alternatives --set c++ /usr/bin/g++
}

install_system_deps
set_gcc_version