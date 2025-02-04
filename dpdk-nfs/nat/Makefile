NF_DEVICES := 30

NF_FILES := nat_main.c config.c flowmanager.c flow.c loop.c state.c

NF_AUTOGEN_SRCS := flow.h

NF_ARGS := --wan $(or $(WAN),0) \
           --expire $(or $(EXPIRATION_TIME),1000000) \
           --max-flows 65536 \
           --extip $(or $(MB_IP_EXTERNAL),1.2.3.4)

NF_LAYER := 4


include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
