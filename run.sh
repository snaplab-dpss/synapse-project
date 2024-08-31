#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
CONTAINER_NAME="synapse"

build() {
   pushd $SCRIPT_DIR >/dev/null
      docker build . -t $CONTAINER_NAME
   popd >/dev/null
}

run() {
   # docker run \
   #    --rm \
   #    --privileged \
   #    --network host \
   #    -v $HOME/.gitconfig:/home/docker/.gitconfig:ro \
   #    -v $HOME/.ssh:/home/docker/.ssh:ro \
   #    -v $SCRIPT_DIR:/home/docker/workspace \
   #    -e DISPLAY=$DISPLAY \
   #    -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
   #    -v ~/.Xauthority:/home/docker/.Xauthority \
   #    -it \
   #    $CONTAINER_NAME

   docker run \
      --rm \
      --privileged \
      --network host \
      -e DISPLAY=$DISPLAY \
      -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
      -v ~/.Xauthority:/home/docker/.Xauthority \
      -it \
      $CONTAINER_NAME
}

build
run
