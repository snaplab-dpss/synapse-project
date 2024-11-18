#pragma once

#include <vector>
#include <stdexcept>
#include <stdint.h>

#include "packet.h"

namespace netcache {

struct query_t {
	bool valid;

	// mac_t mac_src;
	// ipv4_t ip_src;
	// ipv4_t ip_dst;
	// uint8_t ip_proto;
	// port_t port_src;
	// port_t port_dst;
	uint8_t op;
	uint8_t seq;
	uint16_t key;
	uint32_t val;

	query_t(pkt_hdr_t* pkt, ssize_t pkt_size) {
		valid = pkt->has_valid_protocol();

		if (!valid) {
			#ifdef DEBUG
			printf("Invalid protocol packet. Ignoring.\n");
			#endif
			return;
		}

		valid =
			(pkt_size >= (pkt->get_l2_size()
						  + pkt->get_l3_size()
						  + pkt->get_l4_size()
						  + pkt->get_netcache_hdr_size()));

		if (!valid) {
			#ifdef DEBUG
			printf("Packet too small. Ignoring.\n");
			#endif
			return;
		}

		auto l2 = pkt->get_l2();
		auto l3 = pkt->get_l3();
		auto l4 = pkt->get_l4();

		// for (auto byte = 0; byte < sizeof(mac_t); byte++) {
		// 	mac_src[byte] = l2->src_mac[byte];
		// }

		// ip_src = l3->src_ip;
		// ip_dst = l3->dst_ip;
		// ip_proto = l3->protocol;

		// switch (ip_proto) {
		// 	case IP_PROTO_TCP: {
		// 		auto tcp_hdr = (tcp_hdr_t*)l4.first;
		// 		port_src = tcp_hdr->src_port;
		// 		port_dst = tcp_hdr->dst_port;
		// 	} break;
		// 	case IP_PROTO_UDP: {
		// 		auto udp_hdr = (udp_hdr_t*)l4.first;
		// 		port_src = udp_hdr->src_port;
		// 		port_dst = udp_hdr->dst_port;
		// 	} break;
		// }

		auto netcache_hdr = pkt->get_netcache_hdr();

		op = netcache_hdr->op;
		seq = netcache_hdr->seq;
		key = netcache_hdr->key;
		val = netcache_hdr->val;
	}

	void serialize_append(std::vector<uint8_t>& buffer, uint64_t value, size_t bytes) const {
		for (auto i = 0; i < bytes; i++) {
			auto byte = (value >> ((bytes - i - 1) * 8)) & 0xff;
			buffer.push_back(byte);
		}
	}

	std::vector<uint8_t> serialize() const {
		auto buffer = std::vector<uint8_t>();

		serialize_append(buffer, op, 1);
		serialize_append(buffer, seq, 4);
		serialize_append(buffer, key, 2);
		serialize_append(buffer, val, 4);

		return buffer;
	}
};

}  // namespace netcache
