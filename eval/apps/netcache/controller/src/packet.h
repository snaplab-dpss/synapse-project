#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <iostream>

#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17
#define PORT_NETCACHE 50000

#define MAX_PACKET_SIZE 65536

namespace netcache {

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

struct netcache_hdr_t {
	uint8_t op;
	uint8_t seq;
	uint16_t key;
	uint32_t val;
} __attribute__((packed));

std::string mac_to_str(mac_t mac);
std::string ip_to_str(ipv4_t ip);
std::string port_to_str(port_t port);

struct pkt_hdr_t {
	uint8_t buffer[MAX_PACKET_SIZE];

	eth_hdr_t* get_l2() const { return (eth_hdr_t*)((uint8_t*)buffer); }

	size_t get_l2_size() const { return sizeof(eth_hdr_t); }

	ipv4_hdr_t* get_l3() const {
		auto l2_hdr = get_l2();
		auto l2_hdr_size = get_l2_size();
		return (ipv4_hdr_t*)((uint8_t*)l2_hdr + l2_hdr_size);
	}

	size_t get_l3_size() const { return sizeof(ipv4_hdr_t); }

	std::pair<void*, uint16_t> get_l4() const {
		auto ip_hdr = get_l3();
		auto ip_size = get_l3_size();

		switch (ip_hdr->protocol) {
			case IP_PROTO_TCP: {
				return std::pair<void*, uint16_t>((uint8_t*)ip_hdr + ip_size, IP_PROTO_TCP);
			} break;
			case IP_PROTO_UDP: {
				return std::pair<void*, uint16_t>((uint8_t*)ip_hdr + ip_size, IP_PROTO_UDP);
			} break;
			default: {
				printf("\n*** Not a TCP/UDP packet! ***\n");
				exit(1);
			} break;
		}
	}

	bool has_valid_protocol() const {
		auto l4_hdr_size = get_l4_size();

		if (l4_hdr_size == 0) {
			printf("\n*** Not a TCP/UDP packet! Can't extract the netcache header. ***\n");
			return false;
		}

		auto l4_hdr = get_l4();
		switch (l4_hdr.second) {
			case IP_PROTO_TCP: {
				tcp_hdr_t* tcp_hdr = (tcp_hdr_t*)l4_hdr.first;
				if (ntohs(tcp_hdr->src_port) == 50000 || ntohs(tcp_hdr->dst_port) == 50000) {
					return true;
				} else return false;
			} break;
			case IP_PROTO_UDP: {
				udp_hdr_t* udp_hdr = (udp_hdr_t*)l4_hdr.first;
				if (ntohs(udp_hdr->src_port) == 50000 || ntohs(udp_hdr->dst_port) == 50000) {
					return true;
				} else return false;
			} break;
			default: {
				return false;
			} break;
		}
	}

	size_t get_l4_size() const {
		auto ip_hdr = get_l3();

		switch (ip_hdr->protocol) {
			case IP_PROTO_TCP: {
				return sizeof(tcp_hdr_t);
			} break;
			case IP_PROTO_UDP: {
				return sizeof(udp_hdr_t);
			} break;
			default: {
				return 0;
			} break;
		}
	}

	size_t get_netcache_hdr_size() const {
		return sizeof(netcache_hdr_t);
	}

	netcache_hdr_t* get_netcache_hdr() const {
		auto l4_hdr = get_l4();
		auto l4_hdr_size = get_l4_size();

		if (l4_hdr_size == 0) {
			printf(
				"\n*** Not a TCP/UDP packet! Can't extract the netcache header. ***\n");
			exit(1);
		}

		auto netcache_hdr = (uint8_t*)l4_hdr.first + l4_hdr_size;
		return static_cast<netcache_hdr_t*>((void*)netcache_hdr);
	}

	void pretty_print_base() {
		auto eth_hdr = get_l2();
		auto ip_hdr = get_l3();
		auto l4_hdr = get_l4();

		printf("###[ Ethernet ]###\n");
		printf("  dst  %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->dst_mac[0],
			   eth_hdr->dst_mac[1], eth_hdr->dst_mac[2], eth_hdr->dst_mac[3],
			   eth_hdr->dst_mac[4], eth_hdr->dst_mac[5]);
		printf("  src  %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->src_mac[0],
			   eth_hdr->src_mac[1], eth_hdr->src_mac[2], eth_hdr->src_mac[3],
			   eth_hdr->src_mac[4], eth_hdr->src_mac[5]);
		printf("  type 0x%x\n", ntohs(eth_hdr->eth_type));

		printf("###[ IP ]###\n");
		printf("  ihl     %u\n", (ip_hdr->ihl & 0x0f));
		printf("  version %u\n", (ip_hdr->ihl & 0xf0) >> 4);
		printf("  tos     %u\n", ip_hdr->version);
		printf("  len     %u\n", ntohs(ip_hdr->tot_len));
		printf("  id      %u\n", ntohs(ip_hdr->id));
		printf("  off     %u\n", ntohs(ip_hdr->frag_off));
		printf("  ttl     %u\n", ip_hdr->ttl);
		printf("  proto   %u\n", ip_hdr->protocol);
		printf("  chksum  0x%x\n", ntohs(ip_hdr->check));
		printf("  src     %u.%u.%u.%u\n", (ip_hdr->src_ip >> 0) & 0xff,
			   (ip_hdr->src_ip >> 8) & 0xff, (ip_hdr->src_ip >> 16) & 0xff,
			   (ip_hdr->src_ip >> 24) & 0xff);
		printf("  dst     %u.%u.%u.%u\n", (ip_hdr->dst_ip >> 0) & 0xff,
			   (ip_hdr->dst_ip >> 8) & 0xff, (ip_hdr->dst_ip >> 16) & 0xff,
			   (ip_hdr->dst_ip >> 24) & 0xff);

		switch (l4_hdr.second) {
			case IP_PROTO_TCP: {
				auto tcp_hdr = static_cast<tcp_hdr_t*>(l4_hdr.first);
				printf("###[ TCP ]###\n");
				printf("  sport   %u\n", ntohs(tcp_hdr->src_port));
				printf("  dport   %u\n", ntohs(tcp_hdr->dst_port));
			} break;
			case IP_PROTO_UDP: {
				auto udp_hdr = static_cast<udp_hdr_t*>(l4_hdr.first);
				printf("###[ UDP ]###\n");
				printf("  sport   %u\n", ntohs(udp_hdr->src_port));
				printf("  dport   %u\n", ntohs(udp_hdr->dst_port));
			} break;
			default: {
				printf("\n*** Not TCP/UDP packet! ***\n");
			} break;
		}
	}

	void pretty_print_netcache() {

		if (!has_valid_protocol()) {
			return;
		}
		pretty_print_base();
		auto netcache_hdr = get_netcache_hdr();

		printf("###[ NetCache ]###\n");
		printf("  op                 %u\n", ntohl(netcache_hdr->op));
		printf("  seq                 %u\n", ntohl(netcache_hdr->seq));
		printf("  key                 %u\n", ntohl(netcache_hdr->key));
		printf("  val                 %u\n", ntohl(netcache_hdr->val));
	}
} __attribute__((packed));

};	// namespace netcache
