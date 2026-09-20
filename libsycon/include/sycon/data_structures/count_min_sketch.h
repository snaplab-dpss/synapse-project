#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "synapse_ds.h"
#include "../primitives/table.h"
#include "../primitives/register.h"
#include "../time.h"
#include "../field.h"
#include "../hash.h"

namespace sycon {

class CountMinSketch : public SynapseDS {
private:
  std::vector<Register> rows;

  const u32 height;
  const u32 width;
  const time_ms_t periodic_cleanup_interval;

  const std::vector<CRC32> row_hashers; // One polynomial per row (CRC32::per_row).
  const u32 hash_mask;

public:
  CountMinSketch(const std::string &_name, const std::vector<std::string> &_rows_names, time_ms_t _periodic_cleanup_interval);

  u32 count_min(const buffer_t &key);
  void cleanup();

private:
  static std::vector<Register> build_rows(const std::vector<std::string> &rows_names);
  static u32 get_width(const std::vector<Register> &rows);

  static u32 build_hash_mask(u32 width);

  std::vector<u32> calculate_hashes(const buffer_t &key);
};

} // namespace sycon