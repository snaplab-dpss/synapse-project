#include <math.h>

#include "map_register.h"

namespace synapse {
namespace tofino {
namespace {
bits_t index_size_from_total_entries(u32 capacity) {
  // Log base 2 of the capacity
  // Assert capacity is a power of 2
  SYNAPSE_ASSERT((capacity & (capacity - 1)) == 0, "Capacity must be a power of 2");
  return bits_t(log2(capacity));
}

Register build_expirator(const TNAProperties &properties, DS_ID id, u32 capacity) {
  bits_t hash_size = index_size_from_total_entries(capacity);
  bits_t timestamp_size = 32;
  return Register(properties, id + "_expirator", capacity, hash_size, timestamp_size,
                  {RegisterAction::WRITE});
}

std::vector<Register> build_keys(const TNAProperties &properties, DS_ID id,
                                 const std::vector<bits_t> &keys_sizes, u32 capacity) {
  std::vector<Register> cache_keys;

  bits_t hash_size = index_size_from_total_entries(capacity);

  int i = 0;
  for (bits_t key_size : keys_sizes) {
    Register cache_key(properties, id + "_key_" + std::to_string(i), capacity, hash_size,
                       key_size, {RegisterAction::READ, RegisterAction::SWAP});
    i++;
    cache_keys.push_back(cache_key);
  }

  return cache_keys;
}
} // namespace

MapRegister::MapRegister(const TNAProperties &properties, DS_ID _id, u32 _num_entries,
                         const std::vector<bits_t> &_keys_sizes)
    : DS(DSType::MAP_REGISTER, false, _id), num_entries(_num_entries),
      expirator(build_expirator(properties, _id, _num_entries)),
      keys(build_keys(properties, id, _keys_sizes, _num_entries)) {
  SYNAPSE_ASSERT(num_entries > 0, "Number of entries must be greater than 0");
}

MapRegister::MapRegister(const MapRegister &other)
    : DS(other.type, other.primitive, other.id), num_entries(other.num_entries),
      expirator(other.expirator), keys(other.keys) {}

DS *MapRegister::clone() const { return new MapRegister(*this); }

void MapRegister::debug() const {
  Log::dbg() << "\n";
  Log::dbg() << "======== REGISTER MAP ========\n";
  Log::dbg() << "ID:      " << id << "\n";
  Log::dbg() << "Entries: " << num_entries << "\n";
  expirator.debug();
  for (const Register &key : keys) {
    key.debug();
  }
  Log::dbg() << "==============================\n";
}

std::vector<std::unordered_set<const DS *>> MapRegister::get_internal() const {
  // Access to the expirator comes first, then the keys.

  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  internal_ds.back().insert(&expirator);

  internal_ds.emplace_back();
  for (const Register &key : keys)
    internal_ds.back().insert(&key);

  return internal_ds;
}

} // namespace tofino
} // namespace synapse