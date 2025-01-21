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

#include <iostream>
// #include <rte_eal.h>
// #include <rte_ethdev.h>
// #include <rte_mbuf.h>
// #include <rte_mbuf_core.h>

#include "packet.h"
#include "constants.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

Listener::Listener(const std::string &iface) {
	// struct rte_eth_dev_info dev_info;
	// uint16_t num_ports = rte_eth_dev_count_avail();

	// for (int i = 0; i < num_ports; i++) {
	// 	rte_eth_dev_info_get(i, &dev_info);
	// 	if (iface == dev_info.driver_name) {
	// 		port_id = i;
	// 		break;
	// 	}
	// 	std::cerr << "Iface " << iface << " not found." << std::endl;
	// 	exit(1);
	// }

	// struct rte_eth_conf port_conf;
	// memset(&port_conf, 0, sizeof(port_conf));

	// rte_eth_dev_configure(port_id, 1, 1, &port_conf);
	// rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE, rte_eth_dev_socket_id(port_id), NULL,
	// 					   rte_pktmbuf_pool_create("mbuf_pool", NUM_MBUFS, 0, 0, MBUF_DATA_SIZE,
	// 											   rte_socket_id()));
	// rte_eth_dev_start(port_id);

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

	// rte_eth_dev_stop(port_id);
	// rte_eth_dev_close(port_id);

	close(sock_recv);
	free(buffer);
}

query_t Listener::receive_query() {
	// uint16_t num_rx = rte_eth_rx_burst(port_id, 0, buf, 1);
	// auto pkt = (pkt_hdr_t *)rte_pktmbuf_mtod(buf[0], char *);

	// #ifndef NDEBUG
	// 	if (pkt->has_valid_protocol()) {
	// 		pkt->pretty_print_base();
	// 		pkt->pretty_print_netcache();
	// 	}
	// #endif

	// size_t data_size = rte_pktmbuf_data_len(buf[0]);

	// rte_pktmbuf_free(buf[0]);

	// return query_t(pkt, data_size);


	struct sockaddr_ll saddr;
	auto saddr_size = sizeof(struct sockaddr);

	// Receive a packet */
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
