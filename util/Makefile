# get current dir, see https://stackoverflow.com/a/8080530
SELF_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# Output directory
OUT_DIR=build

PKGCONF ?= pkg-config
PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS  += $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS  = $(shell $(PKGCONF) --static --libs libdpdk)
LDFLAGS += -L$(RTE_SDK)/$(RTE_TARGET)

# Same name for everyone, makes it easier to run them all with the same script
APP := nf
# allow the use of advanced globs in paths
SHELL := /bin/bash -O extglob -O globstar -c
# NF base source;
# somehow because of DPDK makefile magic wildcards mess up everything here,
# so we ask echo to expand those
SRCS-y = ./x86.c
# Compiler flags
CFLAGS += -std=gnu11
CFLAGS += -DCAPACITY_POW2
CFLAGS += -O3
# CFLAGS += -O0 -g -rdynamic -DENABLE_LOG -Wfatal-errors

all: $(OUT_DIR)/$(APP)

$(OUT_DIR)/$(APP): $(SRCS-y) $(PC_FILE)
	@mkdir -p $(OUT_DIR)
	@$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS)