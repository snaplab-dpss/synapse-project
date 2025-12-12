#!/bin/bash

set -euo pipefail

sudo apt-get update -qq
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
	clang-format
