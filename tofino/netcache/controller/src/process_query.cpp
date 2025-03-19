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

void ProcessQuery::update_cache(netcache_hdr_t *nc_hdr) {
  if (!nc_hdr) {
    return;
  }

  // Check if the set is not empty.
  if (Controller::controller->available_keys.empty()) {
    std::cout << "Error: no empty positions in the data plane cache.";
    return;
  }

  // Pick a value from the set (i.e., the first).
  // Use that value as the index for the k/v to insert in the data plane.
  auto it = Controller::controller->available_keys.begin();

  // Add the index/key to the controller map.
  std::array<uint8_t, KV_KEY_SIZE> key;
  std::memcpy(key.data(), nc_hdr->key, sizeof(nc_hdr->key));

  if (Controller::controller->cached_keys.find(key) != Controller::controller->cached_keys.end()) {
    DEBUG("Key already in cache");
    return;
  }

  Controller::controller->key_storage[*it] = key;
  Controller::controller->cached_keys.insert(key);

  // Add the index/key to the data plane keys table and reset the key_count[index] register.
  Controller::controller->keys.add_entry(nc_hdr->key, *it);
  Controller::controller->reg_key_count.allocate(*it, 0);

  // Add the value to the value registers in the index position.
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
  Controller::controller->reg_v0_31.allocate(*it, update_v0_31);
  Controller::controller->reg_v32_63.allocate(*it, update_v32_63);
  Controller::controller->reg_v64_95.allocate(*it, update_v64_95);
  Controller::controller->reg_v96_127.allocate(*it, update_v96_127);
  Controller::controller->reg_v128_159.allocate(*it, update_v128_159);
  Controller::controller->reg_v160_191.allocate(*it, update_v160_191);
  Controller::controller->reg_v192_223.allocate(*it, update_v192_223);
  Controller::controller->reg_v224_255.allocate(*it, update_v224_255);
  Controller::controller->reg_v256_287.allocate(*it, update_v256_287);
  Controller::controller->reg_v288_319.allocate(*it, update_v288_319);
  Controller::controller->reg_v320_351.allocate(*it, update_v320_351);
  Controller::controller->reg_v352_383.allocate(*it, update_v352_383);
  Controller::controller->reg_v384_415.allocate(*it, update_v384_415);
  Controller::controller->reg_v416_447.allocate(*it, update_v416_447);
  Controller::controller->reg_v448_479.allocate(*it, update_v448_479);
  Controller::controller->reg_v480_511.allocate(*it, update_v480_511);
  Controller::controller->reg_v512_543.allocate(*it, update_v512_543);
  Controller::controller->reg_v544_575.allocate(*it, update_v544_575);
  Controller::controller->reg_v576_607.allocate(*it, update_v576_607);
  Controller::controller->reg_v608_639.allocate(*it, update_v608_639);
  Controller::controller->reg_v640_671.allocate(*it, update_v640_671);
  Controller::controller->reg_v672_703.allocate(*it, update_v672_703);
  Controller::controller->reg_v704_735.allocate(*it, update_v704_735);
  Controller::controller->reg_v736_767.allocate(*it, update_v736_767);
  Controller::controller->reg_v768_799.allocate(*it, update_v768_799);
  Controller::controller->reg_v800_831.allocate(*it, update_v800_831);
  Controller::controller->reg_v832_863.allocate(*it, update_v832_863);
  Controller::controller->reg_v864_895.allocate(*it, update_v864_895);
  Controller::controller->reg_v896_927.allocate(*it, update_v896_927);
  Controller::controller->reg_v928_959.allocate(*it, update_v928_959);
  Controller::controller->reg_v960_991.allocate(*it, update_v960_991);
  Controller::controller->reg_v992_1023.allocate(*it, update_v992_1023);
  // Remove the selected value from the set.
  Controller::controller->available_keys.erase(it);
}

std::vector<std::vector<uint32_t>> ProcessQuery::sample_values() {
  std::random_device rd;
  std::mt19937 gen(rd());

  // Generate k random indexes.

  std::uniform_int_distribution<> dis(1, Controller::controller->args.store_size);
  std::unordered_set<int> elems;

  while (elems.size() < Controller::controller->args.sample_size) {
    elems.insert(dis(gen));
  }

  std::vector<int> sampl_index(elems.begin(), elems.end());

  // Vector that stores all key indexes and counters randomly sampled from the switch.
  std::vector<std::vector<uint32_t>> sampl_vec;

  for (int i : sampl_index) {
    sampl_vec.push_back({(uint16_t)i, Controller::controller->reg_key_count.retrieve(i, false)});
  }

  return sampl_vec;
}

} // namespace netcache
