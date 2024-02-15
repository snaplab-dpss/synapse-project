#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"

PATHSFILE=$SCRIPT_DIR/paths.sh
PYTHON_ENV_DIR=$SCRIPT_DIR/env

SAMPLE_CONFIG_FILE="$SCRIPT_DIR/sample_experiment_config.toml"
DEFAULT_CONFIG_FILE="$SCRIPT_DIR/experiment_config.toml"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

cd $SCRIPT_DIR

# Make sure all submodules are cloned.
git submodule update --init --recursive

get_deps() {
  sudo apt update
  sudo apt-get -y install \
    python3-pyelftools \
    python3-pip \
    python3-venv
}

add_expr_to_paths_file() {
  if ! grep "^${1}" "$PATHSFILE" >/dev/null; then
    echo "${1}" >> "$PATHSFILE"
    . "$PATHSFILE"
  fi
}

create_paths_file() {
  rm -f $PATHSFILE > /dev/null 2>&1 || true
  touch $PATHSFILE
}

setup_env() {
  pushd "$SCRIPT_DIR"
    if [ ! -d $PYTHON_ENV_DIR ]; then
      python3 -m venv $PYTHON_ENV_DIR
    fi

    add_expr_to_paths_file ". $PYTHON_ENV_DIR/bin/activate"
    . $PATHSFILE
    pip3 install -r requirements.txt
  popd

  echo
    printf "Source ${GREEN}$PYTHON_ENV_DIR${NC} to enter the python virtual env."
}

init_config() {
    # Initialize config.
    if [ ! -f "$DEFAULT_CONFIG_FILE" ]; then
        echo "Initializing config file with default values."
        cp $SAMPLE_CONFIG_FILE $DEFAULT_CONFIG_FILE
    fi

    echo
    printf "Edit ${GREEN}$DEFAULT_CONFIG_FILE${NC} to reflect your environment or \
    specify a different config file when running experiment.py.\n"
}

get_deps
create_paths_file
setup_env
init_config