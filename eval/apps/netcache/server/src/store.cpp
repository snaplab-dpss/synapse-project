#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>

#include "packet.h"
#include "store.h"
#include "constants.h"
#include "server_reply.h"
#include "listener.h"

namespace netcache {

Store::Store(const int dpdk_port, rte_mempool * pool) {
	port_id = dpdk_port;
	mbuf_pool = pool;
}

Store::~Store() {}

void Store::test(pkt_hdr_t* pkt) {
	/* std::cout << "TEST START" << std::endl; */
	pkt->swap_addr();

	/* std::cout << "Z!" << std::endl; */
	buf[0] = rte_pktmbuf_alloc(mbuf_pool);
    /* struct pkt_hdr_t* pkt_reply = rte_pktmbuf_mtod(buf[0], pkt_hdr_t*); */
    rte_memcpy(rte_pktmbuf_mtod(buf[0], pkt_hdr_t*), pkt, sizeof(&pkt));
	/* std::cout << "B!" << std::endl; */

	/* pkt_reply = pkt; */

	/* if (pkt_reply->has_valid_protocol()) { */
		/* std::cout << "C!" << std::endl; */
	/* } */

	uint16_t num_tx =
		/* rte_eth_tx_burst(port_id, 0, buf, 1); */
		rte_eth_tx_burst(port_id, 0, buf, 1);
	if (num_tx > 0) {
		std::cout << "SENT! " << num_tx << std::endl;
	}

	rte_pktmbuf_free(buf[0]);
}

void Store::read_query(const pkt_hdr_t& pkt) {
	// Retrieve value from KV map using query.key.
	uint32_t val;
	auto it = kv_map.find(pkt.get_netcache_hdr()->key);

	if (it == kv_map.end()) {
		val = 0;
	} else {
		val = it->second;
	}

	pkt.get_netcache_hdr()->val = val;

	pkt_hdr_t pkt_reply = pkt;
	pkt.swap_addr();

	// Initialize a server_reply struct with the op, key and newly obtained value.
	/* server_reply_t reply = server_reply_t(query.key, val); */

	// Serialize server_reply and send it back.
	/* auto buffer = reply.serialize(); */

	buf[0] = rte_pktmbuf_alloc(mbuf_pool);
    rte_pktmbuf_mtod(buf[0], struct pkt_hdr_t*);

	uint16_t num_tx =
		rte_eth_tx_burst(port_id, 0, buf, 1);
}

/*
void Store::write_query(const query_t& query) {
	// Add/Update the value on the KV map using query.key.
	kv_map[query.key] = query.val;

	// If successful, send a confirmation to the controller.
	float ack = 1;

	auto sent_len =
		sendto(Listener::listener_ptr->sockfd, &ack, sizeof(ack), MSG_CONFIRM,
			   (const struct sockaddr*)&Listener::listener_ptr->ctrl_addr,
			   Listener::listener_ptr->ctrl_len);

	if (sent_len < sizeof(ack)) {
		fprintf(stderr, "Truncated packet.\n");
		exit(EXIT_FAILURE);
	}
}
*/

/*
void Store::del_query(const query_t& query) {
	// Delete the key/value from the KV map using query.key.
	kv_map.erase(query.key);

	// If successful, send a confirmation to the controller.
	float ack = 0;

	auto sent_len =
		sendto(Listener::listener_ptr->sockfd, &ack, sizeof(ack), MSG_CONFIRM,
			   (const struct sockaddr*)&Listener::listener_ptr->ctrl_addr,
			   Listener::listener_ptr->ctrl_len);

	if (sent_len < sizeof(ack)) {
		fprintf(stderr, "Truncated packet.\n");
		exit(EXIT_FAILURE);
	}
}
*/

}  // namespace netcache
