NF_FILES := kvs_main.c kvs_config.c kvs_manager.c state.c entry.c loop.c

NF_ARGS := \
	--ext $(or $(EXTERNAL),0) \
	--int $(or $(INTERNAL),1) \
	--capacity $(or $(CAPACITY),16384) \
	--sample-size $(or $(SAMPLE_SIZE),100) \
	--sketch-height $(or $(SKETCH_HEIGHT),4) \
	--sketch-width $(or $(SKETCH_WIDTH),16384) \
	--sketch-cleanup-interval $(or $(SKETCH_CLEANUP_INTERVAL),1000000) \
	--hh-threshold $(or $(HH_THRESHOLD),10000)

NF_LAYER := 5

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
