#include "listener.h"

#include <cstddef>
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

/* #include <rte_eal.h> */
/* #include <rte_ethdev.h> */
/* #include <rte_mbuf.h> */
/* #include <rte_log.h> */

#include "query.h"
#include "store.h"
#include "constants.h"

#define BUFFER_SIZE 65536
#define IP_ADDR "127.0.0.1"
#define PORT 50051

typedef unsigned char byte_t;

namespace netcache {

/* Listener::Listener(const int dpdk_port) { */
Listener::Listener() {

	/* struct rte_eth_conf port_conf; */
	/* const uint16_t rx_rings = 1, tx_rings = 1; */
	/* uint16_t nb_rxd = RX_RING_SIZE; */
	/* uint16_t nb_txd = TX_RING_SIZE; */
	/* int retval; */
	/* uint16_t q; */
	/* struct rte_eth_dev_info dev_info; */
	/* struct rte_eth_txconf txconf; */

	/* if (!rte_eth_dev_is_valid_port(dpdk_port)) { */
	/* 	exit(1); */
	/* } */

	/* memset(&port_conf, 0, sizeof(struct rte_eth_conf)); */

	/* retval = rte_eth_dev_info_get(dpdk_port, &dev_info); */
	/* if (retval != 0) { */
	/* 	printf("Error during getting device (port %u) info: %s\n", dpdk_port, strerror(-retval)); */
	/* 	exit(1); */
	/* } */

	/* if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE) */
	/* 	port_conf.txmode.offloads |= */
	/* 		RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE; */

	/* /1* Configure the Ethernet device. *1/ */
	/* retval = rte_eth_dev_configure(dpdk_port, rx_rings, tx_rings, &port_conf); */
	/* if (retval != 0) */
	/* 	exit(1); */

	/* retval = rte_eth_dev_adjust_nb_rx_tx_desc(dpdk_port, &nb_rxd, &nb_txd); */
	/* if (retval != 0) */
	/* 	exit(1); */

	/* /1* Allocate and set up 1 RX queue per Ethernet port. *1/ */
	/* for (q = 0; q < rx_rings; q++) { */
	/* 	retval = rte_eth_rx_queue_setup(dpdk_port, q, nb_rxd, */
	/* 			rte_eth_dev_socket_id(dpdk_port), NULL, mbuf_pool); */
	/* 	if (retval < 0) */
	/* 		exit(1); */
	/* } */

	/* txconf = dev_info.default_txconf; */
	/* txconf.offloads = port_conf.txmode.offloads; */
	/* /1* Allocate and set up 1 TX queue per Ethernet port. *1/ */
	/* for (q = 0; q < tx_rings; q++) { */
	/* 	retval = rte_eth_tx_queue_setup(dpdk_port, q, nb_txd, */
	/* 			rte_eth_dev_socket_id(dpdk_port), &txconf); */
	/* 	if (retval < 0) */
	/* 		exit(1); */
	/* } */
	/* std::cout << "AAAAAAAAAA" << std::endl; */

	/* /1* Starting Ethernet port. 8< *1/ */
	/* retval = rte_eth_dev_start(dpdk_port); */

	/* std::cout << "BBBBBBBBB" << std::endl; */

	/* struct rte_eth_dev_info dev_info; */

		/* port_id = dpdk_port; */

	/* struct rte_eth_conf port_conf; */
	/* memset(&port_conf, 0, sizeof(port_conf)); */

	/* rte_eth_dev_configure(port_id, 1, 1, &port_conf); */
	/* rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE, rte_eth_dev_socket_id(port_id), NULL, */
	/* 					   rte_pktmbuf_pool_create("mbuf_pool", NUM_MBUFS, 0, 0, MBUF_DATA_SIZE, */
	/* 											   rte_socket_id())); */
	/* uint16_t rx_desc = 1024; */
    /* uint16_t tx_desc = 1024; */
	/* rte_eth_dev_adjust_nb_rx_tx_desc(port_id, &rx_desc, &tx_desc); */
	/* rte_eth_dev_start(port_id); */

	/* struct sockaddr_in target_addr; */
	/* target_addr.sin_family = AF_INET; */
	/* target_addr.sin_addr.s_addr = rte_cpu_to_be_32(inet_addr(IP_ADDR)); */
	/* target_addr.sin_port = rte_cpu_to_be_16(PORT); */

	/* buffer = (byte_t *)malloc(sizeof(byte_t) * BUFFER_SIZE); */
	/* sockfd = socket(AF_INET, SOCK_DGRAM, 0); */



	/* if (sockfd < 0) { */
	/*	perror("sock_recv creation failed"); */
	/*	exit(1); */
	/* } */

	/* // struct sockaddr_in serv_addr; */
	/* memset(&serv_addr, 0, sizeof(serv_addr)); */
	/* serv_addr.sin_family = AF_INET; */
	/* serv_addr.sin_addr.s_addr = inet_addr(IP_ADDR); */
	/* serv_addr.sin_port = htons(PORT); */

	/* if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) { */
		/* perror("sock_recv bind failed."); */
		/* close(sockfd); */
		/* exit(1); */
	/* } */

	/* fflush(stdout); */
}

Listener::~Listener() {
	close(sockfd);
}

query_t Listener::receive_query() {
	/* uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, buf, 1); */

	/* // If nothing is received, return an empty query struct and return. */
	/* if (nb_rx == 0) { */
	/* 	return query_t(); */
	/* } */
	/* count++; */

	/* std::cout << "Packet received: count " << count << "\n"; */
	/* auto buffer = (uint8_t *)rte_pktmbuf_mtod(buf[0], char *); */

	/* size_t data_size = rte_pktmbuf_data_len(buf[0]); */

	/* rte_pktmbuf_free(buf[0]); */

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
