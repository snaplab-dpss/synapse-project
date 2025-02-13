#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <iostream>

#define PORT_NETCACHE 50000

#define MAX_PACKET_SIZE 65536

namespace netcache {

struct netcache_hdr_t {
	uint8_t op;
	uint8_t seq;
	uint16_t key;
	uint32_t val;
	uint16_t cpu;
} __attribute__((packed));

};	// namespace netcache
