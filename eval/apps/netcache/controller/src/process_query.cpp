#include <iostream>
#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
// #include <rte_ethdev.h>
// #include <rte_malloc.h>
// #include <rte_mbuf.h>
// #include <rte_lcore.h>
// #include <rte_eal.h>
// #include <rte_cycles.h>

#include "process_query.h"
#include "constants.h"
#include "server_reply.h"
#include "netcache.h"

namespace netcache {

ProcessQuery::ProcessQuery(const int dpdk_port) {
	port_id = dpdk_port;

	// mbuf_pool = rte_pktmbuf_pool_create(
	// 		"MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0,
	// 		RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
}

ProcessQuery::~ProcessQuery() {}

// void ProcessQuery::read_query(const pkt_hdr_t& pkt) {
// 	// Sample k values from the switch's cached item counter array.
// 	// k is defined in conf.kv.initial_entries.

// 	std::random_device rd;
// 	std::mt19937 gen(rd());

// 	std::uniform_int_distribution<> dis(1, Controller::controller->conf.kv.store_size);
// 	std::unordered_set<int> elems;

// 	while (elems.size() < Controller::controller->conf.kv.initial_entries) {
// 		elems.insert(dis(gen));
// 	}

// 	std::vector<int> sampl_index(elems.begin(), elems.end());

// 	// Vector that stores all key indexes and counters randomly sampled from the switch.
// 	std::vector<std::vector<uint32_t>> sampl_cntr;

// 	for (int i: sampl_index) {
// 		sampl_cntr.push_back({(uint32_t) i,
// 							   Controller::controller->reg_vtable.retrieve(i, false)});
// 	}

// 	// Retrieve the index of the smallest counter value from the vector.

// 	uint32_t smallest_val = std::numeric_limits<uint32_t>::max();
// 	int smallest_idx = -1;

// 	for (size_t i = 0; i < sampl_cntr.size(); ++i) {
// 		if (sampl_cntr[i][1] < smallest_val) {
// 			smallest_val = sampl_cntr[i][1];
// 			smallest_idx = static_cast<int>(i);
// 		}
// 	}

// 	// If the smallest counter value < HH report counter:
// 	if (sampl_cntr[smallest_idx][1] < pkt.get_netcache_hdr()->val) {
// 		// Evict the key corresponding to the smallest.
// 		Controller::controller->reg_vtable.allocate((uint32_t) sampl_cntr[smallest_idx][0], 0);

// 		// Retrieve the updated HH report value from the server.

// 		buf[0] = rte_pktmbuf_alloc(mbuf_pool);
// 		rte_pktmbuf_mtod(buf[0], struct pkt_hdr_t*);

// 		uint16_t num_tx =
// 			rte_eth_tx_burst(port_id, 0, buf, 1);

// 		sleep(1000);

// 		// TO-DO: need to add the correct port here.
// 		uint16_t nb_rx = rte_eth_rx_burst(1, 0, buf, BURST_SIZE);

// 		if (nb_rx == 0) { exit(1); }

// 		auto buffer = (uint8_t *)rte_pktmbuf_mtod(buf[0], char *);
// 		size_t data_size = rte_pktmbuf_data_len(buf[0]);

// 		/* rte_pktmbuf_free(buf[0]); */

// 		auto pkt_reply = (pkt_hdr_t *)(buffer);

// 		if (pkt_reply->has_valid_protocol()) {
// 			#ifndef NDEBUG
// 			std::cout << "reply.key " << pkt_reply->get_netcache_hdr()->key << "\n";
// 			std::cout << "reply.val " << pkt_reply->get_netcache_hdr()->val << "\n";
// 			#endif

// 			// Update the key/value corresponding to the HH report directly on the switch.
// 			Controller::controller->reg_vtable.allocate(pkt_reply->get_netcache_hdr()->key,
// 														pkt_reply->get_netcache_hdr()->val);

// 			// Update the cache lookup table to delete the corresponding key.
// 			Controller::controller->cache_lookup.del_entry(pkt_reply->get_netcache_hdr()->key);
// 		} else {
// 			std::cerr << "Invalid packet received.";
// 		}
// 	}
// }

}  // namespace netcache
