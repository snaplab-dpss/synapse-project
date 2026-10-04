FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG USER=synapse

# The container's user keeps the base image's uid 1000 (the stock "ubuntu" account is dropped),
# so files it writes into the mounted project belong to the host's user.
RUN apt-get update && apt-get install -y sudo adduser
RUN userdel -r ubuntu 2>/dev/null || true
RUN adduser --disabled-password --gecos "" --uid 1000 $USER
RUN adduser $USER sudo
RUN echo "%$USER ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER $USER
WORKDIR /home/$USER

RUN sudo apt-get update -qq && sudo apt-get install -yqq \
    man build-essential wget curl git vim tzdata tmux zsh time parallel \
    iputils-ping iproute2 net-tools tcpreplay iperf \
    psmisc htop gdb xdg-utils clang-format \
    python3-pip python3-venv python3-scapy python3-pyelftools python-is-python3 xdot \
    gperf libgoogle-perftools-dev libpcap-dev meson pkg-config \
    bison flex zlib1g-dev libncurses5-dev libpcap-dev \
    opam m4 libgmp-dev \
    cmake \
    linux-headers-generic libnuma-dev

RUN sudo dpkg-reconfigure --frontend noninteractive tzdata
RUN mkdir /home/$USER/.ssh

RUN curl -fsSL https://raw.githubusercontent.com/zimfw/install/master/install.zsh | zsh
RUN echo "set -g default-terminal \"screen-256color\"" >> /home/$USER/.tmux.conf
RUN echo "set-option -g default-shell /bin/zsh" >> /home/$USER/.tmux.conf
RUN sudo chsh -s $(which zsh) 
RUN echo "skip_global_compinit=1" >> /home/$USER/.zshenv
RUN echo "source ~/.profile" >> /home/$USER/.zshrc

RUN echo "source ~/synapse-project/paths.sh 2>/dev/null || true" >> /home/$USER/.zshrc

COPY --chown=$USER:$USER tools/deps/install_package_deps.sh .
RUN chmod +x install_package_deps.sh
RUN ./install_package_deps.sh
RUN rm install_package_deps.sh

# The project, built: the image is a working installation (so `docker build` is the whole
# install check). tools/dev/run_dev_container.sh mounts your checkout over it for development.
COPY --chown=$USER:$USER . synapse-project
WORKDIR synapse-project
RUN tools/deps/build_deps.sh
RUN bash -c 'source paths.sh && make -C dpdk-nfs lib'
RUN bash -c 'source paths.sh && synapse/build-release.sh'

CMD [ "/bin/zsh" ]