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

  // Add the index/key to the data plane keys table and reset the key_count[index] register.
  bool successfuly_added_key = Controller::controller->keys.try_add_entry(nc_hdr->key, *it);

  if (!successfuly_added_key) {
    DEBUG("Failed to add key to the keys table");
    return;
  }

  Controller::controller->reg_key_count.allocate(*it, 0);

  Controller::controller->key_storage[*it] = key;
  Controller::controller->cached_keys.insert(key);

  // Add the value to the value registers in the index position.
  uint32_t update_v;
  std::memcpy(&update_v, nc_hdr->val, sizeof(update_v));

  // Update the key/value corresponding to the HH report directly on the switch.
  Controller::controller->reg_v.allocate(*it, update_v);
  // Remove the selected value from the set.
  Controller::controller->available_keys.erase(it);
}

} // namespace netcache
