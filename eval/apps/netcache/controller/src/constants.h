#pragma once

#define ETHER_TYPE_IPV4 0x0800
#define UDP_PROTO 17
#define TCP_PROTO 6

// Netcache

#define READ_QUERY		0x0
#define WRITE_QUERY		0x1
#define DELETE_QUERY	0x2
#define NC_PORT			50000
#define NC_HDR_SIZE		8

// DPDK

#define BURST_SIZE 1
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
