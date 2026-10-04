#pragma once

#define ETHER_TYPE_IPV4 0x0800
#define UDP_PROTO 17
#define TCP_PROTO 6

#define CPU_PORT_TNA1 320
#define CPU_PORT_TNA2 0

constexpr const int SWITCH_PACKET_MAX_BUFFER_SIZE = 10000;

// Netcache

#define READ_QUERY 0x0
#define WRITE_QUERY 0x1
#define DELETE_QUERY 0x2
#define NC_PORT 670
#define NC_HDR_SIZE 10

#define KV_KEY_SIZE 4
#define KV_VAL_SIZE 4

#define BURST_SIZE 1

// The cached keys' counters are cleared every this many sketch resets: a reported key competes
// against counts accumulated over several periods, not against counters just zeroed with the
// sketch, which would let any report displace a cached key right after every reset.
#define KEY_COUNT_RESET_PERIODS 3

// A cached key whose traffic stops for this long frees its slot. Without it a slot is reclaimed
// only when a random probe happens to land on it, so under churn a growing share of the cache
// holds keys that no longer receive traffic.
#define KEY_IDLE_TIMEOUT_MS 1000
