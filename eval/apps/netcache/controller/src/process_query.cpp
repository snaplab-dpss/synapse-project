#include <iostream>

#include "process_query.h"
#include "constants.h"
#include "server_reply.h"
#include "netcache.h"

#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 50051

namespace netcache {

ProcessQuery::ProcessQuery() {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// Creating socket file descriptor
	if (sockfd < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERVER_PORT);
	servaddr.sin_addr.s_addr = inet_addr(SERVER_HOST);
}

void ProcessQuery::hot_read_query(const query_t& hot_read_query) {
	// Sample k values from the switch's cached item counter array.
	// k is defined in conf.kv.initial_entries.

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution<> dis(1, Controller::controller->conf.kv.store_size);
	std::unordered_set<int> elems;

	while (elems.size() < Controller::controller->conf.kv.initial_entries) {
		elems.insert(dis(gen));
	}

	std::vector<int> sampl_index(elems.begin(), elems.end());

	// Vector that stores all key indexes and counters randomly sampled from the switch.
	std::vector<std::vector<uint32_t>> sampl_cntr;

	for (int i: sampl_index) {
		sampl_cntr.push_back({(uint32_t) i,
							   Controller::controller->reg_vtable.retrieve(i, false)});
	}

	// Retrieve the index of the smallest counter value from the vector.

	uint32_t smallest_val = std::numeric_limits<uint32_t>::max();
	int smallest_idx = -1;

	for (size_t i = 0; i < sampl_cntr.size(); ++i) {
		if (sampl_cntr[i][1] < smallest_val) {
			smallest_val = sampl_cntr[i][1];
			smallest_idx = static_cast<int>(i);
		}
	}

	// If the smallest counter value < HH report counter:
	if (sampl_cntr[smallest_idx][1] < hot_read_query.val) {
		// Evict the key corresponding to the smallest.
		Controller::controller->reg_vtable.allocate((uint32_t) sampl_cntr[smallest_idx][0], 0);

		// Retrieve the updated HH report value from the server.

		auto buffer = hot_read_query.serialize();

		auto sent_len =
			sendto(sockfd, (const char*)buffer.data(), buffer.size(), MSG_CONFIRM,
				(const struct sockaddr*)&servaddr, sizeof(servaddr));

		if (sent_len != buffer.size()) {
			fprintf(stderr, "Truncated packet.\n");
			exit(EXIT_FAILURE);
		}
		struct sockaddr_in src_addr;
		socklen_t socklen = sizeof(src_addr);
		std::vector<uint8_t> buffer_reply;

		ssize_t len = recvfrom(sockfd, &buffer_reply, 65536, 0,
							(struct sockaddr*)&src_addr, &socklen);
		if (len < 0) {
			std::cerr << "Failed to receive a response from the server." << std::endl;
			return;
		}

		server_reply_t reply = server_reply_t(buffer_reply, len);

		#ifndef NDEBUG
		std::cout << "reply.key " << reply.key << "\n";
		std::cout << "reply.val " << reply.val << "\n";
		#endif

		// Update the key/value corresponding to the HH report directly on the switch.
		Controller::controller->reg_vtable.allocate(reply.key, reply.val);

		// Update the cache lookup table to delete the corresponding key.
		Controller::controller->cache_lookup.del_entry(reply.key);
	}
}

void ProcessQuery::write_query(const query_t& write_query) {

	auto buffer = write_query.serialize();

	auto sent_len =
		sendto(sockfd, (const char*)buffer.data(), buffer.size(), MSG_CONFIRM,
			   (const struct sockaddr*)&servaddr, sizeof(servaddr));

	if (sent_len != buffer.size()) {
		fprintf(stderr, "Truncated packet.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in src_addr;
	socklen_t socklen = sizeof(src_addr);
	float write_confirm;

	ssize_t recv_len = recvfrom(sockfd, &write_confirm, sizeof(write_confirm), 0,
								(struct sockaddr*)&src_addr, &socklen);

	if (recv_len < 0) {
		if (write_confirm != -1) {
			std::cout << "Value successfully updated in the server." << std::endl;
		}
	} else {
		std::cerr << "Failed to receive a confirmation from the server." << std::endl;
		return;
	}
}

void ProcessQuery::del_query(const query_t& del_query) {

	auto buffer = del_query.serialize();

	auto sent_len =
		sendto(sockfd, (const char*)buffer.data(), buffer.size(), MSG_CONFIRM,
			   (const struct sockaddr*)&servaddr, sizeof(servaddr));

	if (sent_len != buffer.size()) {
		fprintf(stderr, "Truncated packet.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in src_addr;
	socklen_t socklen = sizeof(src_addr);
	float del_confirm;

	ssize_t len = recvfrom(sockfd, &del_confirm, sizeof(del_confirm), 0,
						   (struct sockaddr*)&src_addr, &socklen);

	if (len < 0) {
		if (del_confirm != -1) {
			std::cout << "Value successfully deleted from the server." << std::endl;
		}
	} else {
		std::cerr << "Failed to receive a confirmation from the server." << std::endl;
		return;
	}
}

}  // namespace netcache
