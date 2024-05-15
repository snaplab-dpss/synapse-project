NF_FILES := bridge_main.c bridge_config.c dyn_value.c loop.c state.c

NF_ARGS := --expire $(or $(EXPIRATION_TIME),1000000) --capacity $(or $(CAPACITY),65536)

NF_DEVICES := 2

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
