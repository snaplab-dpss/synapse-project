#pragma once

// Netcache

#define READ_QUERY 0
#define WRITE_QUERY 1
#define KVS_FAILURE 0
#define KVS_SUCCESS 1
#define KVSTORE_PORT 670
#define KVSTORE_CAPACITY (1 << 20)

#define KV_KEY_SIZE 16
#define KV_VAL_SIZE 16

// DPDK

#define BURST_SIZE 32
#define NUM_MBUFS 4096
#define MBUF_CACHE_SIZE 250
#define RX_RING_SIZE 2048
#define TX_RING_SIZE 2048
