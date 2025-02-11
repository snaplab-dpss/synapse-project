// #include "listener.h"

// #include <linux/if_packet.h>
// #include <net/ethernet.h>
// #include <net/if.h>
// #include <netdb.h>
// #include <netinet/in.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <unistd.h>

// #include <iostream>
// #include <rte_eal.h>
// #include <rte_ethdev.h>
// #include <rte_mbuf.h>
// #include <rte_mbuf_core.h>

// #include "packet.h"
// #include "constants.h"

// typedef unsigned char byte_t;

// namespace netcache {

// Listener::Listener(const int dpdk_port) {
// 	port_id = dpdk_port;
// }

// Listener::~Listener() {}

// pkt_hdr_t Listener::receive_query() {
// 	uint16_t nb_rx = rte_eth_rx_burst(1, 0, buf, BURST_SIZE);

// 	if (nb_rx == 0) {
// 		pkt_hdr_t pkt;
// 		return pkt;
// 	}
// 	count++;

// 	std::cout << "Packet received: count " << count << "\n";

// 	auto buffer = (uint8_t *)rte_pktmbuf_mtod(buf[0], char *);
// 	size_t data_size = rte_pktmbuf_data_len(buf[0]);

// 	/* rte_pktmbuf_free(buf[0]); */

// 	auto pkt = (pkt_hdr_t *)(buffer);

// 	#ifndef NDEBUG
// 	if (pkt->has_valid_protocol()) {
// 		pkt->pretty_print_base();
// 		pkt->pretty_print_netcache();
// 	}
// 	#endif

// 	return *pkt;
// }

// };	// namespace netcache
