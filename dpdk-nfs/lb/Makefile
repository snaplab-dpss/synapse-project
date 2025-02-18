NF_DEVICES := 32

NF_FILES := lb_main.c balancer.c config.c ip_addr.c backend.c flow.c loop.c state.c

NF_AUTOGEN_SRCS := flow.h backend.h ip_addr.h

# CHT height must be a prime number
NF_ARGS := --flow-expiration $(or $(EXPIRATION_TIME),1000000) \
           --flow-capacity $(or $(CAPACITY),65536) \
           --backend-capacity 32 \
           --cht-height 97 \
           --backend-expiration 100000000

NF_LAYER := 3

NF_BENCH_NEEDS_REVERSE_TRAFFIC := true

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
