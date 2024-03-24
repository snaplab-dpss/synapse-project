APP := ep0

P4_COMPILATION_VARS := 
CONTROLLER_ARGS := \
	--expiration-time $(or $(EXPIRATION_TIME),0) \
	--r0 $(or $(RATIO),0.5) \
	--iterations $(or $(ITERATIONS),0)

CURRENT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
include $(CURRENT_DIR)/../../../../tools/Makefile