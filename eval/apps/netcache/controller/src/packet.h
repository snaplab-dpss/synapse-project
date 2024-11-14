#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

#define MAX_PACKET_SIZE 10000

namespace peregrine {

typedef uint8_t mac_t[6];
typedef uint32_t ipv4_t;
typedef uint16_t port_t;

struct eth_hdr_t {
	mac_t dst_mac;
	mac_t src_mac;
	uint16_t eth_type;
} __attribute__((packed));

struct ipv4_hdr_t {
	uint8_t ihl : 4;
	uint8_t version : 4;
	uint8_t ecn : 2;
	uint8_t dscp : 6;
	uint16_t tot_len;
	uint16_t id;
	uint16_t frag_off;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t check;
	ipv4_t src_ip;
	ipv4_t dst_ip;
} __attribute__((packed));

struct tcp_hdr_t {
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t seq_no;
	uint32_t ack_no;
	uint16_t opts;
	uint16_t window;
	uint16_t checksum;
	uint16_t urgent_ptr;
} __attribute__((packed));

struct udp_hdr_t {
	port_t src_port;
	port_t dst_port;
	uint16_t length;
	uint16_t checksum;
} __attribute__((packed));

struct icmp_hdr_t {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
} __attribute__((packed));

};	// namespace peregrine