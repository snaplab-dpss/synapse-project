APP := table_map

P4_COMPILATION_VARS := 
CONTROLLER_ARGS := --expiration-time $(or $(EXPIRATION_TIME),1000)

CURRENT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
include $(CURRENT_DIR)/../../../../tools/Makefile