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

#define KV_KEY_SIZE 16
#define KV_VAL_SIZE 16

#define BURST_SIZE 1
