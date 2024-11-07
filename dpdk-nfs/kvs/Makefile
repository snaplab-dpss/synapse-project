NF_FILES := kvs_main.c kvs_config.c state.c entry.c loop.c

NF_ARGS := \
	--client $(or $(CLIENT),0) \
	--server $(or $(SERVER),1) \
	--capacity $(or $(CAPACITY),16384) \
	--expire $(or $(EXPIRE),1000000)

NF_LAYER := 5

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
