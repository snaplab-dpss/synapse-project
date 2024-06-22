APP:=perfmodel

P4_BUILD:=../../../tools/p4_build.sh

install:
	$(P4_BUILD) $(APP).p4

install-tofino2:
	$(P4_BUILD) --with-tofino2 $(APP).p4

run-switchd:
	@cd $(SDE); ./run_switchd.sh -p $(APP) -c "$(shell find $(SDE) -name "$(APP).conf" | grep "/tofino/" | head -n 1)"

run-switchd-tofino2:
	@cd $(SDE); ./run_switchd.sh -p $(APP) -c "$(shell find $(SDE) -name "$(APP).conf" | grep "/tofino2/" | head -n 1)"

run:
	@$(CONTROLLER) --topology $(TOPOLOGY)