NF_FILES := kvstore_main.c kvstore_config.c kvstore_manager.c state.c entry.c loop.c

NF_ARGS := --max-entries $(or $(CAPACITY),65536)

NF_LAYER := 5

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
