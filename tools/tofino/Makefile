#########################################################
# SyNAPSE makefile
# Used to compile libsycon controllers and P4 programs.
# Usage: APP=prog_name make -f PATH_TO_THIS_MAKEFILE
#########################################################

ifndef SDE_INSTALL
$(error "SDE_INSTALL is not set.")
endif

ifndef APP
SOURCES := $(wildcard *cpp)
APP := $(basename $(SOURCES))
endif

# Kind of a hack, but oh well...
P4_COMPILATION_VARS ?= INVALID_P4_COMPILATION_VARS
ifeq ($(P4_COMPILATION_VARS),INVALID_P4_COMPILATION_VARS)
$(error "P4_COMPILATION_VARS is not set.")
endif

CONTROLLER_ARGS ?= INVALID_CONTROLLER_ARGS
ifeq ($(CONTROLLER_ARGS),INVALID_CONTROLLER_ARGS)
$(error "CONTROLLER_ARGS is not set.")
endif

CURRENT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

BUILD_DIR   := build
DEBUG_DIR   := $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release

DATAPLANE  := $(APP).p4
CONTROLLER := $(APP).cpp

CONTROLLER_BIN       := $(RELEASE_DIR)/$(APP)
CONTROLLER_BIN_DEBUG := $(DEBUG_DIR)/$(APP)

P4_BUILD        := $(CURRENT_DIR)/p4_build.sh
P4C             := $(SDE_INSTALL)/bin/bf-p4c
P4C_DEBUG_FLAGS := -g --verbose 2 --create-graphs

P4_ASSEMBLY_TNA1  := $(SDE)/build/p4-build/tofino/$(APP)/$(APP)/tofino/pipe/$(APP).bfa
P4_ASSEMBLY_TNA2  := $(SDE)/build/p4-build/tofino/$(APP)/$(APP)/tofino2/pipe/$(APP).bfa
P4_ASSEMBLY_DEBUG := $(BUILD_DIR)/pipe/$(APP).bfa

P4_TNA1_BUILD_FLAGS := --target tofino --arch tna
P4_TNA2_BUILD_FLAGS := --target tofino2 --arch t2na

CONTROLLER_COMMON_FLAGS        := -std=c++17 -Wall
CONTROLLER_BUILD_RELEASE_FLAGS := $(CONTROLLER_COMMON_FLAGS) -DNDEBUG=1 -O3
CONTROLLER_BUILD_DEBUG_FLAGS   := $(CONTROLLER_COMMON_FLAGS) -O0 -g
CONTROLLER_LIBS                := -Wl,-rpath,$(SDE_INSTALL)/lib -lsycon
CONTROLLER_LIBS_DEBUG          := -Wl,-rpath,$(SDE_INSTALL)/lib -lsycond
CONTROLLER_INCLUDES            := -I$(SDE_INSTALL)/include

PACKET_SENDER := $(CURRENT_DIR)/send_packets.py

ENV_VARS := SDE_INSTALL=$(SDE_INSTALL)

.PHONY: all
all: $(CONTROLLER_BIN) $(CONTROLLER_BIN_DEBUG)

debug: $(APP).p4
	$(ENV_VARS) $(P4C) $(P4C_DEBUG_FLAGS) $(P4_TNA1_BUILD_FLAGS) -o $(BUILD_DIR) $(APP).p4 $(P4_COMPILATION_VARS)

debug-tofino2: $(APP).p4
	$(ENV_VARS) $(P4C) $(P4C_DEBUG_FLAGS) $(P4_TNA2_BUILD_FLAGS) -o $(BUILD_DIR) $(APP).p4 $(P4_COMPILATION_VARS)

install: $(P4_ASSEMBLY_TNA1)
$(P4_ASSEMBLY_TNA1): $(APP).p4
	$(ENV_VARS) $(P4_BUILD) $(APP).p4 $(P4_COMPILATION_VARS)

install-tofino2: $(P4_ASSEMBLY_TNA2)
$(P4_ASSEMBLY_TNA2): $(APP).p4
	$(ENV_VARS) $(P4_BUILD) --with-tofino2 $(APP).p4 $(P4_COMPILATION_VARS)

controller-debug: $(CONTROLLER_BIN_DEBUG)
$(CONTROLLER_BIN_DEBUG): $(CONTROLLER)
	@mkdir -p $(DEBUG_DIR)
	$(CXX) \
		$(CONTROLLER_INCLUDES) \
		$(CONTROLLER_BUILD_DEBUG_FLAGS) \
		$(CONTROLLER) \
		-o $(CONTROLLER_BIN_DEBUG) \
		$(CONTROLLER_LIBS_DEBUG)

controller: $(CONTROLLER_BIN)
$(CONTROLLER_BIN): $(CONTROLLER)
	@mkdir -p $(RELEASE_DIR)
	$(CXX) \
		$(CONTROLLER_INCLUDES) \
		$(CONTROLLER_BUILD_RELEASE_FLAGS) \
		$(CONTROLLER) \
		-o $(CONTROLLER_BIN) \
		$(CONTROLLER_LIBS)

run-controller-model: $(CONTROLLER_BIN_DEBUG)
	sudo -E $(CONTROLLER_BIN_DEBUG) --model --tna 1 $(CONTROLLER_ARGS)

run-model: install
	cd $(SDE) && ./run_tofino_model.sh -p $(APP)

run-packet-sender: install
	sudo $(PACKET_SENDER)

run-dev: install controller-debug
	@tmux new-session -d -s synapse-p4-dev
	@tmux split-window -v -d -p 33
	@tmux split-window -h -d -p 33

	@tmux send -t %0 'make run-controller-model' ENTER
	@tmux send -t %1 'make run-model' ENTER
	@tmux send -t %2 'make run-packet-sender' ENTER

	@tmux select-pane -t %2
	@tmux a

clean:
	rm -rf $(BUILD_DIR) core* bf_drivers.log* zlog-cfg-cur log