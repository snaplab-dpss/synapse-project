#include <array>
#include <optional>
#include <unordered_set>
#include <thread>

#include "../../include/sycon/data_structures/count_min_sketch.h"
#include "../../include/sycon/config.h"

namespace sycon {

const std::vector<u32> CountMinSketch::HASH_SALTS = {0xfbc31fc7, 0x2681580b, 0x486d7e2f, 0x1f3a2b4d, 0x7c5e9f8b, 0x3a2b4d1f,
                                                     0x5e9f8b7c, 0x2b4d1f3a, 0x9f8b7c5e, 0xb4d1f3a2, 0x4d1f3a2b, 0x8b7c5e9f};

CountMinSketch::CountMinSketch(const std::string &_name, const std::vector<std::string> &_rows_names, time_ms_t _periodic_cleanup_interval)
    : SynapseDS(_name), rows(build_rows(_rows_names)), height(_rows_names.size()), width(get_width(rows)),
      periodic_cleanup_interval(_periodic_cleanup_interval), hash_salts(build_hash_salts(rows)), hash_mask(build_hash_mask(width)) {
  assert(rows.size() == height);
  assert(hash_salts.size() == height);

  std::thread([this]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(periodic_cleanup_interval));
      cfg.begin_transaction();
      cleanup();
      cfg.commit_transaction();
    }
  }).detach();
}

std::vector<Register> CountMinSketch::build_rows(const std::vector<std::string> &rows_names) {
  assert(!rows_names.empty() && "CMS register names must not be empty");

  std::vector<Register> rows;
  for (const std::string &name : rows_names) {
    rows.emplace_back(name);
  }

  return rows;
}

u32 CountMinSketch::get_width(const std::vector<Register> &rows) {
  const u32 capacity = rows.back().get_capacity();

  for (const Register &row : rows) {
    assert(row.get_capacity() == capacity);
  }

  return capacity;
}

std::vector<buffer_t> CountMinSketch::build_hash_salts(const std::vector<Register> &rows) {
  const size_t num_hashes = rows.size();
  assert(HASH_SALTS.size() >= num_hashes && "Not enough hash salts defined");

  std::vector<buffer_t> hash_salts;
  for (size_t i = 0; i < num_hashes; i++) {
    buffer_t hash_salt(4);
    hash_salt.set(0, 4, HASH_SALTS[i]);
    hash_salts.push_back(hash_salt);
  }

  return hash_salts;
}

u32 CountMinSketch::build_hash_mask(u32 width) {
  auto hash_size_from_capacity = [](size_t capacity) {
    assert((capacity & (capacity - 1)) == 0 && "Hash size must be a power of 2");
    bits_t hash_size = 0;
    while (capacity >>= 1) {
      hash_size++;
    }
    return hash_size;
  };

  const bits_t hash_size = hash_size_from_capacity(width);
  const u32 hash_mask    = (1ULL << hash_size) - 1;

  return hash_mask;
}

std::vector<u32> CountMinSketch::calculate_hashes(const buffer_t &key) {
  std::vector<u32> hashes;
  for (const buffer_t &hash_salt : hash_salts) {
    const buffer_t hash_input = key.append(hash_salt);
    const u32 hash            = crc32.hash(hash_input) & hash_mask;
    hashes.push_back(hash);
  }
  return hashes;
}

u32 CountMinSketch::count_min(const buffer_t &key) {
  const std::vector<u32> hashes = calculate_hashes(key);

  u32 min = std::numeric_limits<u32>::max();
  for (size_t i = 0; i < height; i++) {
    min = std::min(min, rows[i].get_max(hashes[i]));
  }

  return min;
}

void CountMinSketch::cleanup() {
  LOG_DEBUG("Cleaning CMS...");

  for (Register &row : rows) {
    row.overwrite_all_entries(0);
  }
}

} // namespace sycon