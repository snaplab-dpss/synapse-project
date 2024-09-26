APP:=pol

P4_BUILD:=~/tools/p4_build.sh
PTF_TEST_SCRIPT:=~/tools/run_ptf_test.sh
PTF_TEST_TOFINO2_SCRIPT:=~/tools/run_ptf_test_tofino2.sh
P4I:=$(SDE_INSTALL)/bin/p4i
P4C:=$(SDE_INSTALL)/bin/bf-p4c
P4_DEBUG_FLAGS:=-g --verbose 2 --create-graphs
DEBUG_BUILD_DIR:=build

P4_PTF_TESTS_DIR:=ptf_tests
CONTROLLER_SOURCE_DIR:=controller

TESTS:=$(wildcard $(P4_PTF_TESTS_DIR)/*.py)
CONTROLLER:=$(CONTROLLER_SOURCE_DIR)/controller.py
TOPOLOGY:=$(CONTROLLER_SOURCE_DIR)/topology.json

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

p4i:
	@$(P4I) --no-browser -w . --loglevel info

switchd:
	@cd $(SDE); ./run_switchd.sh -p $(APP) -c "$(shell find $(SDE) -name "$(APP).conf" | grep "/tofino/" | head -n 1)"

switchd2:
	@cd $(SDE); ./run_switchd.sh -p $(APP) --arch tf2 -c "$(shell find $(SDE) -name "$(APP).conf" | grep "/tofino2/" | head -n 1)"

test: $(P4_PTF_TESTS_DIR)/*.py
	$(PTF_TEST_SCRIPT) $(APP).p4 $^

test2: $(P4_PTF_TESTS_DIR)/*.py
	$(PTF_TEST_TOFINO2_SCRIPT) $(APP).p4 $^

run:
	@$(CONTROLLER)