# SyNAPSE

## Tested OSes

- Ubuntu 23.04
- Ubuntu 22.04

## Setup

After cloning the repository, pull all the dependencies (configured as submodules):

```
$ git submodule update --init --recursive
```

## Build

Run the `build.sh` script to build the project from scratch:

```
$ ./build.sh
```

**Warning**: this may take a while and consume a lot of CPU + memory resources.

## Container

Use the docker-compose to build the container.

```
$ docker-compose build
```

This will build an image named `synapse`. Consult the `docker-compose.yml` for more details regarding the image setup. For example, it expects to find both a `.gitconfig` file and a `.ssh` directory on the home directory.


To spawn a container, execute:

```
$ docker-compose run synapse
```