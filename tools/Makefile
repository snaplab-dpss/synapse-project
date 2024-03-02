#########################################################
# SyNAPSE makefile
# Used to compile LibSycon controllers and P4 programs.
# Usage: APP=prog_name make -f PATH_TO_THIS_MAKEFILE
#########################################################

ifndef SDE_INSTALL
$(error "SDE_INSTALL is not set.")
endif

SOURCES = $(wildcard *cpp)
APP = $(basename $(SOURCES))

# Optional compilation variables
P4_COMPILATION_VARS ?=

CURRENT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

BUILD_DIR := build
DEBUG_DIR := $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release

DATAPLANE := $(APP).p4
CONTROLLER := $(APP).cpp

CONTROLLER_BIN := $(RELEASE_DIR)/$(APP)
CONTROLLER_BIN_DEBUG := $(DEBUG_DIR)/$(APP)

P4_BUILD := $(CURRENT_DIR)/p4_build.sh

P4C=$(SDE_INSTALL)/bin/bf-p4c
P4_DEBUG_FLAGS=-g --verbose 2 --create-graphs

CONTROLLER_BUILD_FLAGS := -std=c++17 -DNDEBUG=1 -O3
CONTROLLER_BUILD_DEBUG_FLAGS := -std=c++17 -O0 -g

all: $(CONTROLLER_BIN) $(CONTROLLER_BIN_DEBUG)

debug-tofino1:
	SDE_INSTALL=$(SDE_INSTALL) \
		$(P4C) \
		$(P4_DEBUG_FLAGS) \
		-o $(BUILD_DIR) \
		$(APP).p4 $(P4_COMPILATION_VARS)

debug-tofino2:
	SDE_INSTALL=$(SDE_INSTALL) \
		$(P4C) \
		$(P4_DEBUG_FLAGS) \
		--target tofino2 --arch t2na \
		-o $(BUILD_DIR) \
		$(APP).p4 $(P4_COMPILATION_VARS)

install-tofino1:
	SDE_INSTALL=$(SDE_INSTALL) $(P4_BUILD) $(APP).p4 $(P4_COMPILATION_VARS)

install-tofino2:
	SDE_INSTALL=$(SDE_INSTALL) $(P4_BUILD) --with-tofino2 $(APP).p4 $(P4_COMPILATION_VARS)

$(CONTROLLER_BIN): $(CONTROLLER)
	mkdir -p $(RELEASE_DIR)
	c++ -I$(SDE_INSTALL)/include $(CONTROLLER_BUILD_FLAGS) $(CONTROLLER) -o $(CONTROLLER_BIN) -Wl,-rpath,$(SDE_INSTALL)/lib -lsycon

$(CONTROLLER_BIN_DEBUG): $(CONTROLLER)
	mkdir -p $(DEBUG_DIR)
	c++ -I$(SDE_INSTALL)/include $(CONTROLLER_BUILD_DEBUG_FLAGS) $(CONTROLLER) -o $(CONTROLLER_BIN_DEBUG) -Wl,-rpath,$(SDE_INSTALL)/lib -lsycond

clean:
	rm -rf $(BUILD_DIR) core* bf_drivers.log* zlog-cfg-cur