#include <iostream>

#include "store.h"
#include "constants.h"
#include "server_reply.h"

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

Store::Store() {
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

void Store::hot_read_query(const query_t& query) {
	// Retrieve value from KV map using query.key.
	uint32_t val;
	auto it = kv_map.find(query.key);

    if (it != kv_map.end()) {
		val = 0;
    } else {
        val = it->second;
    }

	// Initialize a server_reply struct with the op, key and newly obtained value.
	server_reply_t reply = server_reply_t(query.key, val);

	// Serialize server_reply and send it to the controller.
	auto buffer = reply.serialize();
	auto sent_len =
		sendto(sockfd, (const char*)buffer.data(), buffer.size(), MSG_CONFIRM,
			   (const struct sockaddr*)&servaddr, sizeof(servaddr));

	if (sent_len != buffer.size()) {
		fprintf(stderr, "Truncated packet.\n");
		exit(EXIT_FAILURE);
	}
}

void Store::write_query(const query_t& query) {
	// Add/Update the value on the KV map using query.key.
	kv_map[query.key] = query.val;

	// If successful, send a confirmation to the controller.
	float ack = 0;
	auto sent_len =
		sendto(sockfd, &ack, sizeof(ack), MSG_CONFIRM,
			   (const struct sockaddr*)&servaddr, sizeof(servaddr));

	if (sent_len < sizeof(ack)) {
		fprintf(stderr, "Truncated packet.\n");
		exit(EXIT_FAILURE);
	}
}

void Store::del_query(const query_t& query) {
	// Delete the key/value from the KV map using query.key.
	kv_map.erase(query.key);
	// If successful, send a confirmation to the controller.
	float ack = 0;
	auto sent_len =
		sendto(sockfd, &ack, sizeof(ack), MSG_CONFIRM,
			   (const struct sockaddr*)&servaddr, sizeof(servaddr));

	if (sent_len < sizeof(ack)) {
		fprintf(stderr, "Truncated packet.\n");
		exit(EXIT_FAILURE);
	}
}

}  // namespace netcache
