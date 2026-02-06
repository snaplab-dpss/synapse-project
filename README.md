# Synapse

## System

This project was tested on Ubuntu 20.04, 22.04, and 24.04. If you want to run on other OSes, we recommend you use a docker container (check out the `tools/dev/run_dev_container.sh` script).

## Setup

After cloning the repository, pull all the dependencies (configured as submodules):

<pre>
$ git submodule update --init --recursive
</pre>

### Installation

We now need to build and install all dependencies. We provide a couple of useful tools for this:

- `tools/deps/install_package_deps.sh`: install system package dependencies
- `tools/deps/build_deps.sh`: build project dependencies

You should only need to run each of these scripts *once*. If you are using our container script (`tools/dev/run_dev_container.sh`), notice that it already installs all the requires system package dependencies. Therefore, you need only run the `build_deps.sh` *once* inside the container.

After running `build_deps.sh`, a new file will appear on the project root directory: `paths.sh`. You should always source this file (`source paths.sh`) when working on this project, as it exports the relevant environmental variables.

**Useful tip**: this project makes heavy use of Graphviz to render BDDs, Execution Plans, and Search Spaces as graphs. All these Graphviz files are typically encoded as a `.dot` file. You can render these files [xdot](https://github.com/jrfonseca/xdot.py), which is installed during the setup phase.

### Building Synapse

Building Synapse is very simple:

<pre>
> cd synapse
synapse/> mkdir build && cd build
synapse/build/> cmake .. -GNinja
synapse/build/> ninja
</pre>

Or if you want to build with address sanitizer on for debugging:

<pre>
> cd synapse
synapse/> mkdir build && cd build
synapse/build/> cmake .. -GNinja -DENABLE_ADDRESS_SANITIZER=1 -DCMAKE_BUILD_TYPE=Debug
synapse/build/> ninja
</pre>

Or you can just use the available `synapse/build-release.sh` or `synapse/build-debug.sh` scripts, these will automate the steps shown above.
You can find all created binaries in `synapse/build/bin/`. Run the binaries with `--help` to show the help menu:

<pre>
synapse/build/> ./bin/synapse --help
Synapse
Usage: ./bin/synapse [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --in TEXT REQUIRED          Input file for BDD deserialization.
  --out TEXT [.]              Output directory for every generated file.
  --name TEXT                 Synthesized filenames (without extensions) (defaults to "synapse-{bdd filename}").
  --config TEXT REQUIRED      Configuration file.
  --heuristic ENUM:value in {ds-pref-fcfscachedset->11,ds-pref-fcfscachedtable->10,ds-pref-cuckoo->9,ds-pref-hhtable->8,ds-pref-guardedmaptable->7,max-tput->4,greedy->3,gallium->2,max-controller->12,random->5,dfs->1,ds-pref-simple->6,bfs->0} OR {11,10,9,8,7,4,3,2,12,5,1,6,0} REQUIRED
                              Chosen heuristic.
  --profile TEXT              BDD profile file JSON.
  --seed UINT [2828706629]    Random seed.
  --peek UINT ...             Peek execution plans.
  --force-decisions UINT ...  Force search to use these EPs.
  --no-reorder                Deactivate BDD reordering.
  --show-prof                 Show NF profiling.
  --show-ep                   Show winner Execution Plan.
  --show-ss                   Show the entire search space.
  --show-bdd                  Show the BDD's solution.
  --assert-integrity          Assert integrity of EPs during search.
  --backtrack                 Pause on backtrack.
  --not-greedy                Don't stop on first solution.
  --random-uniform-profile    Use a random uniform profile for the BDD.
  --skip-synthesis            Skip synthesis step (only search).
  --dry-run                   Don't run search.
</pre>

Here is a toy example of a Synapse execution:

<pre>
synapse/build/> ./bin/synapse --in ../../bdds/kvs.bdd --config ../../configs/tofino2-kvs.toml --heuristic max-tput --seed 0 --profile ../../profiles/kvs-f40000-c10000-zipf0_8.json --show-ep --show-ss --show-bdd --out . --name nf
</pre>

This will generate the following files:
- `nf.p4`: The switch data plane implementation portion of the offloaded solution.
- `nf.cpp`: The controller implementation.
- `nf-ep.dot`: The final Execution Plan rendered as a Graphviz graph.
- `nf-ss.dot`: The instantiated search space rendered as a Graphviz graph.
- `nf-bdd.dot`: The final solution's BDD rendered as a Graphviz graph.
- `nf.json`: The Synapse searh report (in JSON format).
- `nf.txt`: The Synapse search report (in human readable format).

## Generating BDDs

Synapse receives NFs encoded as BDDs as input (e.g. `bdds/fw.bdd`). We already pre-generated the BDDs for all NFs currently developed on this repository. This BDDs can be found in the `bdds` folder, and their corresponding DPDK implementations are in `dpdk-nfs`. To automatically regenerate these BDDs, you can use the `tools/generate_bdds.sh` script.

However, if you want to build your own NFs and manually generate BDDs for them, you basically need to (1) develop the NF using our custom DPDK NF development library (passed on by [Vigor](https://github.com/vigor-nf/vigor)), (2) run it by an exhaustive symbolic execution engine, and (3) use Synapse to generate the BDD.

### Manualy running exhaustive symbolic execution (ESE)

To manually run ESE:

1. Navigate into the NF's directory
2. Run `make symbex` to manually generate all of the NF's call paths.

The NF's configuration parameters under ESE are inside the makefile on its directory. The final ESE results can be found inside the NF's directory, on a folder entitled `klee-last`. For each time you run ESE, a new `klee-out-{i}` is generated (with `{i}` depending on the number of times ESE was previously run). `klee-last` simply points to the latest `klee-out-{i}`.

Here is the example output of ESE run on the NOP NF:

<pre>
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
</pre>

## Building a BDD

Use one of Synapse's tools to generate the corresponding BDD:

<pre>
synapse/build/bin/> call-paths-to-bdd --out your-nf.bdd your-nf/klee-last/*.call_path
</pre>

If you want to visualize the generated BDD using Graphviz:

<pre>
synapse/build/bin/> bdd-visualizer --in your-nf.bdd --out your-nf.dot
</pre>

## Generating traces and profiles

Synapse uses profiling information to guide its search towards workload-optimized solutions. To automatically generate pcaps and corresponding NF profiling information across a variety of traffic profiles, we use the `tools/profiler.py` script.

<pre>
profiler.py --help
usage: profiler.py [-h] --nfs {echo,fwd,fw,nat,kvs,psd,cl,pol} [{echo,fwd,fw,nat,kvs,psd,cl,pol} ...] [--total-packets TOTAL_PACKETS [TOTAL_PACKETS ...]] [--rate RATE [RATE ...]] [--packet-size PACKET_SIZE [PACKET_SIZE ...]]
                   [--total-flows TOTAL_FLOWS [TOTAL_FLOWS ...]] [--zipf-params ZIPF_PARAMS [ZIPF_PARAMS ...]] [--churn CHURN [CHURN ...]] [--debug] [--max-concurrent-tasks MAX_CONCURRENT_TASKS] [--skip-pcap-generation] [--skip-profiler-generation]
                   [--show-cmds-output] [--show-cmds] [--show-execution-plan] [--dry-run] [--force] [--force-profile-stats] [--silence]

Profiler helper script. This will generate the pcaps and profiles for all the possible combinations of the provided parameters. Profiler dir: /home/fcp/synapse-project/profiles. Pcap dir: /home/fcp/synapse-project/pcaps.

options:
  -h, --help            show this help message and exit
  --nfs {echo,fwd,fw,nat,kvs,psd,cl,pol} [{echo,fwd,fw,nat,kvs,psd,cl,pol} ...]
                        Target NFs to profile
  --total-packets TOTAL_PACKETS [TOTAL_PACKETS ...]
                        Total packets to send
  --rate RATE [RATE ...]
                        Rate (bps) to send packets at
  --packet-size PACKET_SIZE [PACKET_SIZE ...]
                        Packet size (bytes)
  --total-flows TOTAL_FLOWS [TOTAL_FLOWS ...]
                        Total flows to generate
  --zipf-params ZIPF_PARAMS [ZIPF_PARAMS ...]
                        Zipf parameters
  --churn CHURN [CHURN ...]
                        Churn rate (fpm)
  --debug               Enable debug mode (synapse runs much slower)
  --max-concurrent-tasks MAX_CONCURRENT_TASKS
                        Maximum number of concurrent tasks to run. If <= 0, uses number of CPU cores.
  --skip-pcap-generation
                        Skip pcap generation
  --skip-profiler-generation
                        Skip profiler generation
  --show-cmds-output    Show command output during execution
  --show-cmds           Show requested commands during execution
  --show-execution-plan
                        Show execution plan
  --dry-run
  --force               Force execution **of all tasks** even if files are already produced
  --force-profile-stats
                        Force regeneration of profile stats
  --silence             Silence all output except errors
</pre>

Running the script without arguments will, for each NF and *using all available cores*:
1. Synthesize a DPDK version of the NF that profiles its execution under a provided set of pcaps
2. Generate pcaps and profiling information for all combinations of churn values and skew (churn between 0 and 1M fpm, and skew parameter between 0 and 1.2). This can take a while depending on the NF an the number of cores.
3. Run the NF profiler against each workload.
4. Generate profiling reports, by default written to the `profiles` folder on the project root directory.
