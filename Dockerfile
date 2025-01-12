FROM --platform=${BUILDPLATFORM} ubuntu:24.10

# https://stackoverflow.com/questions/51023312/docker-having-issues-installing-apt-utils
ARG DEBIAN_FRONTEND=noninteractive

ENV TZ=Europe/Lisbon
ENV GCC_VERSION=11
ENV PERL5LIB=''
ENV MANPATH=''
ENV PROMPT_COMMAND=''

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# The install scripts require sudo (no need to clean apt cache, the setup script will install stuff)
RUN apt-get update && apt-get install -y sudo adduser

# Create a user with passwordless sudo
RUN adduser --disabled-password --gecos '' docker
RUN adduser docker sudo
RUN echo '%docker ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER docker
WORKDIR /home/docker

# COPY setup_system.sh .
# RUN ./setup_system.sh

RUN sudo apt-get update -qq && sudo apt-get install -yqq \
    man build-essential wget curl git vim tzdata tmux zsh \
    iputils-ping iproute2 net-tools tcpreplay iperf \
    psmisc htop gdb xdg-utils \
    python3-pip python3-venv python3-scapy python-is-python3 xdot \
    gperf libgoogle-perftools-dev libpcap-dev meson pkg-config \
    bison flex zlib1g-dev libncurses5-dev libpcap-dev \
    opam m4 libgmp-dev \
    gcc-$GCC_VERSION g++-$GCC_VERSION cmake \
    linux-headers-generic

RUN sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 100
RUN sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION 100

RUN sudo update-alternatives --set g++ /usr/bin/g++-$GCC_VERSION
RUN sudo update-alternatives --set gcc /usr/bin/gcc-$GCC_VERSION

RUN sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 100
RUN sudo update-alternatives --set cc /usr/bin/gcc

RUN sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 100
RUN sudo update-alternatives --set c++ /usr/bin/g++

RUN sudo dpkg-reconfigure --frontend noninteractive tzdata
RUN mkdir /home/docker/.ssh
RUN chown -R docker:docker /home/docker/.ssh

RUN curl -fsSL https://raw.githubusercontent.com/zimfw/install/master/install.zsh | zsh
RUN echo "set -g default-terminal \"screen-256color\"" >> /home/docker/.tmux.conf
RUN echo "set-option -g default-shell /bin/zsh" >> /home/docker/.tmux.conf
RUN sudo chsh -s $(which zsh) 
RUN echo "source ~/.profile" >> /home/docker/.zshrc

RUN python3 -m venv env
RUN echo "source ~/env/bin/activate" >> /home/docker/.zshrc
RUN . ~/env/bin/activate && pip install setuptools wheel

###########################
#     Additional tools    #
###########################

# If you want to install additional packages or augment the container in any other way,
# do it here so that you don't have to rebuild everything from scratch.

WORKDIR workspace

CMD [ "/bin/zsh" ]