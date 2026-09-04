NF_DEVICES := 32

NF_FILES := hyperloglog_main.c config.c loop.c state.c

NF_ARGS := --estimators 64 --scaling 20

NF_LAYER := 3

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
