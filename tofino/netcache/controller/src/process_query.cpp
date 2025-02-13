#include <iostream>
#include <cstdint>
#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "packet.h"
#include "process_query.h"
#include "constants.h"
#include "netcache.h"

namespace netcache {

ProcessQuery::ProcessQuery(const int in, const int out, rte_mempool *pool) {
	port_in = in;
	port_out = out;
	mbuf_pool = pool;
}

ProcessQuery::~ProcessQuery() {}

void ProcessQuery::read_query() {
	uint16_t nb_rx = rte_eth_rx_burst(port_in, 0, buf, BURST_SIZE);

	if (nb_rx == 0) {
		return;
	}

    struct netcache_hdr_t *nc_hdr = check_pkt(buf[0]);
    if(!nc_hdr) { return; }

    std::vector<std::vector<uint32_t>> sampl_cntr = sample_values();

 	// Retrieve the index of the smallest counter value from the vector.
 	uint32_t smallest_val = std::numeric_limits<uint32_t>::max();
 	int smallest_idx = -1;

 	for (size_t i = 0; i < sampl_cntr.size(); ++i) {
 		if (sampl_cntr[i][1] < smallest_val) {
 			smallest_val = sampl_cntr[i][1];
 			smallest_idx = static_cast<int>(i);
 		}
 	}

 	// If the smallest counter value < HH report counter,
    // update the data plane cache
 	if (sampl_cntr[smallest_idx][1] < nc_hdr->val) {
        update_cache(buf, sampl_cntr, smallest_idx, nc_hdr);
 	}
}

struct netcache_hdr_t* ProcessQuery::check_pkt(struct rte_mbuf *mbuf) {
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_hdr;
    struct rte_udp_hdr *udp_hdr;
    struct rte_tcp_hdr *tcp_hdr;

    if (mbuf->pkt_len < sizeof(struct rte_ether_hdr)) {
        #ifndef NDEBUG
            std::cout << "Received pkt: not enough data for ethernet header." << std::endl;
        #endif
        return nullptr;
    }

    eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

    if (eth_hdr->ether_type != rte_be_to_cpu_16(ETHER_TYPE_IPV4)) {
        if (mbuf->pkt_len < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)) {
            #ifndef NDEBUG
                std::cout << "Received pkt: not enough data for IPv4 header." << std::endl;
            #endif
            return nullptr;
        }
    }

    ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));

    if (ipv4_hdr->next_proto_id != UDP_PROTO || ipv4_hdr->next_proto_id !=TCP_PROTO) {
        return nullptr;
    }

    if (ipv4_hdr->next_proto_id == UDP_PROTO) {
        if (mbuf->pkt_len < sizeof(struct rte_ether_hdr)
                            + sizeof(struct rte_ipv4_hdr)
                            + sizeof(struct rte_udp_hdr)) {
            #ifndef NDEBUG
                std::cout << "Received pkt: not enough data for udp header." << std::endl;
            #endif
            return nullptr;
        }
        udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
        if (udp_hdr->src_port == rte_be_to_cpu_16(NC_PORT)
                    || udp_hdr->dst_port == rte_be_to_cpu_16(NC_PORT)) {
            if (buf[0]->pkt_len >= sizeof(struct rte_ether_hdr)
                                + sizeof(struct rte_ipv4_hdr)
                                + sizeof(struct rte_udp_hdr)
                                + NC_HDR_SIZE) {
                uint8_t *nc_hdr_ptr = (uint8_t *)(udp_hdr + 1);
                struct netcache_hdr_t *nc_hdr = (struct netcache_hdr_t *)nc_hdr_ptr;
                return nc_hdr;
            } else {
                #ifndef NDEBUG
                    std::cout << "Received pkt: not enough data for netcache header." << std::endl;
                #endif
            }
        }
    }

    if (ipv4_hdr->next_proto_id == TCP_PROTO) {
        if (mbuf->pkt_len < sizeof(struct rte_ether_hdr)
                            + sizeof(struct rte_ipv4_hdr)
                            + sizeof(struct rte_tcp_hdr)) {
            #ifndef NDEBUG
                std::cout << "Received pkt: not enough data for tcp header." << std::endl;
            #endif
            return nullptr;
        }
        tcp_hdr = (struct rte_tcp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
        if (tcp_hdr->src_port == rte_be_to_cpu_16(NC_PORT)
                    || tcp_hdr->dst_port == rte_be_to_cpu_16(NC_PORT)) {
            if (buf[0]->pkt_len >= sizeof(struct rte_ether_hdr)
                                + sizeof(struct rte_ipv4_hdr)
                                + sizeof(struct rte_tcp_hdr)
                                + NC_HDR_SIZE) {
                uint8_t *nc_hdr_ptr = (uint8_t *)(tcp_hdr + 1);
                struct netcache_hdr_t *nc_hdr = (struct netcache_hdr_t *)nc_hdr_ptr;
                return nc_hdr;
            } else {
                #ifndef NDEBUG
                    std::cout << "Received pkt: not enough data for netcache header." << std::endl;
                #endif
            }
        }
    }

    return nullptr;
}

void ProcessQuery::update_cache(struct rte_mbuf **buf,
                                std::vector<std::vector<uint32_t>> sampl_cntr,
                                int smallest_idx,
                                struct netcache_hdr_t *nc_hdr) {

    // Send the HH report to the server.
    uint16_t nb_tx = rte_eth_tx_burst(port_out, 0, buf, 1);

	if (nb_tx > 0) {
        #ifndef NDEBUG
		    std::cout << "Sent " << nb_tx << "packet(s)." << std::endl;
        #endif
        /* rte_pktmbuf_free(buf[0]); */
	}

    sleep(1000);

    // Retrieve the updated HH report value from the server.
    uint16_t nb_rx = rte_eth_rx_burst(port_in, 0, buf, BURST_SIZE);

    if (nb_rx == 0) { return; }

    struct netcache_hdr_t *nc_hdr_reply = check_pkt(buf[0]);
    if(!nc_hdr_reply) { return; }

    // Evict the key corresponding to the smallest.
    Controller::controller->reg_vtable.allocate((uint32_t) sampl_cntr[smallest_idx][0], 0);

    #ifndef NDEBUG
        std::cout << "reply.key " << nc_hdr_reply->key << "\n";
        std::cout << "reply.val " << nc_hdr_reply->val << "\n";
    #endif

    // Update the key/value corresponding to the HH report directly on the switch.
    Controller::controller->reg_vtable.allocate(nc_hdr_reply->key,
                                                nc_hdr_reply->val);

    // Update the cache lookup table to delete the corresponding key.
    Controller::controller->cache_lookup.del_entry(nc_hdr_reply->key);
}

// Sample k values from the switch's cached item counter array.
// k is defined in conf.kv.initial_entries.
std::vector<std::vector<uint32_t>> ProcessQuery::sample_values() {
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

    return sampl_cntr;
}

}  // namespace netcache
