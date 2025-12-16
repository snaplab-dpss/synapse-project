#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
ROOT_DIR=$SCRIPT_DIR/../..
CONTAINER_NAME="synapse"
PLATFORM=linux/amd64

OS_NAME=$(uname -s)

if [ $OS_NAME == "Darwin" ]; then
    # On MacOS, we need to ensure that XQuartz is running and that
    # "Allow connections from network clients" is enabled in its preferences.
    if ! pgrep XQuartz > /dev/null; then
        echo "Starting XQuartz..."
        open -a XQuartz
        sleep 2
    fi
    # Allow connections to X server from localhost
    xhost +localhost || true
    DISPLAY=host.docker.internal:0
else
    if [ -z "${DISPLAY:-}" ]; then
        echo "DISPLAY variable is not set. Please set it to run the container with GUI support."
        exit 1
    fi
    DISPLAY=${DISPLAY:-}
fi

cd $ROOT_DIR

docker build \
   --build-arg BUILDPLATFORM=$PLATFORM \
   --platform=$PLATFORM \
   -f $SCRIPT_DIR/Dockerfile \
   -t $CONTAINER_NAME \
   $ROOT_DIR

# Create a .gitconfig file if it doesn't exist to avoid docker errors
if [ ! -f $HOME/.gitconfig ]; then
    touch $HOME/.gitconfig
fi

# Same for .ssh directory
if [ ! -d $HOME/.ssh ]; then
    mkdir -p $HOME/.ssh
    chmod 700 $HOME/.ssh
fi

docker run \
   --rm \
   --privileged \
   --network host \
   --platform=$PLATFORM \
   -v $HOME/.ssh:/home/ubuntu/.ssh:ro \
   -v $HOME/.gitconfig:/home/ubuntu/.gitconfig:ro \
   -v $ROOT_DIR:/home/ubuntu/synapse-project \
   -e DISPLAY=$DISPLAY \
   -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
   -v ~/.Xauthority:/home/ubuntu/.Xauthority \
   -it \
   $CONTAINER_NAME
