NF_DEVICES := 32

NF_FILES := kvs_main.c config.c state.c entry.c loop.c

NF_ARGS := --server $(or $(SERVER),0) \
           --capacity $(or $(CAPACITY),65536) \
           --expire $(or $(EXPIRE),1000000)

NF_LAYER := 5

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
