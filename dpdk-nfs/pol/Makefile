NF_FILES := policer_main.c policer_config.c ip_addr.c state.c

RATE  := 1073741824 # 2^30 = 1GBps
BURST := 131072     # 2^17 = 131KB

NF_ARGS := --wan 0 \
           --lan 1 \
           --rate $(or $(POLICER_RATE),$(RATE)) \
           --burst $(or $(POLICER_BURST),$(BURST)) \
           --capacity $(or $(CAPACITY),65536)

NF_LAYER := 3

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
