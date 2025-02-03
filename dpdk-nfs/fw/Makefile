NF_DEVICES := 30

NF_FILES := fw_main.c fw_config.c fw_flowmanager.c flow.c loop.c state.c

NF_AUTOGEN_SRCS := flow.h

NF_ARGS := --expire $(or $(EXPIRATION_TIME),1000000) \
           --max-flows $(or $(CAPACITY),65536) \
           --fwd-table $(or $(FWD_TABLE),fwd_tbl.txt)

NF_LAYER := 4

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
