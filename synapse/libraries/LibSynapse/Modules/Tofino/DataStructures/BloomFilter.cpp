#include <LibSynapse/Modules/Tofino/DataStructures/BloomFilter.h>

#include <cassert>
#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::bits_from_pow2_capacity;

namespace {

std::vector<Hash> build_hashes(DS_ID id, u32 height, const std::vector<bits_t> &keys, bits_t hash_size) {
  std::vector<Hash> hashes;
  for (size_t i = 0; i < height; i++) {
    Hash hash(id + "_hash_" + std::to_string(i), keys, hash_size);
    hashes.push_back(hash);
  }

  return hashes;
}

std::vector<Register> build_rows(const tna_properties_t &properties, DS_ID id, u32 width, u32 height) {
  std::vector<Register> rows;
  const bits_t hash_size    = bits_from_pow2_capacity(width);
  const bits_t counter_size = 32;

  for (size_t i = 0; i < height; i++) {
    Register row(properties, id + "_row_" + std::to_string(i), width, hash_size, counter_size,
                 {
                     RegisterActionType::Read,
                     RegisterActionType::SetToOne,
                     RegisterActionType::SetToOneAndReturnOldValue,
                 });
    rows.push_back(row);
  }

  return rows;
}

} // namespace

const std::vector<u32> BloomFilter::HASH_SALTS = {0xfbc31fc7, 0x2681580b, 0x486d7e2f, 0x1f3a2b4d, 0x7c5e9f8b, 0x3a2b4d1f,
                                                  0x5e9f8b7c, 0x2b4d1f3a, 0x9f8b7c5e, 0xb4d1f3a2, 0x4d1f3a2b, 0x8b7c5e9f};

BloomFilter::BloomFilter(const tna_properties_t &properties, DS_ID _id, const std::vector<bits_t> &_keys, u32 _width, u32 _height)
    : DS(DSType::BloomFilter, false, _id), width(_width), height(_height), hash_size(bits_from_pow2_capacity(_width)),
      hashes(build_hashes(_id, _height, _keys, hash_size)), rows(build_rows(properties, _id, _width, _height)) {
  assert(width > 0 && "Width must be greater than 0");
  assert(height > 0 && "Height must be greater than 0");
  assert(hashes.size() == height && "Invalid number of hashes");
  assert(rows.size() == height && "Invalid number of rows");
}

BloomFilter::BloomFilter(const BloomFilter &other)
    : DS(other.type, other.primitive, other.id), width(other.width), height(other.height), hash_size(other.hash_size), hashes(other.hashes),
      rows(other.rows) {}

DS *BloomFilter::clone() const { return new BloomFilter(*this); }

void BloomFilter::debug() const {
  std::cerr << "\n";
  std::cerr << "======== BLOOM FILTER ========\n";
  std::cerr << "ID:     " << id << "\n";
  std::cerr << "Width:  " << width << "\n";
  std::cerr << "Height: " << height << "\n";
  std::cerr << "==============================\n";
}

std::vector<std::unordered_set<const DS *>> BloomFilter::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  assert(rows.size() == height && "Invalid number of rows");
  assert(hashes.size() == height && "Invalid number of hashes");

  for (size_t i = 0; i < height; i++) {
    internal_ds.emplace_back();
    internal_ds.back().insert(&hashes[i]);
    internal_ds.back().insert(&rows[i]);
  }

  return internal_ds;
}

} // namespace Tofino
} // namespace LibSynapse