#include "listener.h"

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "query.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

Listener::Listener(const std::string &iface) {
	std::vector<uint8_t> buffer;
	sock_recv = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if (sock_recv < 0) {
		perror("sock_recv creation failed");
		exit(1);
	}

	struct sockaddr_ll saddr;
	memset(&saddr, 0, sizeof(struct sockaddr_ll));

	saddr.sll_family = AF_PACKET;
	saddr.sll_protocol = htons(ETH_P_ALL);
	saddr.sll_ifindex = if_nametoindex(iface.c_str());

	if (bind(sock_recv, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		perror("sock_recv bind failed");
		close(sock_recv);
		exit(1);
	}

	printf("Listening to interface %s...\n", iface.c_str());
	fflush(stdout);
}

Listener::~Listener() {
	close(sock_recv);
}

query_t Listener::receive_query() {
	struct sockaddr_ll saddr;
	auto saddr_size = sizeof(struct sockaddr);
	std::vector<uint8_t> buffer(BUFFER_SIZE);

	// Receive a packet
	ssize_t len =
		recvfrom(sock_recv, &buffer, sizeof(buffer), 0, (struct sockaddr *)&saddr,
				(socklen_t *)&saddr_size);

	if (len < 0) {
		printf("recvfrom error , failed to receive query.\n");
		exit(1);
	}

	return query_t(buffer, len);	
}

};	// namespace netcache
