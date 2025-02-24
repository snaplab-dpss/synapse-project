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
	uint16_t key;
	uint32_t val;
	uint8_t status;
	uint16_t port;
} __attribute__((packed));

};	// namespace netcache
