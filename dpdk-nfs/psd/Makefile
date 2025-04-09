NF_DEVICES := 32

NF_FILES := psd_main.c config.c state.c ip_addr.c counter.c touched_port.c loop.c

NF_ARGS := --internal-devs 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30 \
           --fwd-rule 0,1 --fwd-rule 1,0 \
           --fwd-rule 2,3 --fwd-rule 3,2 \
           --fwd-rule 4,5 --fwd-rule 5,4 \
           --fwd-rule 6,7 --fwd-rule 7,6 \
           --fwd-rule 8,9 --fwd-rule 9,8 \
           --fwd-rule 10,11 --fwd-rule 11,10 \
           --fwd-rule 12,13 --fwd-rule 13,12 \
           --fwd-rule 14,15 --fwd-rule 15,14 \
           --fwd-rule 16,17 --fwd-rule 17,16 \
           --fwd-rule 18,19 --fwd-rule 19,18 \
           --fwd-rule 20,21 --fwd-rule 21,20 \
           --fwd-rule 22,23 --fwd-rule 23,22 \
           --fwd-rule 24,25 --fwd-rule 25,24 \
           --fwd-rule 26,27 --fwd-rule 27,26 \
           --fwd-rule 28,29 --fwd-rule 29,28 \
           --fwd-rule 30,31 --fwd-rule 31,30 \
           --capacity $(or $(CAPACITY),65536) \
           --max-ports $(or $(MAX_PORTS),16) \
           --expire $(or $(EXPIRATION_TIME),100000)

NF_LAYER := 4

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
