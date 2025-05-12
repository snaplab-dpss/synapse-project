#!/bin/bash

########################################################################################################################################
#   Note: This is all based on our specific testbed configuration.
#   You may want to adjust the port configuration on each NF.
#   For example, our NFs will run on a Tofino programmable switch with 32 ports (1 to 32). However, we only have ports 3-32 connected.
#   That is why we generate pcaps only for devices 2-31 (devices start by 0).
########################################################################################################################################

set -euo pipefail

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
PROJECT_DIR=$(realpath $SCRIPT_DIR/..)

TOOLS_DIR=$PROJECT_DIR/tools
SYNTHESIZED_DIR=$PROJECT_DIR/synthesized
PCAPS_DIR=$PROJECT_DIR/pcaps
PROFILES_DIR=$PROJECT_DIR/profiles
BDDS_DIR=$PROJECT_DIR/bdds
SYNAPSE_DIR=$PROJECT_DIR/synapse

CHURN="0 1000 10000 100000 1000000"
TRAFFIC="0 0.2 0.4 0.6 0.8 1.0"

# Everything will be done inside the synthesized directory
cd $SYNTHESIZED_DIR

export TOOLS_DIR
export SYNTHESIZED_DIR
export PCAPS_DIR
export PROFILES_DIR
export BDDS_DIR
export SYNAPSE_DIR

log_and_run() {
    echo
    echo "[*] Running cmd: $@"
    echo
    eval $@
}

export -f log_and_run

gen_and_build_profiler() {
    nf=$1
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-synthesizer \
        --target profiler \
        --in $BDDS_DIR/$nf.bdd \
        --out $SYNTHESIZED_DIR/$nf-profiler.cpp

    log_and_run NF=$nf-profiler.cpp make -f $TOOLS_DIR/Makefile.dpdk
}

export -f gen_and_build_profiler

profile_echo() {
    flows=$1
    churn=$2
    zipf_param=$3

    distribution="unif"
    if [ "$zipf_param" != "0" ]; then
        distribution="zipf"
    fi

    pcap="echo-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        s=$(python -c "print(str(int($zipf_param) if int($zipf_param) == $zipf_param else $zipf_param).replace('.','_'))")
        pcap="$pcap${s}"
    fi

    echo "Generating profile for $pcap"

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/echo-profiler \
        $PROFILES_DIR/$report.json \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/echo.bdd \
        --profile $PROFILES_DIR/$report.json \
        --out $PROFILES_DIR/$report.dot
}

export -f profile_echo

profile_fwd() {
    flows=$1
    churn=$2
    zipf_param=$3

    distribution="unif"
    if [ "$zipf_param" != "0" ]; then
        distribution="zipf"
    fi

    pcap="fwd-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        s=$(python -c "print(str(int($zipf_param) if int($zipf_param) == $zipf_param else $zipf_param).replace('.','_'))")
        pcap="$pcap${s}"
    fi

    echo "Generating profile for $pcap"

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/fwd-profiler \
        $PROFILES_DIR/$report.json \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/fwd.bdd \
        --profile $PROFILES_DIR/$report.json \
        --out $PROFILES_DIR/$report.dot
}

export -f profile_fwd

profile_fw() {
    flows=$1
    churn=$2
    zipf_param=$3

    distribution="unif"
    if [ "$zipf_param" != "0" ]; then
        distribution="zipf"
    fi

    pcap="fw-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        s=$(python -c "print(str(int($zipf_param) if int($zipf_param) == $zipf_param else $zipf_param).replace('.','_'))")
        pcap="$pcap${s}"
    fi

    echo "Generating profile for $pcap"

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/fw-profiler \
        $PROFILES_DIR/$report.json \
        --warmup 2:$PCAPS_DIR/$pcap-dev2-warmup.pcap \
        --warmup 4:$PCAPS_DIR/$pcap-dev4-warmup.pcap \
        --warmup 6:$PCAPS_DIR/$pcap-dev6-warmup.pcap \
        --warmup 8:$PCAPS_DIR/$pcap-dev8-warmup.pcap \
        --warmup 10:$PCAPS_DIR/$pcap-dev10-warmup.pcap \
        --warmup 12:$PCAPS_DIR/$pcap-dev12-warmup.pcap \
        --warmup 14:$PCAPS_DIR/$pcap-dev14-warmup.pcap \
        --warmup 16:$PCAPS_DIR/$pcap-dev16-warmup.pcap \
        --warmup 18:$PCAPS_DIR/$pcap-dev18-warmup.pcap \
        --warmup 20:$PCAPS_DIR/$pcap-dev20-warmup.pcap \
        --warmup 22:$PCAPS_DIR/$pcap-dev22-warmup.pcap \
        --warmup 24:$PCAPS_DIR/$pcap-dev24-warmup.pcap \
        --warmup 26:$PCAPS_DIR/$pcap-dev26-warmup.pcap \
        --warmup 28:$PCAPS_DIR/$pcap-dev28-warmup.pcap \
        --warmup 30:$PCAPS_DIR/$pcap-dev30-warmup.pcap \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/fw.bdd \
        --profile $PROFILES_DIR/$report.json \
        --out $PROFILES_DIR/$report.dot
}

export -f profile_fw

profile_nat() {
    flows=$1
    churn=$2
    zipf_param=$3

    distribution="unif"
    if [ "$zipf_param" != "0" ]; then
        distribution="zipf"
    fi

    pcap="nat-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        s=$(python -c "print(str(int($zipf_param) if int($zipf_param) == $zipf_param else $zipf_param).replace('.','_'))")
        pcap="$pcap${s}"
    fi

    echo "Generating profile for $pcap"

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/nat-profiler \
        $PROFILES_DIR/$report.json \
        --warmup 2:$PCAPS_DIR/$pcap-dev2-warmup.pcap \
        --warmup 4:$PCAPS_DIR/$pcap-dev4-warmup.pcap \
        --warmup 6:$PCAPS_DIR/$pcap-dev6-warmup.pcap \
        --warmup 8:$PCAPS_DIR/$pcap-dev8-warmup.pcap \
        --warmup 10:$PCAPS_DIR/$pcap-dev10-warmup.pcap \
        --warmup 12:$PCAPS_DIR/$pcap-dev12-warmup.pcap \
        --warmup 14:$PCAPS_DIR/$pcap-dev14-warmup.pcap \
        --warmup 16:$PCAPS_DIR/$pcap-dev16-warmup.pcap \
        --warmup 18:$PCAPS_DIR/$pcap-dev18-warmup.pcap \
        --warmup 20:$PCAPS_DIR/$pcap-dev20-warmup.pcap \
        --warmup 22:$PCAPS_DIR/$pcap-dev22-warmup.pcap \
        --warmup 24:$PCAPS_DIR/$pcap-dev24-warmup.pcap \
        --warmup 26:$PCAPS_DIR/$pcap-dev26-warmup.pcap \
        --warmup 28:$PCAPS_DIR/$pcap-dev28-warmup.pcap \
        --warmup 30:$PCAPS_DIR/$pcap-dev30-warmup.pcap \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/nat.bdd \
        --profile $PROFILES_DIR/$report.json \
        --out $PROFILES_DIR/$report.dot
}

export -f profile_nat

profile_kvs() {
    flows=$1
    churn=$2
    zipf_param=$3

    distribution="unif"
    if [ "$zipf_param" != "0" ]; then
        distribution="zipf"
    fi

    pcap="kvs-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        s=$(python -c "print(str(int($zipf_param) if int($zipf_param) == $zipf_param else $zipf_param).replace('.','_'))")
        pcap="$pcap${s}"
    fi

    echo "Generating profile for $pcap"

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/kvs-profiler \
        $PROFILES_DIR/$report.json \
        --warmup 2:$PCAPS_DIR/$pcap-dev2-warmup.pcap \
        --warmup 3:$PCAPS_DIR/$pcap-dev3-warmup.pcap \
        --warmup 4:$PCAPS_DIR/$pcap-dev4-warmup.pcap \
        --warmup 5:$PCAPS_DIR/$pcap-dev5-warmup.pcap \
        --warmup 6:$PCAPS_DIR/$pcap-dev6-warmup.pcap \
        --warmup 7:$PCAPS_DIR/$pcap-dev7-warmup.pcap \
        --warmup 8:$PCAPS_DIR/$pcap-dev8-warmup.pcap \
        --warmup 9:$PCAPS_DIR/$pcap-dev9-warmup.pcap \
        --warmup 10:$PCAPS_DIR/$pcap-dev10-warmup.pcap \
        --warmup 11:$PCAPS_DIR/$pcap-dev11-warmup.pcap \
        --warmup 12:$PCAPS_DIR/$pcap-dev12-warmup.pcap \
        --warmup 13:$PCAPS_DIR/$pcap-dev13-warmup.pcap \
        --warmup 14:$PCAPS_DIR/$pcap-dev14-warmup.pcap \
        --warmup 15:$PCAPS_DIR/$pcap-dev15-warmup.pcap \
        --warmup 16:$PCAPS_DIR/$pcap-dev16-warmup.pcap \
        --warmup 17:$PCAPS_DIR/$pcap-dev17-warmup.pcap \
        --warmup 18:$PCAPS_DIR/$pcap-dev18-warmup.pcap \
        --warmup 19:$PCAPS_DIR/$pcap-dev19-warmup.pcap \
        --warmup 20:$PCAPS_DIR/$pcap-dev20-warmup.pcap \
        --warmup 21:$PCAPS_DIR/$pcap-dev21-warmup.pcap \
        --warmup 22:$PCAPS_DIR/$pcap-dev22-warmup.pcap \
        --warmup 23:$PCAPS_DIR/$pcap-dev23-warmup.pcap \
        --warmup 24:$PCAPS_DIR/$pcap-dev24-warmup.pcap \
        --warmup 25:$PCAPS_DIR/$pcap-dev25-warmup.pcap \
        --warmup 26:$PCAPS_DIR/$pcap-dev26-warmup.pcap \
        --warmup 27:$PCAPS_DIR/$pcap-dev27-warmup.pcap \
        --warmup 28:$PCAPS_DIR/$pcap-dev28-warmup.pcap \
        --warmup 29:$PCAPS_DIR/$pcap-dev29-warmup.pcap \
        --warmup 30:$PCAPS_DIR/$pcap-dev30-warmup.pcap \
        --warmup 31:$PCAPS_DIR/$pcap-dev31-warmup.pcap \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/kvs.bdd \
        --profile $PROFILES_DIR/$report.json \
        --out $PROFILES_DIR/$report.dot
}

export -f profile_kvs

profile_cl() {
    flows=$1
    churn=$2
    zipf_param=$3

    distribution="unif"
    if [ "$zipf_param" != "0" ]; then
        distribution="zipf"
    fi

    pcap="cl-f$flows-c$churn-$distribution"
    if [ "$distribution" == "zipf" ]; then
        s=$(python -c "print(str(int($zipf_param) if int($zipf_param) == $zipf_param else $zipf_param).replace('.','_'))")
        pcap="$pcap${s}"
    fi

    echo "Generating profile for $pcap"

    report=$pcap

    log_and_run $SYNTHESIZED_DIR/build/cl-profiler \
        $PROFILES_DIR/$report.json \
        --warmup 2:$PCAPS_DIR/$pcap-dev2-warmup.pcap \
        --warmup 4:$PCAPS_DIR/$pcap-dev4-warmup.pcap \
        --warmup 6:$PCAPS_DIR/$pcap-dev6-warmup.pcap \
        --warmup 8:$PCAPS_DIR/$pcap-dev8-warmup.pcap \
        --warmup 10:$PCAPS_DIR/$pcap-dev10-warmup.pcap \
        --warmup 12:$PCAPS_DIR/$pcap-dev12-warmup.pcap \
        --warmup 14:$PCAPS_DIR/$pcap-dev14-warmup.pcap \
        --warmup 16:$PCAPS_DIR/$pcap-dev16-warmup.pcap \
        --warmup 18:$PCAPS_DIR/$pcap-dev18-warmup.pcap \
        --warmup 20:$PCAPS_DIR/$pcap-dev20-warmup.pcap \
        --warmup 22:$PCAPS_DIR/$pcap-dev22-warmup.pcap \
        --warmup 24:$PCAPS_DIR/$pcap-dev24-warmup.pcap \
        --warmup 26:$PCAPS_DIR/$pcap-dev26-warmup.pcap \
        --warmup 28:$PCAPS_DIR/$pcap-dev28-warmup.pcap \
        --warmup 30:$PCAPS_DIR/$pcap-dev30-warmup.pcap \
        2:$PCAPS_DIR/$pcap-dev2.pcap \
        3:$PCAPS_DIR/$pcap-dev3.pcap \
        4:$PCAPS_DIR/$pcap-dev4.pcap \
        5:$PCAPS_DIR/$pcap-dev5.pcap \
        6:$PCAPS_DIR/$pcap-dev6.pcap \
        7:$PCAPS_DIR/$pcap-dev7.pcap \
        8:$PCAPS_DIR/$pcap-dev8.pcap \
        9:$PCAPS_DIR/$pcap-dev9.pcap \
        10:$PCAPS_DIR/$pcap-dev10.pcap \
        11:$PCAPS_DIR/$pcap-dev11.pcap \
        12:$PCAPS_DIR/$pcap-dev12.pcap \
        13:$PCAPS_DIR/$pcap-dev13.pcap \
        14:$PCAPS_DIR/$pcap-dev14.pcap \
        15:$PCAPS_DIR/$pcap-dev15.pcap \
        16:$PCAPS_DIR/$pcap-dev16.pcap \
        17:$PCAPS_DIR/$pcap-dev17.pcap \
        18:$PCAPS_DIR/$pcap-dev18.pcap \
        19:$PCAPS_DIR/$pcap-dev19.pcap \
        20:$PCAPS_DIR/$pcap-dev20.pcap \
        21:$PCAPS_DIR/$pcap-dev21.pcap \
        22:$PCAPS_DIR/$pcap-dev22.pcap \
        23:$PCAPS_DIR/$pcap-dev23.pcap \
        24:$PCAPS_DIR/$pcap-dev24.pcap \
        25:$PCAPS_DIR/$pcap-dev25.pcap \
        26:$PCAPS_DIR/$pcap-dev26.pcap \
        27:$PCAPS_DIR/$pcap-dev27.pcap \
        28:$PCAPS_DIR/$pcap-dev28.pcap \
        29:$PCAPS_DIR/$pcap-dev29.pcap \
        30:$PCAPS_DIR/$pcap-dev30.pcap \
        31:$PCAPS_DIR/$pcap-dev31.pcap
    
    log_and_run $SYNAPSE_DIR/build/bin/bdd-visualizer \
        --in $BDDS_DIR/cl.bdd \
        --profile $PROFILES_DIR/$report.json \
        --out $PROFILES_DIR/$report.dot
}

export -f profile_cl

generate_profiles_echo() {
    flows=40000

    parallel \
        -j $(nproc) --verbose \
        profile_echo $flows \
        ::: $CHURN \
        ::: $TRAFFIC
}

generate_profiles_fwd() {
    flows=40000
    
    parallel \
        -j $(nproc) --verbose \
        profile_fwd $flows \
        ::: $CHURN \
        ::: $TRAFFIC
}

generate_profiles_fw() {
    flows=40000

    parallel \
        -j $(nproc) --verbose \
        profile_fw $flows \
        ::: $CHURN \
        ::: $TRAFFIC
}

generate_profiles_nat() {
    flows=40000

    parallel \
        -j $(nproc) --verbose \
        profile_nat $flows \
        ::: $CHURN \
        ::: $TRAFFIC
}

generate_profiles_kvs() {
    flows=100000

    parallel \
        -j $(nproc) --verbose \
        profile_kvs $flows \
        ::: $CHURN \
        ::: $TRAFFIC
}

generate_profiles_cl() {
    flows=40000

    parallel \
        -j $(nproc) --verbose \
        profile_cl $flows \
        ::: $CHURN \
        ::: $TRAFFIC
}

main() {
    parallel \
        -j $(nproc) --verbose \
        gen_and_build_profiler \
        ::: echo fwd fw nat kvs cl

    # generate_profiles_echo
    # generate_profiles_fwd
    # generate_profiles_fw
    # generate_profiles_nat
    generate_profiles_kvs
    # generate_profiles_cl
}

main