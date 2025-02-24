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
#include <bf_rt/bf_rt.hpp>
#include <bf_switchd/bf_switchd.h>

#include "packet.h"
#include "process_query.h"
#include "constants.h"
#include "netcache.h"
/* #include "pcie.h" */

namespace netcache {

// std::shared_ptr<ProcessQuery> ProcessQuery::process_query;

ProcessQuery::ProcessQuery() {}

ProcessQuery::~ProcessQuery() {}

/* bool ProcessQuery::read_query(uint8_t *buffer, uint32_t size) { */
/* 	auto pkt = (pkt_hdr_t *)(buffer); */

/* 	bool valid = pkt->has_valid_protocol(); */

/* 	if (!valid) { */
/* 		#ifdef DEBUG */
/* 			printf("Invalid protocol packet. Ignoring.\n"); */
/* 		#endif */
/* 		return false; */
/* 	} */
/* 	pkt->pretty_print_base(); */

/* 	valid = (size >= (pkt->get_l2_size() */
/* 					  + pkt->get_l3_size() */
/* 					  + pkt->get_l4_size() */
/* 					  + pkt->get_netcache_hdr_size())); */

/* 	if (!valid) { */
/* 		#ifdef DEBUG */
/* 			printf("Packet too small. Ignoring.\n"); */
/* 		#endif */
/* 		return false; */
/* 	} */

/* 	auto nc_hdr = pkt->get_netcache_hdr(); */

/* 	if (nc_hdr->op == 0) { return true; } */
/* 	return false; */

/*     std::vector<std::vector<uint32_t>> sampl_cntr = sample_values(); */

/*  	// Retrieve the index of the smallest counter value from the vector. */
/*  	uint32_t smallest_val = std::numeric_limits<uint32_t>::max(); */
/*  	int smallest_idx = -1; */

/*  	for (size_t i = 0; i < sampl_cntr.size(); ++i) { */
/*  		if (sampl_cntr[i][1] < smallest_val) { */
/*  			smallest_val = sampl_cntr[i][1]; */
/*  			smallest_idx = static_cast<int>(i); */
/*  		} */
/*  	} */

/*  	// If the smallest counter value < HH report counter, */
/*     // update the data plane cache */
/*  	if (sampl_cntr[smallest_idx][1] < nc_hdr->val) { */
/* 		return true; */
/*         /1* update_cache(buf, sampl_cntr, smallest_idx, nc_hdr); *1/ */
/*  	} */
/* } */

void ProcessQuery::update_cache(struct netcache_hdr_t *nc_hdr) {

    if(!nc_hdr) { return; }

    #ifndef NDEBUG
        std::cout << "reply.key " << nc_hdr->key << "\n";
        std::cout << "reply.val " << nc_hdr->val << "\n";
    #endif

    // Update the key/value corresponding to the HH report directly on the switch.
    Controller::controller->reg_vtable.allocate(nc_hdr->key,
                                                nc_hdr->val);

	// Update the cache lookup table to delete the corresponding key.
	// Controller::controller->cache_lookup.del_entry(nc_hdr->key);
	Controller::controller->reg_cache_lookup.allocate((uint32_t)nc_hdr->key, 0);
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
