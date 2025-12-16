FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG GCC_VERSION=11

RUN apt-get update && apt-get install -y sudo adduser
RUN adduser ubuntu sudo
RUN echo '%ubuntu ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER ubuntu
WORKDIR /home/ubuntu

RUN sudo apt-get update -qq && sudo apt-get install -yqq \
    man build-essential wget curl git vim tzdata tmux zsh time parallel \
    iputils-ping iproute2 net-tools tcpreplay iperf \
    psmisc htop gdb xdg-utils clang-format \
    python3-pip python3-venv python3-scapy python3-pyelftools python-is-python3 xdot \
    gperf libgoogle-perftools-dev libpcap-dev meson pkg-config \
    bison flex zlib1g-dev libncurses5-dev libpcap-dev \
    opam m4 libgmp-dev \
    gcc-$GCC_VERSION g++-$GCC_VERSION cmake \
    linux-headers-generic libnuma-dev

RUN sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 100
RUN sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION 100

RUN sudo update-alternatives --set g++ /usr/bin/g++-$GCC_VERSION
RUN sudo update-alternatives --set gcc /usr/bin/gcc-$GCC_VERSION

RUN sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 100
RUN sudo update-alternatives --set cc /usr/bin/gcc

RUN sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 100
RUN sudo update-alternatives --set c++ /usr/bin/g++

RUN sudo dpkg-reconfigure --frontend noninteractive tzdata
RUN mkdir /home/ubuntu/.ssh
RUN chown -R ubuntu:ubuntu /home/ubuntu/.ssh

RUN curl -fsSL https://raw.githubusercontent.com/zimfw/install/master/install.zsh | zsh
RUN echo "set -g default-terminal \"screen-256color\"" >> /home/ubuntu/.tmux.conf
RUN echo "set-option -g default-shell /bin/zsh" >> /home/ubuntu/.tmux.conf
RUN sudo chsh -s $(which zsh) 
RUN echo "skip_global_compinit=1" >> /home/ubuntu/.zshenv
RUN echo "source ~/.profile" >> /home/ubuntu/.zshrc

RUN echo "source ~/synapse-project/paths.sh 2>/dev/null || true" >> /home/ubuntu/.zshrc

COPY --chown=ubuntu:ubuntu tools/deps/install_package_deps.sh .
RUN chmod +x install_package_deps.sh
RUN ./install_package_deps.sh
RUN rm install_package_deps.sh

WORKDIR synapse-project

CMD [ "/bin/zsh" ]