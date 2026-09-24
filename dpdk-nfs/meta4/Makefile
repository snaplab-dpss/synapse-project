NF_DEVICES := 32

NF_FILES := meta4_main.c config.c state.c session.c

# The watch list, one name per line in domains.txt. A name's position there is the domain ID its
# counters are indexed by, and without at least one the NF has nothing to attribute traffic to.
# These are the exact names from the domain list the Meta4 authors published
# (tofino/meta4/known_domains_v1.txt) that fit what this parser reads: at most 4 labels of at most
# 15 bytes, and no wildcard other than a leading one. A leading "*" label matches any label there,
# so *.google.com covers www.google.com and the rest of its subdomains without listing them.
# The traffic generator reads the same file, so every name it emits is one the NF is watching for.
NF_DOMAINS := $(shell cat $(dir $(lastword $(MAKEFILE_LIST)))domains.txt)

# Clients left out of the accounting. The same prefix test.py uses, so a default run and the test
# agree on which addresses are meant to be ignored.
NF_IGNORED_CLIENTS := 10.9.0.0/24

NF_ARGS := --drt-capacity $(or $(DRT_CAPACITY),65536) \
           --drt-expire $(or $(DRT_EXPIRATION_TIME),1000000) \
           --known-domains $(or $(NUM_KNOWN_DOMAINS),2048) \
           $(foreach d,$(NF_DOMAINS),--domain $(d)) \
           $(foreach c,$(NF_IGNORED_CLIENTS),--ignore-client $(c))


include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/../Makefile
