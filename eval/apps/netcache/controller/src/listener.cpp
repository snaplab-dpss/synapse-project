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

#include "packet.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

Listener::Listener(const std::string &iface) {
	buffer = (byte_t *)malloc(sizeof(byte_t) * BUFFER_SIZE);
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
	free(buffer);
}

query_t Listener::receive_query() {
	struct sockaddr_ll saddr;
	auto saddr_size = sizeof(struct sockaddr);

	// Receive a packet
	auto data_size =
		recvfrom(sock_recv, buffer, 65536, 0, (struct sockaddr *)&saddr, (socklen_t *)&saddr_size);

	if (data_size < 0) {
		printf("recvfrom error , failed to get packets\n");
		exit(1);
	}
	auto pkt = (pkt_hdr_t *)(buffer);

#ifndef NDEBUG
	if (pkt->has_valid_protocol()) {
		pkt->pretty_print_base();
		pkt->pretty_print_netcache();
	}
#endif

	return query_t(pkt, data_size);
}

};	// namespace netcache
