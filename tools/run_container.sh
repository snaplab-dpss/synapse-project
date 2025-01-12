#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
ROOT_DIR=$SCRIPT_DIR/..
CONTAINER_NAME="synapse"
PLATFORM=linux/amd64
DISPLAY=host.docker.internal:0

build() {
   docker build \
      --build-arg BUILDPLATFORM=$PLATFORM \
      --platform=$PLATFORM \
      -t $CONTAINER_NAME \
      $ROOT_DIR
}

run() {
   docker run \
      --rm \
      --privileged \
      --network host \
      --platform=$PLATFORM \
      -v $HOME/.ssh:/home/docker/.ssh:ro \
      -v $ROOT_DIR:/home/docker/workspace \
      -e DISPLAY=$DISPLAY \
      -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
      -v ~/.Xauthority:/home/docker/.Xauthority \
      -it \
      $CONTAINER_NAME
}

build
run
