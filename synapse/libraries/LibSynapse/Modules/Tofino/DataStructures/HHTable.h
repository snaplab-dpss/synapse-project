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

constexpr const char *const HHTABLE_CMS_WIDTH_PARAM = "cms_width";

struct HHTable : public DS {
  static const std::vector<u32> HASH_SALTS;
  static const std::vector<u32> CMS_WIDTH_CANDIDATES;

  static constexpr const u32 CMS_HEIGHT{4};

  u32 capacity;
  std::vector<bits_t> keys_sizes;
  u32 cms_width;
  u32 cms_height;
  bits_t hash_size;

  std::vector<Table> tables;
  Register cached_counters;
  std::vector<Hash> hashes;
  std::vector<Register> count_min_sketch;
  Register threshold;
  Digest digest;

  HHTable(const tna_properties_t &properties, DS_ID id, u32 op, u32 capacity, const std::vector<bits_t> &keys_sizes, u32 cms_width, u32 cms_height,
          u8 digest_type);

  HHTable(const HHTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(u32 op) const;
  DS_ID add_table(u32 op);
  const Table *get_table(u32 op) const;
};

} // namespace Tofino
} // namespace LibSynapse