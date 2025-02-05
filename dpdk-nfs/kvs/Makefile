NF_DEVICES := 30

NF_FILES := kvs_main.c kvs_config.c state.c entry.c loop.c

NF_ARGS := \
	--server $(or $(SERVER),0) \
	--lpm-config $(or $(LPM_CONFIG),"lpm.cfg") \
	--capacity $(or $(CAPACITY),8192) \
	--expire $(or $(EXPIRE),1000000)

NF_LAYER := 5

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
