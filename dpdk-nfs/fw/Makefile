NF_DEVICES := 30

NF_FILES := fw_main.c config.c flowmanager.c flow.c loop.c state.c

NF_AUTOGEN_SRCS := flow.h

NF_ARGS := --internal-devs 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28 \
           --fwd-rule 0,1 \
           --fwd-rule 2,3 \
           --fwd-rule 4,5 \
           --fwd-rule 6,7 \
           --fwd-rule 8,9 \
           --fwd-rule 10,11 \
           --fwd-rule 12,13 \
           --fwd-rule 14,15 \
           --fwd-rule 16,17 \
           --fwd-rule 18,19 \
           --fwd-rule 20,21 \
           --fwd-rule 22,23 \
           --fwd-rule 24,25 \
           --fwd-rule 26,27 \
           --fwd-rule 28,29 \
           --expire 1000000 \
           --max-flows 65536

NF_LAYER := 4

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
