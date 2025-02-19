# SyNAPSE

## System

This project was tested on Ubuntu 20.04, 22.04, and 24.04. If you want to run on other OSs, we recommend you use a docker container (check out the `tools/run_container.sh` script).

## Setup

After cloning the repository, pull all the dependencies (configured as submodules):

```
$ git submodule update --init --recursive
```

## Installation

The installation

We now need to build and install all dependencies. We provide a couple of useful tools for this:

- `tools/install_package_deps.sh`: install system package dependencies
- `tools/build_deps.sh`: build project dependencies

You should only need to run each of these scripts *once*. If you are using our container script (`tools/run_container.sh`), notice that it already install all the requires system package dependencies. Therefore, you need only run the `build_deps.sh` *once* inside the container.

After running `build_deps.sh`, a new file will appear on the project root directory: `paths.sh`. You should always source this file (`source paths.sh`) when working on this project, as it exports the relevant environmental variables.

## Building synapse

Building synapse is very simple:

```
> cd synapse
synapse/> ./build.sh
```

This will build synapse and create all the binaries. You can find all created binaries in `synapse/build/bin/`. Run the binaries with `--help` print the help menu.

## Running exhaustive symbolic execution (ESE)

To manually run ESE:

1. Navigate into the NF's directory
2. Run `make symbex` to manually generate all of the NF's call paths.

The NF's configuration parameters under ESE are inside the makefile on its directory. The final ESE results can be found inside the NF's directory, on a folder entitled `klee-out-{i}` (with `{i}` depending on the number of times ESE was previously run).

Here is the example output of ESE run on the NOP NF:

```
$ cd dpdk-nfs/fwd
$ make symbex
KLEE: output directory is "~/maestro/dpdk-nfs/fwd/klee-out-0"
KLEE: Using Z3 solver backend
KLEE: Deterministic memory allocation starting from 0x40000000
KLEE: WARNING: undefined reference to function: kill (UNSAFE)!
KLEE: WARNING ONCE: Alignment of memory from call "malloc" is not modelled. Using alignment of 8.
KLEE: Deterministic memory allocation starting from 0x40000000

KLEE: done: total instructions = 220410
KLEE: done: completed paths = 5
KLEE: done: generated tests = 5
        Command being timed: "klee -no-externals -allocate-determ -allocate-determ-start-address=0x00040000000 -allocate-determ-size=1000 -dump-call-traces -dump-call-trace-prefixes -solver-backend=z3 -exit-on-error -max-memory=750000 -search=dfs -condone-undeclared-havocs --debug-report-symbdex nf.bc --lcores=0 --no-shconf --no-telemetry -- --lan 0 --wan 1"
        User time (seconds): 1.10
        System time (seconds): 0.03
        Percent of CPU this job got: 96%
        Elapsed (wall clock) time (h:mm:ss or m:ss): 0:01.17
        Average shared text size (kbytes): 0
        Average unshared data size (kbytes): 0
        Average stack size (kbytes): 0
        Average total size (kbytes): 0
        Maximum resident set size (kbytes): 39260
        Average resident set size (kbytes): 0
        Major (requiring I/O) page faults: 159
        Minor (reclaiming a frame) page faults: 5813
        Voluntary context switches: 581
        Involuntary context switches: 2
        Swaps: 0
        File system inputs: 54640
        File system outputs: 5384
        Socket messages sent: 0
        Socket messages received: 0
        Signals delivered: 0
        Page size (bytes): 4096
        Exit status: 0
```

## Building a BDD

To generate a BDD, similarly to running ESE on an NF, simply run `make bdd` inside the NF directory. This will two files:
- `nf.bdd`: the serialized BDD file
- `nf.dot`: the `dot` representation of the BDD, which you can render with, for example, [xdot](https://github.com/jrfonseca/xdot.py) (which is installed during the setup phase).

You can also run the `tools/generate_bdds.sh` script to generate BDDs for all NFs. This will dump all BDDs and corresponding dot files inside the folder `bdds`. Notice that this directory is already populated with BDD files.

## Using a BDD

After generating a BDD, you can use it to your heart's content. For example, you can generate the dot file for the BDD:

```
> synapse/build/bin/bdd-visualizer --in bdds/fwd.bdd --out /tmp/my_fwd.dot
```