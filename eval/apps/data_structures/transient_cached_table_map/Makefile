APP := transient_cached_table_map

P4_COMPILATION_VARS := \
	-DCACHE_CAPACITY_EXPONENT_BASE_2=$(or $(CACHE_CAPACITY_EXPONENT_BASE_2),13) \
	-DEXPIRATION_TIME_SEC=$(or $(EXPIRATION_TIME_SEC),1)

CONTROLLER_ARGS := --expiration-time $(or $(EXPIRATION_TIME),1000)

CURRENT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
include $(CURRENT_DIR)/../../../../tools/Makefile