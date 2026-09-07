NF_DEVICES := 32

NF_FILES := smartcookie_main.c config.c state.c flow_id.c loop.c

# Keys are upstream's defaults 0x33323130 / 0x42413938, given in decimal.
NF_ARGS := --server-dev $(or $(SERVER_DEV),0) \
           --sip-key0 $(or $(SIP_KEY_0),857870640) \
           --sip-key1 $(or $(SIP_KEY_1),1111570744) \
           --bf-height $(or $(BLOOM_FILTER_HEIGHT),2) \
           --bf-width $(or $(BLOOM_FILTER_WIDTH),1048576)

NF_LAYER := 4

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
