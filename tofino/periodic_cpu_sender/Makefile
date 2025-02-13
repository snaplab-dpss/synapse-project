APP := periodic_cpu_sender

P4_COMPILATION_VARS := 
CONTROLLER_ARGS := --ratio=$(or $(RATIO),0.5) --iterations $(or $(ITERATIONS),0)

CURRENT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
include $(CURRENT_DIR)/../../../tools/Makefile