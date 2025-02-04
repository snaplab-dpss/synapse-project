NF_DEVICES := 30

NF_FILES := fw_main.c config.c flowmanager.c flow.c loop.c state.c

NF_AUTOGEN_SRCS := flow.h

NF_ARGS := --wan $(or $(WAN),0) \
           --expire $(or $(EXPIRATION_TIME),1000000) \
           --max-flows $(or $(CAPACITY),65536)

NF_LAYER := 4

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
