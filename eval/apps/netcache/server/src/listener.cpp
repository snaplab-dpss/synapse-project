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
#include <arpa/inet.h>
#include <iostream>

#include "query.h"
#include "store.h"

#define BUFFER_SIZE 65536
#define IP_ADDR "127.0.0.1"
#define PORT 50051

typedef unsigned char byte_t;

namespace netcache {

Listener::Listener() {
	buffer = (byte_t *)malloc(sizeof(byte_t) * BUFFER_SIZE);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0) {
		perror("sock_recv creation failed");
		exit(1);
	}

    // struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("sock_recv bind failed.");
        close(sockfd);
        exit(1);
    }

	fflush(stdout);
}

Listener::~Listener() {
	close(sockfd);
}

query_t Listener::receive_query() {
	// Receive a packet
	ssize_t len =
		recvfrom(sockfd, buffer, 65536, 0, (struct sockaddr *)&ctrl_addr, (socklen_t *)&ctrl_len);
	
	if (len < 0) {
		printf("recvfrom error , failed to receive query.\n");
		exit(1);
	}

	std::vector<uint8_t> buffer_vec(buffer, buffer + 65536);

	return query_t(buffer_vec, len);	
}

};	// namespace netcache
