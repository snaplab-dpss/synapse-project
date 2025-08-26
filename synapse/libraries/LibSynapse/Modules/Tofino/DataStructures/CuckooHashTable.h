#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Hash.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Digest.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

struct CuckooHashTable : public DS {
  static const std::vector<u32> HASH_SALTS;

  static constexpr const u32 CMS_WIDTH{1024};
  static constexpr const u32 CMS_HEIGHT{4};

  static constexpr const bits_t SUPPORTED_KEY_SIZE{32};
  static constexpr const bits_t SUPPORTED_VALUE_SIZE{32};

  u32 capacity;
  bits_t hash_size;

  std::vector<Table> tables;
  Register cached_counters;
  std::vector<Hash> hashes;
  std::vector<Register> count_min_sketch;
  Register threshold;

  CuckooHashTable(const tna_properties_t &properties, DS_ID id, u32 op, u32 capacity);

  CuckooHashTable(const CuckooHashTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(u32 op) const;
  DS_ID add_table(u32 op);
  const Table *get_table(u32 op) const;
};

} // namespace Tofino
} // namespace LibSynapse