#include <LibSynapse/Modules/Tofino/DataStructures/CountMinSketch.h>

#include <cassert>
#include <iostream>

namespace LibSynapse {
namespace Tofino {

namespace {
bits_t index_size_from_width(u32 width) {
  // Log base 2 of the cache capacity
  // Assert cache capacity is a power of 2
  assert((width & (width - 1)) == 0 && "Cache capacity must be a power of 2");
  return bits_t(log2(width));
}

std::vector<Hash> build_hashes(DS_ID id, u32 height, const std::vector<bits_t> &keys) {
  std::vector<Hash> hashes;
  for (size_t i = 0; i < height; i++) {
    Hash hash(id + "_hash_" + std::to_string(i), keys);
    hashes.push_back(hash);
  }

  return hashes;
}

std::vector<Register> build_rows(const TNAProperties &properties, DS_ID id, u32 width, u32 height) {
  std::vector<Register> rows;
  bits_t hash_size    = index_size_from_width(width);
  bits_t counter_size = 32;

  for (size_t i = 0; i < height; i++) {
    Register row(properties, id + "_row_" + std::to_string(i), width, hash_size, counter_size,
                 {RegisterActionType::WRITE, RegisterActionType::READ});
    rows.push_back(row);
  }

  return rows;
}
} // namespace

CountMinSketch::CountMinSketch(const TNAProperties &properties, DS_ID _id, const std::vector<bits_t> &_keys, u32 _width, u32 _height)
    : DS(DSType::COUNT_MIN_SKETCH, false, _id), width(_width), height(_height), hashes(build_hashes(_id, _height, _keys)),
      rows(build_rows(properties, _id, _width, _height)) {}

CountMinSketch::CountMinSketch(const CountMinSketch &other)
    : DS(other.type, other.primitive, other.id), width(other.width), height(other.height), hashes(other.hashes), rows(other.rows) {}

DS *CountMinSketch::clone() const { return new CountMinSketch(*this); }

void CountMinSketch::debug() const {
  std::cerr << "\n";
  std::cerr << "======== COUNT MIN SKETCH ========\n";
  std::cerr << "ID:     " << id << "\n";
  std::cerr << "Width:  " << width << "\n";
  std::cerr << "Height: " << height << "\n";
  std::cerr << "==============================\n";
}

std::vector<std::unordered_set<const DS *>> CountMinSketch::get_internal() const {
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