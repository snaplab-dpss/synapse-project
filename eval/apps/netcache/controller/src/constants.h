#pragma once

// Netcache

#define READ_QUERY		0x0
#define WRITE_QUERY		0x1
#define DELETE_QUERY	0x2

// DPDK

#define NUM_MBUFS 8191
#define MBUF_DATA_SIZE RTE_MBUF_DEFAULT_BUF_SIZE
#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
