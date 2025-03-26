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

struct HHTable : public DS {
  u32 capacity;
  std::vector<bits_t> keys_sizes;
  u32 bloom_width;
  u32 bloom_height;
  u32 cms_width;
  u32 cms_height;
  bits_t hash_size;

  std::vector<Table> tables;
  Register cached_counters;
  std::vector<Hash> hashes;
  std::vector<Register> bloom_filter;
  std::vector<Register> count_min_sketch;
  Register threshold;
  Digest digest;

  HHTable(const tna_properties_t &properties, DS_ID id, u32 op, u32 capacity, const std::vector<bits_t> &keys_sizes, u32 bloom_width,
          u32 bloom_height, u32 cms_width, u32 cms_height);

  HHTable(const HHTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(u32 op) const;
  DS_ID add_table(u32 op);
};

} // namespace Tofino
} // namespace LibSynapse