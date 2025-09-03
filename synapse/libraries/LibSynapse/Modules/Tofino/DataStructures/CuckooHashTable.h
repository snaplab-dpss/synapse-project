#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Hash.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>
#include <array>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

struct CuckooHashTable : public DS {
  static constexpr const u8 MAX_RECIRCULATIONS{4};
  static constexpr const u32 BLOOM_WIDTH{65536};
  static constexpr const bits_t BLOOM_VALUE_SIZE{16};
  static constexpr const bits_t SUPPORTED_KEY_SIZE{32};
  static constexpr const bits_t SUPPORTED_VALUE_SIZE{32};

  u32 total_capacity;
  u32 entries_per_cuckoo_table;
  bits_t cuckoo_index_size;
  bits_t cuckoo_bloom_index_size;

  std::array<Hash, 3> cuckoo_hashes;
  Register reg_k_1;
  Register reg_k_2;
  Register reg_v_1;
  Register reg_v_2;
  Register reg_ts_1;
  Register reg_ts_2;

  std::array<Hash, 3> bloom_hashes;
  Register swap_transient;
  Register swapped_transient;

  CuckooHashTable(const tna_properties_t &properties, DS_ID id, u32 op, u32 capacity);
  CuckooHashTable(const CuckooHashTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;
};

} // namespace Tofino
} // namespace LibSynapse