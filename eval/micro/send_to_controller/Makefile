APP:=send_to_controller

TOOLS_DIR=~/tools
BUILD_DIR:=build

DATAPLANE:=$(APP).p4
CONTROLLER:=$(APP).cpp
CONTROLLER_BIN:=$(BUILD_DIR)/$(APP)

P4_BUILD:=$(TOOLS_DIR)/p4_build.sh
CONTROLLER_BUILD_FLAGS:=-std=c++17 -DNDEBUG=1 -O3

all: $(CONTROLLER_BIN)

install:
	$(P4_BUILD) --with-tofino2 $(P4_SOURCE_DIR)/$(APP).p4

$(CONTROLLER_BIN): $(CONTROLLER)
	mkdir -p $(BUILD_DIR)
	c++ -I$(SDE_INSTALL)/include $(CONTROLLER_BUILD_FLAGS) $(CONTROLLER) -o $(CONTROLLER_BIN) -Wl,-rpath,$(SDE_INSTALL)/lib -lsycon
