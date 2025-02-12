APP:=traffic-generator

MAKEFILE_DIR:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
TOOLS_DIR=$(realpath $(MAKEFILE_DIR)/../tools)

P4_BUILD:=$(TOOLS_DIR)/p4_build.sh
PTF_TEST_SCRIPT:=$(TOOLS_DIR)/run_ptf_test.sh
PTF_TEST_TOFINO2_SCRIPT:=$(TOOLS_DIR)/run_ptf_test_tofino2.sh
P4I:=$(TOOLS_DIR)/run_p4i.sh

P4C:=$(SDE_INSTALL)/bin/bf-p4c
P4_DEBUG_FLAGS:=-g --verbose 2 --create-graphs
DEBUG_BUILD_DIR:=build

install:
	$(P4_BUILD) $(APP).p4

install2:
	$(P4_BUILD) --with-tofino2 $(APP).p4

debug:
	rm -rf $(DEBUG_BUILD_DIR)
	$(P4C) $(P4_DEBUG_FLAGS) -o $(DEBUG_BUILD_DIR) $(APP).p4

debug2:
	rm -rf $(DEBUG_BUILD_DIR)
	$(P4C) $(P4_DEBUG_FLAGS) --target tofino2 --arch t2na -o $(DEBUG_BUILD_DIR) $(APP).p4

switchd:
	@cd $(SDE); ./run_switchd.sh -p $(APP) -c "$(shell find $(SDE) -name "$(APP).conf" | grep "/tofino/" | head -n 1)"

switchd2:
	@cd $(SDE); ./run_switchd.sh -p $(APP) --arch tf2 -c "$(shell find $(SDE) -name "$(APP).conf" | grep "/tofino2/" | head -n 1)"
