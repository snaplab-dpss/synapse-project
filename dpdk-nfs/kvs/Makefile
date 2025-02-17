NF_DEVICES := 32

NF_FILES := kvs_main.c config.c state.c entry.c loop.c

NF_ARGS := --server $(or $(SERVER),0) \
           --lpm-entry 0.0.0.0/6,0 \
           --lpm-entry 4.0.0.0/6,1 \
           --lpm-entry 8.0.0.0/6,2 \
           --lpm-entry 12.0.0.0/6,3 \
           --lpm-entry 16.0.0.0/6,4 \
           --lpm-entry 20.0.0.0/6,5 \
           --lpm-entry 24.0.0.0/6,6 \
           --lpm-entry 28.0.0.0/6,7 \
           --lpm-entry 32.0.0.0/6,8 \
           --lpm-entry 36.0.0.0/6,9 \
           --lpm-entry 40.0.0.0/6,10 \
           --lpm-entry 44.0.0.0/6,11 \
           --lpm-entry 48.0.0.0/6,12 \
           --lpm-entry 52.0.0.0/6,13 \
           --lpm-entry 56.0.0.0/6,14 \
           --lpm-entry 60.0.0.0/6,15 \
           --lpm-entry 64.0.0.0/6,16 \
           --lpm-entry 68.0.0.0/6,17 \
           --lpm-entry 72.0.0.0/6,18 \
           --lpm-entry 76.0.0.0/6,19 \
           --lpm-entry 80.0.0.0/6,20 \
           --lpm-entry 84.0.0.0/6,21 \
           --lpm-entry 88.0.0.0/6,22 \
           --lpm-entry 92.0.0.0/6,23 \
           --lpm-entry 96.0.0.0/6,24 \
           --lpm-entry 100.0.0.0/6,25 \
           --lpm-entry 104.0.0.0/6,26 \
           --lpm-entry 108.0.0.0/6,27 \
           --lpm-entry 112.0.0.0/6,28 \
           --lpm-entry 116.0.0.0/6,29 \
           --lpm-entry 120.0.0.0/6,30 \
           --lpm-entry 124.0.0.0/6,31 \
           --capacity $(or $(CAPACITY),8192) \
           --expire $(or $(EXPIRE),1000000)

NF_LAYER := 5

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
