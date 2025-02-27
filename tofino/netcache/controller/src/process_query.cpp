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

namespace netcache {

ProcessQuery::ProcessQuery() {}

ProcessQuery::~ProcessQuery() {}

void ProcessQuery::update_cache(struct netcache_hdr_t *nc_hdr) {
    if(!nc_hdr) { return; }

    #ifndef DEBUG
        std::cout << "reply.key " << nc_hdr->key << "\n";
        std::cout << "reply.val " << nc_hdr->val << "\n";
    #endif

    std::array<uint8_t, 16> key_tmp;
    std::memcpy(key_tmp.data(), nc_hdr->key, sizeof(nc_hdr->key));	
	uint16_t update_hash = Controller::controller->hash_key[key_tmp];

	uint32_t update_k0_31, update_k32_63, update_k64_95, update_k96_127;
	std::memcpy(&update_k0_31, nc_hdr->key, sizeof(update_k0_31));
    std::memcpy(&update_k32_63, nc_hdr->key + 4, sizeof(update_k32_63));
    std::memcpy(&update_k64_95, nc_hdr->key + 8, sizeof(update_k64_95));
    std::memcpy(&update_k96_127, nc_hdr->key + 12, sizeof(update_k96_127));

	uint32_t update_v0_31, update_v32_63, update_v64_95, update_v96_127;
	uint32_t update_v128_159, update_v160_191, update_v192_223, update_v224_255;
	uint32_t update_v256_287, update_v288_319, update_v320_351, update_v352_383;
	uint32_t update_v384_415, update_v416_447, update_v448_479, update_v480_511;
	uint32_t update_v512_543, update_v544_575, update_v576_607, update_v608_639;
	uint32_t update_v640_671, update_v672_703, update_v704_735, update_v736_767;
	uint32_t update_v768_799, update_v800_831, update_v832_863, update_v864_895;
	uint32_t update_v896_927, update_v928_959, update_v960_991, update_v992_1023;
	std::memcpy(&update_v0_31, nc_hdr->val, sizeof(update_v0_31));
    std::memcpy(&update_v32_63, nc_hdr->val + 4, sizeof(update_v32_63));
    std::memcpy(&update_v64_95, nc_hdr->val + 8, sizeof(update_v64_95));
    std::memcpy(&update_v96_127, nc_hdr->val + 12, sizeof(update_v96_127));
    std::memcpy(&update_v128_159, nc_hdr->val + 16, sizeof(update_v128_159));
    std::memcpy(&update_v160_191, nc_hdr->val + 20, sizeof(update_v160_191));
    std::memcpy(&update_v192_223, nc_hdr->val + 24, sizeof(update_v192_223));
    std::memcpy(&update_v224_255, nc_hdr->val + 28, sizeof(update_v224_255));
    std::memcpy(&update_v256_287, nc_hdr->val + 32, sizeof(update_v256_287));
    std::memcpy(&update_v288_319, nc_hdr->val + 36, sizeof(update_v288_319));
    std::memcpy(&update_v320_351, nc_hdr->val + 40, sizeof(update_v320_351));
    std::memcpy(&update_v352_383, nc_hdr->val + 44, sizeof(update_v352_383));
    std::memcpy(&update_v384_415, nc_hdr->val + 48, sizeof(update_v384_415));
    std::memcpy(&update_v416_447, nc_hdr->val + 52, sizeof(update_v416_447));
    std::memcpy(&update_v448_479, nc_hdr->val + 56, sizeof(update_v448_479));
    std::memcpy(&update_v480_511, nc_hdr->val + 60, sizeof(update_v480_511));
    std::memcpy(&update_v512_543, nc_hdr->val + 64, sizeof(update_v512_543));
    std::memcpy(&update_v544_575, nc_hdr->val + 68, sizeof(update_v544_575));
    std::memcpy(&update_v576_607, nc_hdr->val + 72, sizeof(update_v576_607));
    std::memcpy(&update_v608_639, nc_hdr->val + 76, sizeof(update_v608_639));
    std::memcpy(&update_v640_671, nc_hdr->val + 80, sizeof(update_v640_671));
    std::memcpy(&update_v672_703, nc_hdr->val + 84, sizeof(update_v672_703));
    std::memcpy(&update_v704_735, nc_hdr->val + 88, sizeof(update_v704_735));
    std::memcpy(&update_v736_767, nc_hdr->val + 92, sizeof(update_v736_767));
    std::memcpy(&update_v768_799, nc_hdr->val + 96, sizeof(update_v768_799));
    std::memcpy(&update_v800_831, nc_hdr->val + 100, sizeof(update_v800_831));
    std::memcpy(&update_v832_863, nc_hdr->val + 104, sizeof(update_v832_863));
    std::memcpy(&update_v864_895, nc_hdr->val + 108, sizeof(update_v864_895));
    std::memcpy(&update_v896_927, nc_hdr->val + 112, sizeof(update_v896_927));
    std::memcpy(&update_v928_959, nc_hdr->val + 116, sizeof(update_v928_959));
    std::memcpy(&update_v960_991, nc_hdr->val + 120, sizeof(update_v960_991));
    std::memcpy(&update_v992_1023, nc_hdr->val + 124, sizeof(update_v992_1023));

    // Update the key/value corresponding to the HH report directly on the switch.
	// Key.
	Controller::controller->reg_k0_31.allocate(update_hash, update_k0_31);
	Controller::controller->reg_k32_63.allocate(update_hash, update_k32_63);
	Controller::controller->reg_k64_95.allocate(update_hash, update_k64_95);
	Controller::controller->reg_k96_127.allocate(update_hash, update_k96_127);
	// Value.
	Controller::controller->reg_v0_31.allocate(update_hash, update_v0_31);
	Controller::controller->reg_v32_63.allocate(update_hash, update_v32_63);
	Controller::controller->reg_v64_95.allocate(update_hash, update_v64_95);
	Controller::controller->reg_v96_127.allocate(update_hash, update_v96_127);
	Controller::controller->reg_v128_159.allocate(update_hash, update_v128_159);
	Controller::controller->reg_v160_191.allocate(update_hash, update_v160_191);
	Controller::controller->reg_v192_223.allocate(update_hash, update_v192_223);
	Controller::controller->reg_v224_255.allocate(update_hash, update_v224_255);
	Controller::controller->reg_v256_287.allocate(update_hash, update_v256_287);
	Controller::controller->reg_v288_319.allocate(update_hash, update_v288_319);
	Controller::controller->reg_v320_351.allocate(update_hash, update_v320_351);
	Controller::controller->reg_v352_383.allocate(update_hash, update_v352_383);
	Controller::controller->reg_v384_415.allocate(update_hash, update_v384_415);
	Controller::controller->reg_v416_447.allocate(update_hash, update_v416_447);
	Controller::controller->reg_v448_479.allocate(update_hash, update_v448_479);
	Controller::controller->reg_v480_511.allocate(update_hash, update_v480_511);
	Controller::controller->reg_v512_543.allocate(update_hash, update_v512_543);
	Controller::controller->reg_v544_575.allocate(update_hash, update_v544_575);
	Controller::controller->reg_v576_607.allocate(update_hash, update_v576_607);
	Controller::controller->reg_v608_639.allocate(update_hash, update_v608_639);
	Controller::controller->reg_v640_671.allocate(update_hash, update_v640_671);
	Controller::controller->reg_v672_703.allocate(update_hash, update_v672_703);
	Controller::controller->reg_v704_735.allocate(update_hash, update_v704_735);
	Controller::controller->reg_v736_767.allocate(update_hash, update_v736_767);
	Controller::controller->reg_v768_799.allocate(update_hash, update_v768_799);
	Controller::controller->reg_v800_831.allocate(update_hash, update_v800_831);
	Controller::controller->reg_v832_863.allocate(update_hash, update_v832_863);
	Controller::controller->reg_v864_895.allocate(update_hash, update_v864_895);
	Controller::controller->reg_v896_927.allocate(update_hash, update_v896_927);
	Controller::controller->reg_v928_959.allocate(update_hash, update_v928_959);
	Controller::controller->reg_v960_991.allocate(update_hash, update_v960_991);
	Controller::controller->reg_v992_1023.allocate(update_hash, update_v992_1023);
	// Cache lookup.
	Controller::controller->reg_cache_lookup.allocate(update_hash, 1);
}

// Sample k values from the switch's cached item counter array.
// k is defined in conf.kv.initial_entries.
std::vector<std::vector<uint32_t>> ProcessQuery::sample_values() {
 	std::random_device rd;
 	std::mt19937 gen(rd());

	// Generate k random indexes.

 	std::uniform_int_distribution<> dis(1, Controller::controller->conf.kv.store_size);
 	std::unordered_set<int> elems;

 	while (elems.size() < Controller::controller->conf.kv.initial_entries) {
 		elems.insert(dis(gen));
 	}

 	std::vector<int> sampl_index(elems.begin(), elems.end());

 	// Vector that stores all key indexes and counters randomly sampled from the switch.
	std::vector<std::vector<uint32_t>> sampl_vec;

	for (int i: sampl_index) {
 		sampl_vec.push_back({(uint32_t) i,
 							  Controller::controller->reg_key_count.retrieve(i, false)});
 	}

    return sampl_vec;
}

}  // namespace netcache
