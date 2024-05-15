NF_FILES := bridge_main.c bridge_config.c stat_key.c loop.c state.c

NF_ARGS := --expire $(or $(EXPIRATION_TIME),1000000)

NF_DEVICES := 3

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
